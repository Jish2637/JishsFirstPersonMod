#include "release_updates.h"
#include "update_core.h"
#include "loader_update_policy.h"
#include "mod.h"
#include <Windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <rapidjson/document.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <fstream>
#include <chrono>
namespace jishsfirstpersonmod::release_updates {
namespace fs=std::filesystem;using namespace updates;
namespace {
std::mutex mutex;Snapshot state;
fs::path root,data_path,job_path;std::string metadata;Release remote;
std::atomic<bool> stop{},usable{};bool started{};

void status(const std::string& text) {std::lock_guard lock(mutex);state.status=text;}
std::wstring wide(const std::string& s) {
 const int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);
 if(count<=0)throw std::runtime_error("Invalid UTF-8 URL");std::wstring out(count,0);
 MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),out.data(),count);return out;
}
struct Http {HINTERNET h{};~Http(){if(h)WinHttpCloseHandle(h);}operator HINTERNET() const{return h;}};
#ifdef JISH_UPDATER_TEST
Bytes test_fetch(const std::wstring&,std::size_t,bool);
Bytes fetch(std::wstring url,std::size_t limit,bool asset){return test_fetch(url,limit,asset);}
#else
Bytes fetch(std::wstring url,std::size_t limit,bool asset) {
 const auto deadline=GetTickCount64()+(asset?120000:30000);
 for(unsigned redirects=0;redirects<6;++redirects) {
  if(stop)throw std::runtime_error("Update cancelled");
  const auto address=https_request(url);const auto& host=address.host;const auto& path=address.target;
  Http session{WinHttpOpen(L"JishsFirstPersonMod-Updater/1.36.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
  if(!session.h)throw std::runtime_error("Could not open HTTPS session");WinHttpSetTimeouts(session,8000,8000,8000,8000);
  Http connection{WinHttpConnect(session,host.c_str(),443,0)};
  Http request{connection.h?WinHttpOpenRequest(connection,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr};
  if(!request.h)throw std::runtime_error("Could not open update request");
  DWORD redirects_off=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&redirects_off,sizeof(redirects_off));
  const wchar_t* headers=asset?L"Accept: application/octet-stream\r\n":L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
  if(!WinHttpSendRequest(request,headers,static_cast<DWORD>(-1),nullptr,0,0,0) || !WinHttpReceiveResponse(request,nullptr))throw std::runtime_error("GitHub connection failed (offline, timeout or TLS)");
  DWORD code{},size=sizeof(code);
  if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&code,&size,nullptr))throw std::runtime_error("Could not read HTTP response");
  if(code==301 || code==302 || code==303 || code==307 || code==308) {
   wchar_t location[16384]{};DWORD bytes=sizeof(location);
   if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_LOCATION,nullptr,location,&bytes,nullptr))throw std::runtime_error("Invalid update redirect");
   url=location;if(GetTickCount64()>deadline)throw std::runtime_error("Update request timed out");continue;
  }
  if(code!=200)throw std::runtime_error("GitHub returned HTTP "+std::to_string(code));
  Bytes result;
  for(;;) {
   if(stop)throw std::runtime_error("Update cancelled");if(GetTickCount64()>deadline)throw std::runtime_error("Update request timed out");
   unsigned char buffer[65536];DWORD count{};
   if(!WinHttpReadData(request,buffer,sizeof(buffer),&count))throw std::runtime_error("Update transfer interrupted");if(!count)break;
   if(result.size()+count>limit)throw std::runtime_error("Update exceeds size limit");result.insert(result.end(),buffer,buffer+count);
   if(asset){std::lock_guard lock(mutex);state.received=result.size();}
  }
  return result;
 }
 throw std::runtime_error("Too many update redirects");
}
#endif

std::string own_proxy() {
 const auto hash=sha256(read_file(root/L"dinput8.dll"));
 if(!supported_update_loader(hash))throw std::runtime_error("Unknown dinput8.dll loader; automatic replacement disabled");return hash;
}
void verify_runtime() {
 if(sha256(read_file(root/L"ReSkate.dll"))!=supported_update_runtime_sha256)throw std::runtime_error("Only the exact official ReSkate 2.1.3 runtime is supported");
}
void save_preferences() {write_text(data_path/L"preferences.ini",snapshot().automatic?"automatic_check=1\n":"automatic_check=0\n");}
void check_worker() {
 auto installed=installed_version(root);{std::lock_guard lock(mutex);state.installed=installed;state.available=false;state.latest.clear();}
 auto bytes=fetch(L"https://api.github.com/repos/Jish2637/JishsFirstPersonMod/releases/latest",1024*1024,false);
 if(stop)throw std::runtime_error("Update check cancelled");
 std::string json(bytes.begin(),bytes.end());const auto release=parse_release(json);
 {std::lock_guard lock(mutex);remote=release;metadata=std::move(json);state.latest=release.version;state.available=newer(release,installed);state.status=state.available ? "Version "+release.version+" is available":"Up to date ("+installed+")";}
}
void download_worker() {
 verify_runtime();own_proxy();Release release;std::string json;
 {std::lock_guard lock(mutex);release=remote;json=metadata;state.received=0;state.total=release.size;state.ready=false;}
 const auto bytes=fetch(wide(release.url),archive_limit,true);
 status("Verifying download and package...");const auto contents=unpack(bytes,release);
 if(stop)throw std::runtime_error("Update cancelled");
 const auto job=data_path/L"jobs"/(L"update-"+wide(release.version)+L"-"+std::to_wstring(GetTickCount64()));safe_path(job);
 write_atomic(job/L"archive.zip",bytes);write_text(job/L"release.json",json);
 if(stop)throw std::runtime_error("Update cancelled; verified download retained");
 write_text(data_path/L"pending.txt",job.filename().string());
 {std::lock_guard lock(mutex);job_path=job;state.ready=true;state.available=false;state.status="Update verified. Choose Install after I close Skate.";}
}
std::wstring quoted(const fs::path& path) {
 const auto s=path.wstring();if(s.find(L'"')!=s.npos || s.ends_with(L'\\'))throw std::runtime_error("Invalid updater path");return L"\""+s+L"\"";
}
void arm_worker() {
 verify_runtime();const auto proxy=own_proxy();fs::path job;{std::lock_guard lock(mutex);job=job_path;}
 if(job.empty())throw std::runtime_error("No verified update is staged");
 const auto source=root/L"scripts/Custom/FirstPerson/updater/FirstPersonUpdater.exe";
 const auto helper=job/L"FirstPersonUpdater.exe";
 for(const auto* marker:{L"waiting.txt",L"cancel.txt",L"cancelled.txt",L"error.txt"}) {const auto path=job/marker;safe_path(path);if(fs::exists(path) && !DeleteFileW(path.c_str()))throw std::runtime_error("Cannot clear previous installer state");}
 write_atomic(helper,read_file(source,8*1024*1024));if(!fs::exists(job/L"transaction.json"))write_text(job/L"expected-proxy.txt",proxy);
 // The helper waits on this exact process; it never terminates or relaunches it.
 if(stop)throw std::runtime_error("Installer queuing cancelled; download retained");
 const auto args=quoted(helper)+L" --job "+quoted(job)+L" --root "+quoted(root)+L" --pid "+std::to_wstring(GetCurrentProcessId());
 STARTUPINFOW info{};info.cb=sizeof(info);info.dwFlags=STARTF_USESHOWWINDOW;info.wShowWindow=SW_HIDE;PROCESS_INFORMATION child{};
 auto command=args;
 if(!CreateProcessW(helper.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,job.c_str(),&info,&child))throw std::runtime_error("Could not start the one-shot installer");
 CloseHandle(child.hThread);bool ready=false;
 for(unsigned n=0;n<80;++n) {if(fs::exists(job/L"waiting.txt")){ready=true;break;}if(WaitForSingleObject(child.hProcess,50)==WAIT_OBJECT_0)break;}
 CloseHandle(child.hProcess);
 if(stop) {write_text(job/L"cancel.txt","cancel");throw std::runtime_error("Installer cancellation requested; download retained");}
 if(!ready)throw std::runtime_error("Installer did not become ready; check update logs");
    {std::lock_guard lock(mutex);state.armed=true;state.status="Queued. Close Skate normally, then wait for the installer completion message.";}
}
void launch(void(*function)()) {
 {std::lock_guard lock(mutex);if(state.busy || state.armed)return;state.busy=true;stop=false;}
 try {std::thread([function] {
  try{function();}catch(const std::exception& e){status(e.what());log(std::string("Updater: ")+e.what());}catch(...){status("Update failed");}
  std::lock_guard lock(mutex);state.busy=false;
 }).detach();}catch(...){std::lock_guard lock(mutex);state.busy=false;state.status="Could not create updater worker";}
}
}
Snapshot snapshot(){std::lock_guard lock(mutex);return state;}
void start(const fs::path& companion,const fs::path& data) {
 try {
  {std::lock_guard lock(mutex);if(started)return;started=true;}
  wchar_t exe[32768]{};if(!GetModuleFileNameW(nullptr,exe,32768))throw std::runtime_error("Game folder unavailable");
  root=fs::path(exe).parent_path();data_path=fs::absolute(data/L"updates").lexically_normal();safe_path(data_path);
  if(fs::weakly_canonical(companion)!=fs::weakly_canonical(root/L"scripts/Custom"))throw std::runtime_error("Automatic updates require the standard scripts/Custom installation layout");
  fs::create_directories(data_path);state.installed=installed_version(root);
  if(fs::exists(data_path/L"preferences.ini")) {const auto b=read_file(data_path/L"preferences.ini",1024);state.automatic=std::string(b.begin(),b.end()).find("automatic_check=0")==std::string::npos;}
  if(fs::exists(data_path/L"pending.txt")) {
   const auto b=read_file(data_path/L"pending.txt",1024);const std::string name(b.begin(),b.end());
   if(name.starts_with("update-") && name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-")==name.npos) {
    const auto pending=data_path/L"jobs"/fs::u8path(name);safe_path(pending);
    if(fs::exists(pending/L"release.json") && fs::exists(pending/L"archive.zip")) {
     const auto json=read_file(pending/L"release.json",1024*1024);auto release=parse_release(std::string(json.begin(),json.end()));
     if(newer(release,state.installed) || fs::exists(pending/L"transaction.json")){unpack(read_file(pending/L"archive.zip"),release);remote=release;metadata.assign(json.begin(),json.end());job_path=pending;state.ready=true;state.latest=release.version;}
    }
   }
  }
  state.status=state.ready?"Verified update retained. Install after closing Skate when ready.":"Not checked";
  usable=true;
  if(state.automatic && !state.ready)check();
 }catch(const std::exception& e){status(e.what());log(std::string("Updater startup: ")+e.what());}
}
void check(){if(usable && !snapshot().ready)launch(check_worker);}
void download(){if(!usable)return;const auto s=snapshot();if(s.available && !s.ready)launch(download_worker);}
void cancel(){
 stop=true;fs::path job;{std::lock_guard lock(mutex);job=job_path;}
 if(snapshot().armed && !job.empty()) {
  try {std::thread([job] {
   try {
    write_text(job/L"cancel.txt","cancel");
    for(unsigned i=0;i<80;++i) {
     if(fs::exists(job/L"cancelled.txt")) {std::lock_guard lock(mutex);state.armed=false;state.status="Installer cancelled. Verified download retained.";return;}
     Sleep(100);
    }
    status("Cancellation requested. Wait for the installer to exit before queuing again.");
   }catch(const std::exception& e){status(e.what());}
  }).detach();}catch(...){status("Could not request installer cancellation");}
 }
}
void later(){std::lock_guard lock(mutex);state.available=false;state.status="Update postponed. Use Check for updates when ready.";}
void automatic(bool value){if(!usable)return;{std::lock_guard lock(mutex);state.automatic=value;}try{save_preferences();}catch(const std::exception& e){status(e.what());}}
void install_after_exit(){if(usable && snapshot().ready)launch(arm_worker);}
void open_folder(){if(!data_path.empty())ShellExecuteW(nullptr,L"open",data_path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}
}
