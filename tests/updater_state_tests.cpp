#define JISH_UPDATER_TEST
#include "../src/release_updates.cpp"
#include <iostream>
#include <atomic>
namespace jishsfirstpersonmod {void log(const std::string&) noexcept {}}
namespace jishsfirstpersonmod::release_updates {namespace {
std::atomic<unsigned> calls{};std::atomic<bool> fail_fetch{},block_asset{},block_metadata{};
Bytes zip_fixture;std::string json_fixture;
Bytes test_fetch(const std::wstring&,std::size_t,bool asset) {
 ++calls;if(fail_fetch)throw std::runtime_error("Simulated offline/network error");
 while((asset?block_asset.load():block_metadata.load())) {if(stop)throw std::runtime_error("Update cancelled");Sleep(5);}
 if(stop)throw std::runtime_error("Update cancelled");
 return asset?zip_fixture:Bytes(json_fixture.begin(),json_fixture.end());
}
}}
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
int main(int argc,char** argv){try {
 using namespace jishsfirstpersonmod;using namespace release_updates;using namespace updates;
 require(argc==3,"runtime and client fixtures required");
 require(sha256(read_file(std::filesystem::absolute(argv[1])))==supported_update_runtime_sha256,"shared downloader/helper runtime identity differs from fixture");
 require(supported_update_loader(bundled_proxy_sha256) && multi_mod_loader(bundled_proxy_sha256),"bundled multi-loader rejected");
 require(supported_update_loader(legacy_proxy_sha256) && !multi_mod_loader(legacy_proxy_sha256),"legacy migration policy changed");
 require(multi_mod_loader("438e671de5910e04e1af3ade92f3e72f14cbf12fb63df23676863794a43a0559"),"user's verified UniversalModLoader v2 rejected");
 require(!supported_update_loader(std::string(64,'0')),"unknown loader accepted");
 for(auto known:published_multi_loader_sha256) require(multi_mod_loader(known),"preserved multi-loader digest lost");
 wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);const auto install_root=std::filesystem::path(exe).parent_path();
 const auto isolated=install_root/(L"updater-state-"+std::to_wstring(GetTickCount64()));
 write_text(install_root/L"Mods/JishsFirstPersonMod/manifest.json","{\"version_number\":\"1.34.0\"}");
 write_atomic(install_root/L"ReSkate.dll",read_file(std::filesystem::absolute(argv[1])));
 zip_fixture=read_file(std::filesystem::absolute(argv[2]));const auto tag="1.35.0-reskate-2.1.2";
 json_fixture=std::string("{\"tag_name\":\"")+tag+"\",\"draft\":false,\"prerelease\":false,\"assets\":[{\"name\":\"JishsFirstPersonMod-"+tag+".zip\",\"browser_download_url\":\"https://github.com/Jish2637/JishsFirstPersonMod/releases/download/"+tag+"/JishsFirstPersonMod-"+tag+".zip\",\"size\":"+std::to_string(zip_fixture.size())+",\"digest\":\"sha256:"+sha256(zip_fixture)+"\"}]}";
 write_text(isolated/L"updates/preferences.ini","automatic_check=0\n");
 start(install_root/L"scripts/Custom",isolated);
 require(!snapshot().automatic && !snapshot().busy && calls==0,"disabled automatic checks made a request");
 auto wait=[] {for(unsigned i=0;i<1000;++i){if(!snapshot().busy)return;Sleep(5);}throw std::runtime_error("Worker did not finish");};
 check();wait();require(snapshot().installed=="1.34.0" && snapshot().available && snapshot().latest=="1.35.0","manifest-lowered update not offered");
 const auto proxy=sha256(read_file(install_root/L"dinput8.dll"));later();require(!snapshot().available,"decline not respected");
 fail_fetch=true;check();wait();require(!snapshot().busy && snapshot().status.find("offline")!=std::string::npos,"offline failure not handled");fail_fetch=false;
 block_metadata=true;check();Sleep(20);cancel();wait();block_metadata=false;require(snapshot().status.find("cancel")!=std::string::npos,"check cancellation failed");
 check();wait();block_asset=true;download();Sleep(20);cancel();wait();block_asset=false;
 require(!snapshot().ready && !std::filesystem::exists(isolated/L"updates/pending.txt"),"cancelled download staged/install changed");
 check();wait();download();wait();require(snapshot().ready && !snapshot().armed,"verified download not staged or installer armed without consent");
 require(sha256(read_file(install_root/L"dinput8.dll"))==proxy && installed_version(install_root)=="1.34.0","download changed installed files");
 const auto before=calls.load();check();wait();require(calls==before,"check replaced the release bound to a pending job");
 {std::lock_guard guard(mutex);started=false;usable=false;state=Snapshot{};}
 start(install_root/L"scripts/Custom",isolated);require(snapshot().ready && calls==before,"restart lost a verified pending update or made unwanted requests");
 automatic(false);require(!snapshot().automatic,"automatic preference update");
 std::cout<<"Disabled/once-per-start checks, manifest versions, decline, offline/check/download cancellation, staging without installation, pending recovery PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
