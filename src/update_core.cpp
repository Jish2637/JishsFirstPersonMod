#include "update_core.h"
#include <Windows.h>
#include <bcrypt.h>
#include <winhttp.h>
#include <miniz.h>
#include <rapidjson/document.h>
#include <set>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <limits>
namespace jishsfirstpersonmod::updates {
namespace fs=std::filesystem;
namespace {
[[noreturn]] void bad(const char* why){throw std::runtime_error(why);}
std::string field(const rapidjson::Value& v,const char* name) {
 if(!v.IsObject() || !v.HasMember(name) || !v[name].IsString())bad("Missing string in release metadata");
 return {v[name].GetString(),v[name].GetStringLength()};
}
const std::set<std::string> files{
 "dinput8.dll","LICENSE.txt","Mods/JishsFirstPersonMod/manifest.json",
 "scripts/Custom/JishsFirstPersonMod.dll","scripts/Custom/jishs_first_person_mod.lua",
 "scripts/Custom/FirstPerson/presets/bundled/recommended.ini",
 "scripts/Custom/FirstPerson/updater/FirstPersonUpdater.exe"};
bool allowed(const std::string& name,bool directory) {
 if(name.empty() || name.size()>240 || name.front()=='/' || name.find('\\')!=name.npos || name.find(':')!=name.npos || name.find('\0')!=name.npos)return false;
 if(directory) {for(const auto& f:files)if(f.starts_with(name))return true;return name=="scripts/Custom/FirstPerson/logs/";}
 return files.contains(name);
}
void flush_write(const fs::path& path,const Bytes& data) {
 HANDLE h=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
 if(h==INVALID_HANDLE_VALUE)bad("Could not create update file (permissions or existing file)");
 bool ok=true;std::size_t at{};
 while(at<data.size()) {DWORD done{};const auto n=static_cast<DWORD>(std::min<std::size_t>(1024*1024,data.size()-at));
  if(!WriteFile(h,data.data()+at,n,&done,nullptr)||!done){ok=false;break;}at+=done;
 }
 ok=ok && FlushFileBuffers(h);CloseHandle(h);
 if(!ok){DeleteFileW(path.c_str());bad("Could not write/flush update file");}
}
void copy_durable(const fs::path& from,const fs::path& to) {write_atomic(to,read_file(from));}
void replace(const fs::path& from,const fs::path& to) {
 safe_path(from);safe_path(to);
 if(!MoveFileExW(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))bad("File replacement failed; a file may be locked or read-only");
}
std::vector<std::string> journal_names(const fs::path& path) {
 const auto bytes=read_file(path,1024*1024);rapidjson::Document d;d.Parse(reinterpret_cast<const char*>(bytes.data()),bytes.size());
 if(!d.IsObject() || !d.HasMember("files") || !d["files"].IsArray())bad("Invalid recovery journal");
 std::vector<std::string> names;
 for(const auto& v:d["files"].GetArray()) {
  if(!v.IsString())bad("Invalid recovery target");std::string name(v.GetString(),v.GetStringLength());
  if(!allowed(name,false) || std::find(names.begin(),names.end(),name)!=names.end())bad("Unsafe recovery target");names.push_back(name);
 }
 return names;
}
}
HttpsRequest https_request(const std::wstring& url) {
 URL_COMPONENTS c{};c.dwStructSize=sizeof(c);c.dwHostNameLength=c.dwUrlPathLength=c.dwExtraInfoLength=c.dwUserNameLength=c.dwPasswordLength=static_cast<DWORD>(-1);
 if(url.size()>16384 || !WinHttpCrackUrl(url.c_str(),0,0,&c) || c.nScheme!=INTERNET_SCHEME_HTTPS || c.nPort!=443 || c.dwUserNameLength || c.dwPasswordLength)bad("Unsafe HTTPS update URL");
 HttpsRequest out{std::wstring(c.lpszHostName,c.dwHostNameLength),std::wstring(c.lpszUrlPath,c.dwUrlPathLength)};
 if(out.host!=L"api.github.com" && out.host!=L"github.com" && out.host!=L"release-assets.githubusercontent.com")bad("Unexpected update redirect host");
 if(c.dwExtraInfoLength)out.target.append(c.lpszExtraInfo,c.dwExtraInfoLength);
 if(out.target.find(L'#')!=out.target.npos)bad("Unexpected URL fragment");return out;
}
std::array<unsigned,3> version(const std::string& s) {
 std::array<unsigned,3> out{};std::size_t pos{};
 for(unsigned i=0;i<3;++i) {
  const auto begin=pos;
  while(pos<s.size() && s[pos]>='0' && s[pos]<='9') {if(out[i]>100000)bad("Version component too large");out[i]=out[i]*10+(s[pos++]-'0');}
  if(pos==begin || (pos-begin>1 && s[begin]=='0'))bad("Invalid semantic version");
  if(i<2){if(pos>=s.size() || s[pos++]!='.')bad("Invalid semantic version");}
 }
 if(pos!=s.size())bad("Only stable three-part versions are supported");return out;
}
std::string installed_version(const fs::path& root) {
 const auto bytes=read_file(root/L"Mods/JishsFirstPersonMod/manifest.json",65536);rapidjson::Document d;
 d.Parse(reinterpret_cast<const char*>(bytes.data()),bytes.size());
 if(d.HasParseError() || !d.IsObject())bad("Installed mod manifest is invalid");
 auto v=field(d,"version_number");version(v);return v;
}
Release parse_release(const std::string& json) {
 if(json.size()>1024*1024)bad("Release response is too large");rapidjson::Document d;d.Parse(json.data(),json.size());
 if(d.HasParseError() || !d.IsObject() || !d.HasMember("draft") || !d["draft"].IsBool() || d["draft"].GetBool() ||
    !d.HasMember("prerelease") || !d["prerelease"].IsBool() || d["prerelease"].GetBool())bad("Not a stable published release");
 Release r;r.tag=field(d,"tag_name");const std::string suffix="-reskate-2.1.2";
 if(!r.tag.ends_with(suffix))bad("Latest release is not for ReSkate 2.1.2");r.version=r.tag.substr(0,r.tag.size()-suffix.size());version(r.version);
 if(!d.HasMember("assets") || !d["assets"].IsArray())bad("Release assets are missing");
 const std::string name="JishsFirstPersonMod-"+r.tag+".zip";unsigned matches{};
 for(const auto& a:d["assets"].GetArray()) {
  if(!a.IsObject() || !a.HasMember("name") || !a["name"].IsString() || field(a,"name")!=name)continue;
  ++matches;r.url=field(a,"browser_download_url");r.digest=field(a,"digest");
  if(!a.HasMember("size") || !a["size"].IsUint64())bad("Release asset size missing");r.size=a["size"].GetUint64();
 }
 if(matches!=1 || !r.size || r.size>archive_limit)bad("One bounded client ZIP is required");
 if(r.url!="https://github.com/Jish2637/JishsFirstPersonMod/releases/download/"+r.tag+"/"+name)bad("Unexpected release asset URL");
 if(r.digest.size()!=71 || !r.digest.starts_with("sha256:") || !std::all_of(r.digest.begin()+7,r.digest.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))bad("Valid GitHub SHA-256 digest required");
 return r;
}
bool newer(const Release& r,const std::string& installed){return version(r.version)>version(installed);}
std::string sha256(const Bytes& bytes) {
 BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};unsigned char digest[32]{};
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)bad("SHA-256 provider unavailable");
 bool ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
 if(ok)ok=BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)>=0 && BCryptFinishHash(hash,digest,32,0)>=0;
 if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);if(!ok)bad("SHA-256 failed");
 const char* hex="0123456789abcdef";std::string text;for(auto b:digest){text+=hex[b>>4];text+=hex[b&15];}return text;
}
void safe_path(const fs::path& path) {
 if(!path.is_absolute())bad("Updater path must be absolute");
 fs::path current;
 for(const auto& part:path.lexically_normal()) {
  current/=part;const DWORD attr=GetFileAttributesW(current.c_str());
  if(attr!=INVALID_FILE_ATTRIBUTES && (attr&FILE_ATTRIBUTE_REPARSE_POINT))bad("Updater refuses symbolic links/reparse points");
 }
}
Bytes read_file(const fs::path& path,std::size_t limit) {
 safe_path(fs::absolute(path));std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)bad("Cannot read update file");
 const auto size=in.tellg();if(size<0 || static_cast<std::uint64_t>(size)>limit)bad("Update file is too large");
 Bytes bytes(static_cast<std::size_t>(size));in.seekg(0);if(!bytes.empty() && !in.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))bad("Incomplete file read");return bytes;
}
void write_atomic(const fs::path& destination,const Bytes& data) {
 const auto path=fs::absolute(destination).lexically_normal();safe_path(path);fs::create_directories(path.parent_path());safe_path(path);
 const auto temp=fs::path(path.wstring()+L".part-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
 flush_write(temp,data);try{replace(temp,path);}catch(...){DeleteFileW(temp.c_str());throw;}
}
void write_text(const fs::path& path,const std::string& text){write_atomic(path,Bytes(text.begin(),text.end()));}
std::map<std::string,Bytes> unpack(const Bytes& bytes,const Release& release) {
 if(bytes.size()!=release.size || bytes.size()>archive_limit || "sha256:"+sha256(bytes)!=release.digest)bad("Downloaded ZIP checksum/size mismatch");
 mz_zip_archive zip{};if(!mz_zip_reader_init_mem(&zip,bytes.data(),bytes.size(),0))bad("Invalid ZIP archive");
 struct Close{mz_zip_archive* z;~Close(){mz_zip_reader_end(z);}} close{&zip};
 const auto count=mz_zip_reader_get_num_files(&zip);if(count>128)bad("Too many ZIP entries");
 std::map<std::string,Bytes> result;std::set<std::string> seen;std::uint64_t total{};
 for(unsigned i=0;i<count;++i) {
  mz_zip_archive_file_stat stat{};if(!mz_zip_reader_file_stat(&zip,i,&stat))bad("Invalid ZIP entry");
  const auto length=mz_zip_reader_get_filename(&zip,i,nullptr,0);if(!length || length>241)bad("ZIP filename too long");
  std::vector<char> buffer(length);mz_zip_reader_get_filename(&zip,i,buffer.data(),length);std::string name(buffer.data(),length-1);
  const bool dir=stat.m_is_directory!=0;
  if(!allowed(name,dir) || !seen.insert(name).second || stat.m_is_encrypted || !stat.m_is_supported ||
     (stat.m_external_attr&FILE_ATTRIBUTE_REPARSE_POINT) || (((stat.m_external_attr>>16)&0170000)==0120000))bad("Unsafe or unsupported ZIP entry");
  if(stat.m_uncomp_size>32*1024*1024 || (total+=stat.m_uncomp_size)>expanded_limit)bad("Expanded ZIP is too large");
  if(dir){if(stat.m_uncomp_size)bad("Invalid directory entry");continue;}
  Bytes output(static_cast<std::size_t>(stat.m_uncomp_size));
  if(!mz_zip_reader_extract_to_mem(&zip,i,output.data(),output.size(),0))bad("ZIP CRC/decompression failed");
  result.emplace(name,std::move(output));
 }
 for(const auto& required:files)if(!required.ends_with("FirstPersonUpdater.exe") && !result.contains(required))bad("Client ZIP is incomplete");
 for(const char* binary:{"dinput8.dll","scripts/Custom/JishsFirstPersonMod.dll"}) {
  const auto& b=result.at(binary);if(b.size()<256 || b[0]!='M' || b[1]!='Z')bad("Invalid Windows binary in ZIP");
  std::uint32_t offset{};std::memcpy(&offset,b.data()+0x3c,4);
  if(offset>b.size()-26 || std::memcmp(b.data()+offset,"PE\0\0",4)!=0 || b[offset+4]!=0x64 || b[offset+5]!=0x86)bad("Client DLL is not Windows x64");
 }
 rapidjson::Document d;const auto& manifest=result.at("Mods/JishsFirstPersonMod/manifest.json");d.Parse(reinterpret_cast<const char*>(manifest.data()),manifest.size());
 if(d.HasParseError() || !d.IsObject() || field(d,"version_number")!=release.version)bad("Package manifest does not match release version");
 return result;
}
void recover(const fs::path& root,const fs::path& job) {
 safe_path(root);safe_path(job);const auto journal=job/L"transaction.json";
 if(!fs::exists(journal))return;
 const auto names=journal_names(journal);
 for(auto it=names.rbegin();it!=names.rend();++it) {
  const auto target=root/fs::u8path(*it),backup=job/L"backup"/fs::u8path(*it);
  if(fs::exists(backup))copy_durable(backup,target);
  else if(fs::exists(job/L"absent"/fs::u8path(*it))) {safe_path(target);if(fs::exists(target) && !DeleteFileW(target.c_str()))bad("Rollback could not remove a newly added file");}
 }
 write_text(job/L"result.txt","Rolled back; original installation restored.\n");
 if(!DeleteFileW(journal.c_str()))bad("Could not finish recovery journal");
}
void install(const fs::path& root,const fs::path& job,const Release& release,const std::string& expected_proxy,std::size_t fail_after) {
 safe_path(root);safe_path(job);
 if(sha256(read_file(root/L"dinput8.dll"))!=expected_proxy)bad("Installed dinput8.dll changed; refusing to overwrite another loader");
 if(fs::exists(job/L"transaction.json"))recover(root,job);
 const auto package=unpack(read_file(job/L"archive.zip"),release);
 std::vector<HANDLE> locks;
 try {
  for(const auto& [name,data]:package) {
   const auto target=root/fs::u8path(name);safe_path(target);
   if(fs::exists(target)) {
    const auto h=CreateFileW(target.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(h==INVALID_HANDLE_VALUE)bad("A destination file is locked/read-only; close Skate before updating");locks.push_back(h);
   }
  }
 }catch(...){for(auto h:locks)CloseHandle(h);throw;}
 for(auto h:locks)CloseHandle(h);
 // All new files and backups exist durably before the first live replacement.
 std::string journal="{\"files\":[";bool first=true;
 for(const auto& [name,data]:package) {
  const auto relative=fs::u8path(name);const auto target=root/relative;
  write_atomic(job/L"new"/relative,data);
  if(fs::exists(target))copy_durable(target,job/L"backup"/relative);
  else write_text(job/L"absent"/relative,"");
  if(!first)journal+=',';first=false;journal+='"'+name+'"';
 }
 journal+="]}";write_text(job/L"transaction.json",journal);
 std::size_t applied{};
 try {
  for(const auto& [name,data]:package) {
   if(applied++==fail_after)bad("Injected transaction failure");
   const auto target=root/fs::u8path(name);safe_path(target);fs::create_directories(target.parent_path());safe_path(target);
   replace(job/L"new"/fs::u8path(name),target);
   if(sha256(read_file(target))!=sha256(data))bad("Installed file verification failed");
  }
  write_text(job/L"result.txt","Installed "+release.version+" successfully. Start ReSkate when ready.\n");
  if(!DeleteFileW((job/L"transaction.json").c_str()))bad("Could not commit update journal");
 }catch(...){recover(root,job);throw;}
}
}
