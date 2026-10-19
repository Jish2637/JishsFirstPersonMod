#include "update_core.h"
#include "proxy_digest.h"
#include <Windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <fstream>
#include <stdexcept>
namespace fs=std::filesystem;using namespace jishsfirstpersonmod::updates;
namespace {
void notify(const std::wstring& message,bool error){MessageBoxW(nullptr,message.c_str(),L"Jish's First Person Mod update",MB_OK|(error?MB_ICONERROR:MB_ICONINFORMATION)|MB_SETFOREGROUND);}
std::string text(const fs::path& p,std::size_t limit=1024*1024){auto b=read_file(p,limit);return {b.begin(),b.end()};}
void no_game(const fs::path& root) {
 const auto snap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snap==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot inspect running processes");
 PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);bool exists=false;
 if(Process32FirstW(snap,&entry))do {
  if(_wcsicmp(entry.szExeFile,L"Skate.exe")!=0)continue;
  const auto process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID);
  if(!process){exists=true;break;}
  wchar_t path[32768]{};DWORD size=32768;
  const bool queried=QueryFullProcessImageNameW(process,0,path,&size)!=0;CloseHandle(process);
  if(!queried || fs::weakly_canonical(fs::path(path).parent_path())==fs::weakly_canonical(root)){exists=true;break;}
 }while(Process32NextW(snap,&entry));CloseHandle(snap);
 if(exists)throw std::runtime_error("Skate is running again. Close it and retry; no update was installed.");
}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
 fs::path root,job;HANDLE game{},mutex{};bool acquired=false;
 try {
  int count{};auto argv=CommandLineToArgvW(GetCommandLineW(),&count);if(!argv)return 1;if(count!=7){LocalFree(argv);throw std::runtime_error("Installer must be queued from the mod menu");}
  DWORD pid{};
  for(int i=1;i+1<count;i+=2) {
   const std::wstring key=argv[i];if(key==L"--root")root=argv[i+1];else if(key==L"--job")job=argv[i+1];else if(key==L"--pid")pid=std::stoul(argv[i+1]);else {LocalFree(argv);throw std::runtime_error("Invalid installer argument");}
  }LocalFree(argv);
  if(root.empty() || job.empty() || !pid)throw std::runtime_error("Installer must be queued from the mod menu");
  root=fs::absolute(root).lexically_normal();job=fs::absolute(job).lexically_normal();safe_path(root);safe_path(job);
  const auto jobs=root/L"scripts/Custom/FirstPerson/updates/jobs";
  if(job.parent_path()!=jobs || !job.filename().wstring().starts_with(L"update-"))throw std::runtime_error("Unsafe installer job path");
  game=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!game)throw std::runtime_error("Cannot bind installer to the running game; queue the update again");
  wchar_t image[32768]{};DWORD length=32768;
  if(!QueryFullProcessImageNameW(game,0,image,&length) || fs::weakly_canonical(image)!=fs::weakly_canonical(root/L"Skate.exe"))throw std::runtime_error("Installer parent is not this Skate installation");
  const auto release=parse_release(text(job/L"release.json"));unpack(read_file(job/L"archive.zip"),release);
  const auto expected=text(job/L"expected-proxy.txt",256);
  if(expected!="d77068ccd3d489a5649982c18de7d5e2fbd208fbb4f311382f05e81f1864e13d" && expected!=bundled_proxy_sha256)throw std::runtime_error("Unsupported DirectInput loader");
  write_text(job/L"waiting.txt","Waiting for Skate to close; no relaunch.\n");
  for(;;) {
   if(fs::exists(job/L"cancel.txt")){write_text(job/L"cancelled.txt","Cancelled; download retained.\n");CloseHandle(game);game=nullptr;return 0;}
   const auto result=WaitForSingleObject(game,250);if(result==WAIT_OBJECT_0)break;if(result!=WAIT_TIMEOUT)throw std::runtime_error("Game wait failed");
  }CloseHandle(game);game=nullptr;
  if(fs::exists(job/L"cancel.txt")){write_text(job/L"cancelled.txt","Cancelled; download retained.\n");return 0;}
  // Serialize installers without a service or any autorun/boot-time task.
  const auto root_text=root.u8string();const auto id=sha256(Bytes(root_text.begin(),root_text.end()));
  const std::wstring name=L"Local\\JishFirstPersonUpdater-"+std::wstring(id.begin(),id.end());
  mutex=CreateMutexW(nullptr,FALSE,name.c_str());if(!mutex)throw std::runtime_error("Cannot create installer lock");const auto locked=WaitForSingleObject(mutex,0);if(locked!=WAIT_OBJECT_0 && locked!=WAIT_ABANDONED)throw std::runtime_error("Another installer is already active");acquired=true;
  no_game(root);
  if(sha256(read_file(root/L"ReSkate.dll"))!="d79ff144768ce8b0a8e992c9f134c670a3ff85bb3b437a0eb915876973014301" ||
     sha256(read_file(root/L"Skate.exe",256*1024*1024))!="fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9")throw std::runtime_error("The installed game/ReSkate build changed; update refused");
  if(fs::exists(job/L"transaction.json"))recover(root,job);
  if(!newer(release,installed_version(root)))throw std::runtime_error("This update is no longer newer than the installed manifest");
  install(root,job,release,expected);
  ReleaseMutex(mutex);CloseHandle(mutex);mutex=nullptr;acquired=false;
  notify(L"First Person Mod "+std::wstring(release.version.begin(),release.version.end())+L" was installed successfully.\n\nYou can start ReSkate when ready.",false);return 0;
 }catch(const std::exception& error) {
  if(game)CloseHandle(game);if(acquired)ReleaseMutex(mutex);if(mutex)CloseHandle(mutex);
  try{if(!job.empty())write_text(job/L"error.txt",error.what());}catch(...){}
  const std::string s=error.what();notify(L"The update could not complete:\n\n"+std::wstring(s.begin(),s.end())+L"\n\nThe downloaded ZIP and any backups are retained in the mod's updates folder.",true);return 1;
 }
}
