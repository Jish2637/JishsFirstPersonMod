#include "update_core.h"
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){try {
 using namespace jishsfirstpersonmod::updates;
 if(argc!=3)throw std::runtime_error("usage: release_package_audit client.zip version");
 const auto archive=read_file(std::filesystem::absolute(argv[1]));Release release;
 release.version=argv[2];version(release.version);
 release.tag=release.version+"-reskate-2.1.2";release.size=archive.size();release.digest="sha256:"+sha256(archive);
 release.url="https://github.com/Jish2637/JishsFirstPersonMod/releases/download/"+release.tag+"/JishsFirstPersonMod-"+release.tag+".zip";
 const auto files=unpack(archive,release);
 std::cout<<"Updater package validation PASS: "<<release.version<<", "<<files.size()<<" owned files; "<<release.digest<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
