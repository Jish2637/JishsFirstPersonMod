#include "release_updates.h"
#include "External/rapidjson/document.h"
#include <Windows.h>
#include <winhttp.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>
#pragma comment(lib, "winhttp.lib")

namespace jishsfirstpersonmod::release_updates {
namespace {
std::mutex lock;
Snapshot state{"Not checked", "", "", false, false};
std::wstring asset_url;
std::string digest;
constexpr wchar_t endpoint[]=L"https://api.github.com/repos/Jish2637/JishsFirstPersonMod/releases/latest";
constexpr size_t metadata_limit=1024*1024, archive_limit=64*1024*1024;
std::filesystem::path root() {
    wchar_t path[32768]{};
    if(!GetModuleFileNameW(nullptr,path,32768)) return {};
    return std::filesystem::path(path).parent_path();
}
void status(std::string message) {std::lock_guard guard(lock); state.status=std::move(message);}
std::wstring wide(const std::string& s) {
    if(s.empty()) return {};
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);
    if(!n) return {};
    std::wstring w(n,L'\0');
    return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),w.data(),n)==n ? w : std::wstring{};
}
// Only GitHub's two fixed hosts are accepted, including after HTTP redirects.
bool trusted(const std::wstring& url) {
    URL_COMPONENTS c{}; c.dwStructSize=sizeof(c);
    wchar_t host[256]{}; c.lpszHostName=host; c.dwHostNameLength=256;
    if(!WinHttpCrackUrl(url.c_str(),0,0,&c) || c.nScheme!=INTERNET_SCHEME_HTTPS) return false;
    const std::wstring h(host,c.dwHostNameLength);
    return _wcsicmp(h.c_str(),L"github.com")==0 || _wcsicmp(h.c_str(),L"api.github.com")==0 || _wcsicmp(h.c_str(),L"release-assets.githubusercontent.com")==0;
}
// WinHTTP handles TLS and HTTPS redirects; streamed with a strict byte cap.
bool fetch(const std::wstring& url,const size_t limit,std::vector<char>& data) {
    if(!trusted(url)) return false;
    URL_COMPONENTS c{}; c.dwStructSize=sizeof(c);
    wchar_t host[256]{},path[8192]{};
    c.lpszHostName=host;c.dwHostNameLength=256;c.lpszUrlPath=path;c.dwUrlPathLength=8192;
    if(!WinHttpCrackUrl(url.c_str(),0,0,&c) || c.nScheme!=INTERNET_SCHEME_HTTPS) return false;
    HINTERNET session=WinHttpOpen(L"JishsFirstPersonMod/1.35.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
    if(!session) return false;
    WinHttpSetTimeouts(session,10000,10000,10000,30000);
    HINTERNET connection=WinHttpConnect(session,host,c.nPort,0);
    HINTERNET request=connection?WinHttpOpenRequest(connection,L"GET",path,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr;
    bool ok=false;
    if(request) {
        DWORD redirect=WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
        WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&redirect,sizeof(redirect));
        const wchar_t headers[]=L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
        if(WinHttpSendRequest(request,headers,static_cast<DWORD>(-1L),WINHTTP_NO_REQUEST_DATA,0,0,0) && WinHttpReceiveResponse(request,nullptr)) {
            DWORD code=0,sz=sizeof(code);
            if(WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&code,&sz,WINHTTP_NO_HEADER_INDEX) && code==200) {
                ok=true;
                for(;;) {
                    char buf[16384];DWORD got=0;
                    if(!WinHttpReadData(request,buf,sizeof(buf),&got)) {ok=false;break;}
                    if(!got) break;
                    if(data.size()+got>limit) {ok=false;break;}
                    data.insert(data.end(),buf,buf+got);
                }
            }
        }
    }
    if(request) WinHttpCloseHandle(request);
    if(connection) WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    if(!ok) data.clear();
    return ok;
}
std::string field(const rapidjson::Value& object,const char* key) {
    auto i=object.FindMember(key);
    return i!=object.MemberEnd() && i->value.IsString()?i->value.GetString():"";
}
void check_worker() {
    std::vector<char> response;
    if(!fetch(endpoint,metadata_limit,response)) {status("GitHub release check failed");return;}
    rapidjson::Document doc;
    doc.Parse(response.data(),response.size());
    if(!doc.IsObject() || !doc.HasMember("assets") || !doc["assets"].IsArray()) {status("Invalid release metadata");return;}
    const auto tag=field(doc,"tag_name");
    const auto pos=tag.find("-reskate-");
    if(pos==std::string::npos) {status("Release tag missing ReSkate version");return;}
    const auto version=tag.substr(0,pos), build=tag.substr(pos+9);
    // This mod is specifically compiled for ReSkate 2.1.1. An incompatible update must not install.
    if(build!="2.1.1") {status("Latest release targets ReSkate "+build+"; this installation requires 2.1.1");return;}
    if(version=="1.35.0") {status("Up to date (1.35.0)");return;}
    std::string url,checksum;
    for(const auto& asset:doc["assets"].GetArray()) {
        if(!asset.IsObject()) continue;
        const auto name=field(asset,"name");
        if(name=="JishsFirstPersonMod-"+tag+".zip") {url=field(asset,"browser_download_url"); checksum=field(asset,"digest");break;}
    }
    const auto wurl=wide(url);
    if(wurl.empty() || !trusted(wurl) || checksum.size()!=71 || checksum.substr(0,7)!="sha256:") {status("Release ZIP or checksum missing");return;}
    std::lock_guard guard(lock);
    asset_url=wurl; digest=checksum; state.version=version;state.compatible_build=build;state.available=true;
    state.status="Version "+version+" available";
}
void download_worker() {
    std::wstring url;std::string checksum;
    {std::lock_guard guard(lock);url=asset_url;checksum=digest;}
    std::vector<char> archive;
    if(!fetch(url,archive_limit,archive)) {status("Download failed");return;}
    // This build deliberately does not activate an unverified archive: installation requires
    // a checksum-verified, transactional installer and verified DLL shutdown lifecycle.
    status("Download received; installation disabled until safe installer is implemented");
}
void run(void(*function)()) {
    {std::lock_guard guard(lock);if(state.busy)return;state.busy=true;}
    try {std::thread([function] {try {function();} catch(...) {status("Updater failed");} std::lock_guard guard(lock);state.busy=false;}).detach();}
    catch(...) {std::lock_guard guard(lock);state.busy=false;state.status="Could not start update check";}
}
}
Snapshot snapshot() {std::lock_guard guard(lock);return state;}
void check() {run(check_worker);}
void download() {
    {std::lock_guard guard(lock);if(!state.available)return;}
    run(download_worker);
}
}
