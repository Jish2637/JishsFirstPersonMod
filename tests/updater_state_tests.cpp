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
 start(std::filesystem::temp_directory_path()/"studio-no-install",std::filesystem::temp_directory_path()/"studio-no-state");
 require(!snapshot().automatic && !snapshot().busy && !snapshot().ready && !snapshot().armed && calls==0,"Studio updater activated");
 require(snapshot().status.find("manually")!=std::string::npos,"manual update status missing");
 check();download();automatic(true);install_after_exit();
 require(calls==0 && !snapshot().automatic && !snapshot().ready && !snapshot().armed,"official update bypassed Studio profile policy");
 std::cout<<"Studio profile blocks automatic and manual official update requests and installer arming PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
