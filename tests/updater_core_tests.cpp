#include "update_core.h"
#include <Windows.h>
#include <miniz.h>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstring>
using namespace jishsfirstpersonmod::updates;namespace fs=std::filesystem;
void require(bool v,const char* text){if(!v)throw std::runtime_error(text);}
template<class F> void rejects(F f,const char* why){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}require(rejected,why);}
Bytes bytes(const std::string& s){return {s.begin(),s.end()};}
Bytes dll(){Bytes b(256);b[0]='M';b[1]='Z';unsigned offset=64;std::memcpy(b.data()+0x3c,&offset,4);std::memcpy(b.data()+64,"PE\0\0",4);b[68]=0x64;b[69]=0x86;return b;}
std::map<std::string,Bytes> fixture() {return {
 {"dinput8.dll",dll()},{"scripts/Custom/JishsFirstPersonMod.dll",dll()},
 {"Mods/JishsFirstPersonMod/manifest.json",bytes("{\"version_number\":\"1.35.0\"}")},
 {"scripts/Custom/jishs_first_person_mod.lua",bytes("-- test fixture")},
 {"scripts/Custom/FirstPerson/presets/bundled/recommended.ini",bytes("[test]\nboard_look=1\n")},
 {"LICENSE.txt",bytes("license fixture")}};}
Bytes zip(const std::map<std::string,Bytes>& contents) {
 mz_zip_archive z{};require(mz_zip_writer_init_heap(&z,0,0)!=0,"ZIP writer");
 std::vector<std::pair<std::string,std::string>> aliases;unsigned number{};
 for(const auto& [name,data]:contents) {
  auto alias=std::to_string(++number);alias.resize(name.size(),'x');aliases.emplace_back(alias,name);
  require(mz_zip_writer_add_mem(&z,alias.c_str(),data.data(),data.size(),MZ_BEST_COMPRESSION)!=0,"ZIP entry");
 }
 void* ptr{};size_t length{};require(mz_zip_writer_finalize_heap_archive(&z,&ptr,&length)!=0,"ZIP finalize");
 Bytes result(static_cast<unsigned char*>(ptr),static_cast<unsigned char*>(ptr)+length);mz_free(ptr);mz_zip_writer_end(&z);
 for(const auto& [alias,name]:aliases) {
  for(size_t at=0;at+alias.size()<=result.size();++at)if(std::memcmp(result.data()+at,alias.data(),alias.size())==0) {std::memcpy(result.data()+at,name.data(),name.size());at+=name.size()-1;}
 }
 return result;
}
Release release(const Bytes& b) {Release r;r.version="1.35.0";r.tag=r.version+"-reskate-2.1.2";r.size=b.size();r.digest="sha256:"+sha256(b);r.url="https://github.com/Jish2637/JishsFirstPersonMod/releases/download/"+r.tag+"/JishsFirstPersonMod-"+r.tag+".zip";return r;}
std::string metadata(const Release& r) {
 return "{\"tag_name\":\""+r.tag+"\",\"draft\":false,\"prerelease\":false,\"assets\":[{\"name\":\"JishsFirstPersonMod-"+r.tag+".zip\",\"browser_download_url\":\""+r.url+"\",\"digest\":\""+r.digest+"\",\"size\":"+std::to_string(r.size)+"}]}";
}
int main(int argc,char** argv){try {
 const auto archive=zip(fixture());auto remote=release(archive);const auto json=metadata(remote);
 if(argc>=2) {const auto b=read_file(fs::absolute(argv[1]));auto r=release(b);if(argc==3)r.version=argv[2];unpack(b,r);std::cout<<"Actual published client package verified\n";return 0;}
 require(parse_release(json).version=="1.35.0","metadata parse");
 require(newer(remote,"1.34.0") && !newer(remote,"1.35.0") && !newer(remote,"1.36.0"),"new/equal/older releases");
 require(version("1.10.0")>version("1.9.9"),"numeric semantic order");
 for(const char* s:{"","1.35","1.35.0-test","01.35.0","v1.35.0","1.-1.0","1.9999999.0"})rejects([&]{version(s);},"invalid version accepted");
 auto change=[&](std::string a,std::string b){auto text=json;const auto at=text.find(a);require(at!=text.npos,"fixture replacement");text.replace(at,a.size(),b);return text;};
 for(const auto& text:{std::string("[]"),change("\"draft\":false","\"draft\":true"),change("\"prerelease\":false","\"prerelease\":true"),change("2.1.2","2.1.0"),change("2.1.2","2.1.1"),change("https://github.com/","https://evil.example/"),change("sha256:","sha512:"),change(".zip\"","-source.zip\"")})rejects([&]{parse_release(text);},"unsafe metadata accepted");
 const auto signed_url=https_request(L"https://release-assets.githubusercontent.com/asset.zip?sig=abc%2B123&expires=10");
 require(signed_url.target==L"/asset.zip?sig=abc%2B123&expires=10","signed redirect query lost");
 for(const wchar_t* url:{L"http://github.com/a",L"https://github.com.evil.example/a",L"https://user:password@github.com/a",L"https://github.com:444/a",L"https://evil.example/a",L"https://github.com/a#fragment"})rejects([&]{https_request(url);},"unsafe redirect accepted");
 require(unpack(archive,remote).size()==6,"valid package rejected");
 auto wrong=remote;wrong.digest="sha256:"+std::string(64,'0');rejects([&]{unpack(archive,wrong);},"wrong digest accepted");
 for(const char* name:{"../evil.dll","scripts/Custom/../evil.dll","C:/evil.dll","/evil.dll","scripts\\Custom\\evil.dll","scripts/Custom/FirstPerson/settings.ini","Mods/Other/manifest.json","scripts/Custom/FirstPerson/presets/Main.ini"}) {
  auto f=fixture();f[name]=bytes("bad");const auto b=zip(f);rejects([&]{unpack(b,release(b));},"unsafe ZIP path accepted");
 }
 auto incomplete=fixture();incomplete.erase("LICENSE.txt");auto b=zip(incomplete);rejects([&]{unpack(b,release(b));},"incomplete ZIP accepted");
 auto mismatch=fixture();mismatch["Mods/JishsFirstPersonMod/manifest.json"]=bytes("{\"version_number\":\"9.0.0\"}");b=zip(mismatch);rejects([&]{unpack(b,release(b));},"manifest version mismatch accepted");
 auto bomb=archive;
 for(size_t i=0;i+46<bomb.size();++i)if(std::memcmp(bomb.data()+i,"PK\1\2",4)==0){const unsigned size=200*1024*1024;std::memcpy(bomb.data()+i+24,&size,4);break;}
 rejects([&]{unpack(bomb,release(bomb));},"oversized ZIP entry accepted");
 auto symlink=archive;
 for(size_t i=0;i+46<symlink.size();++i)if(std::memcmp(symlink.data()+i,"PK\1\2",4)==0){const unsigned attr=0120777u<<16;std::memcpy(symlink.data()+i+38,&attr,4);break;}
 rejects([&]{unpack(symlink,release(symlink));},"ZIP symlink accepted");
 auto crc=archive;for(size_t i=0;i+30<crc.size();++i)if(std::memcmp(crc.data()+i,"PK\3\4",4)==0){unsigned short name{},extra{};std::memcpy(&name,crc.data()+i+26,2);std::memcpy(&extra,crc.data()+i+28,2);crc[i+30+name+extra]^=127;break;}
 rejects([&]{unpack(crc,release(crc));},"damaged ZIP contents accepted");
 const auto workspace=fs::absolute("updater-core-fixtures-"+std::to_string(GetTickCount64()));fs::create_directories(workspace);
 auto setup=[&](const std::string& id){const auto root=workspace/id;for(const auto& [name,data]:fixture())write_text(root/fs::u8path(name),"original "+name);
  write_text(root/L"Mods/mods.json","my launcher settings");write_text(root/L"scripts/Custom/FirstPerson/settings.ini","my settings");write_text(root/L"scripts/Custom/FirstPerson/presets/Main.ini","my preset");
  write_atomic(root/L"job/archive.zip",archive);return root;};
 auto original=[&](const fs::path& root){for(const auto& [name,data]:fixture())require(read_file(root/fs::u8path(name))==bytes("original "+name),"original file not restored");};
 auto root=setup("success");auto hash=sha256(read_file(root/L"dinput8.dll"));install(root,root/L"job",remote,hash);
 for(const auto& [name,data]:fixture())require(read_file(root/fs::u8path(name))==data,"installed content mismatch");
 require(read_file(root/L"scripts/Custom/FirstPerson/settings.ini")==bytes("my settings") && read_file(root/L"scripts/Custom/FirstPerson/presets/Main.ini")==bytes("my preset"),"user data overwritten");
 require(read_file(root/L"Mods/mods.json")==bytes("my launcher settings"),"launcher enablement overwritten");
 require(!fs::exists(root/L"job/transaction.json") && fs::exists(root/L"job/backup/dinput8.dll"),"commit/backup state");
 root=setup("multi-loader");hash=sha256(read_file(root/L"dinput8.dll"));
 write_text(root/L"scripts/Custom/dinput8_mods.txt","# load order\nOther.dll\n# Disabled.dll\nJishsFirstPersonMod.dll\n");
 write_text(root/L"scripts/Custom/Other.dll","other mod");write_text(root/L"dinput8_chain.dll","chained proxy");
 install(root,root/L"job",remote,hash,static_cast<std::size_t>(-1),true);
 require(sha256(read_file(root/L"dinput8.dll"))==hash,"multi-mod loader replaced by incoming proxy");
 require(read_file(root/L"scripts/Custom/JishsFirstPersonMod.dll")==fixture().at("scripts/Custom/JishsFirstPersonMod.dll"),"First Person did not update with preserved loader");
 require(read_file(root/L"scripts/Custom/dinput8_mods.txt")==bytes("# load order\nOther.dll\n# Disabled.dll\nJishsFirstPersonMod.dll\n") &&
     read_file(root/L"scripts/Custom/Other.dll")==bytes("other mod") && read_file(root/L"dinput8_chain.dll")==bytes("chained proxy"),"multi-mod configuration/other mod changed");
 require(!fs::exists(root/L"job/backup/dinput8.dll"),"preserved loader entered transaction");
 root=setup("multi-rollback");hash=sha256(read_file(root/L"dinput8.dll"));
 rejects([&]{install(root,root/L"job",remote,hash,2,true);},"preserved-loader rollback failure not injected");original(root);
 rejects([&]{install(root,root/L"job",remote,std::string(64,'0'),static_cast<std::size_t>(-1),true);},"preserve mode bypassed loader ownership check");
 root=setup("rollback");hash=sha256(read_file(root/L"dinput8.dll"));rejects([&]{install(root,root/L"job",remote,hash,3);},"injected failure not raised");original(root);
 root=setup("locked");hash=sha256(read_file(root/L"dinput8.dll"));const auto h=CreateFileW((root/L"scripts/Custom/JishsFirstPersonMod.dll").c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr);require(h!=INVALID_HANDLE_VALUE,"lock fixture");
 rejects([&]{install(root,root/L"job",remote,hash);},"locked DLL accepted");CloseHandle(h);original(root);
 rejects([&]{install(root,root/L"job",remote,std::string(64,'0'));},"unknown proxy accepted");original(root);
 root=setup("readonly");hash=sha256(read_file(root/L"dinput8.dll"));SetFileAttributesW((root/L"LICENSE.txt").c_str(),FILE_ATTRIBUTE_READONLY);
 rejects([&]{install(root,root/L"job",remote,hash);},"read-only target accepted");SetFileAttributesW((root/L"LICENSE.txt").c_str(),FILE_ATTRIBUTE_NORMAL);original(root);
 root=setup("recover");fs::copy_file(root/L"dinput8.dll",root/L"job/backup.dll");write_atomic(root/L"job/backup/dinput8.dll",read_file(root/L"dinput8.dll"));write_text(root/L"job/transaction.json","{\"files\":[\"dinput8.dll\"]}");write_text(root/L"dinput8.dll","interrupted replacement");recover(root,root/L"job");original(root);
 write_text(root/L"job/transaction.json","{\"files\":[\"../escape\"]}");rejects([&]{recover(root,root/L"job");},"malicious journal accepted");
 std::cout<<"Version/metadata/HTTPS/ZIP/checksum, staging, locked/read-only/unknown-loader refusal, rollback/recovery and settings preservation PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
