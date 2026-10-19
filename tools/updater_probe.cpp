#include "release_updates.h"
#include "update_core.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
namespace jishsfirstpersonmod {void log(const std::string& s) noexcept {std::cout<<s<<std::endl;}}
int main(int argc,char** argv){try {
 if(argc!=2)return 2;
 const auto root=std::filesystem::absolute(argv[1]);
 jishsfirstpersonmod::release_updates::start(root/L"scripts/Custom",root/L"scripts/Custom/FirstPerson"/(L"probe-"+std::to_wstring(GetTickCount64())));
 for(unsigned i=0;i<600;++i){auto s=jishsfirstpersonmod::release_updates::snapshot();if(!s.busy){std::cout<<s.status<<std::endl;if(!s.available)throw std::runtime_error("Update was not offered");break;}Sleep(100);}
 jishsfirstpersonmod::release_updates::download();
 for(unsigned i=0;i<1600;++i){auto s=jishsfirstpersonmod::release_updates::snapshot();if(!s.busy){std::cout<<s.status<<std::endl;if(!s.ready)throw std::runtime_error("Download/staging failed");std::cout<<"LIVE manifest-lowered check/download/hash/ZIP/staging PASS; installer was NOT armed."<<std::endl;return 0;}Sleep(100);}
 throw std::runtime_error("Live probe timeout");
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
