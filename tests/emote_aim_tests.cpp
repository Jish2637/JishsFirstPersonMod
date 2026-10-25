#include "emote_aim.h"
#include <iostream>
#include <map>
#include <vector>
#include <stdexcept>
using namespace jishsfirstpersonmod::emote_aim;
void require(bool v,const char* why){if(!v)throw std::runtime_error(why);}
int main(){try {
 require(allowed(true,true,true,true,true,true),"eligible emote rejected");
 for(unsigned disabled=0;disabled<6;++disabled){bool flags[6]{true,true,true,true,true,true};flags[disabled]=false;
  require(!allowed(flags[0],flags[1],flags[2],flags[3],flags[4],flags[5]),"missing gate accepted");}
 Frame frame;frame.owner={0x10000,0x20000,0x30000,0x40000};frame.time=1000;frame.thread=7;frame.eligible=true;
 require(frame.fresh(frame.owner,1099,7),"fresh frame rejected");
 require(!frame.fresh(frame.owner,1100,7) && !frame.fresh(frame.owner,999,7),"stale/reversed timestamp accepted");
 require(!frame.fresh(frame.owner,1001,8),"worker thread accepted");
 for(unsigned i=0;i<4;++i){auto o=frame.owner;switch(i){case 0:o.core++;break;case 1:o.entity++;break;case 2:o.context++;break;case 3:o.rig++;break;}
  require(!frame.fresh(o,1001,7),"replacement owner accepted");}
 frame.eligible=false;require(!frame.fresh(frame.owner,1001,7),"cleared frame accepted");
 Matrix native{},camera{};for(unsigned i=0;i<16;++i){native[i]=float(i);camera[i]=float(100+i);}
 const auto result=orientation_only(native,camera);
 for(unsigned i=0;i<16;++i)require(result[i]==((i<12 && i%4!=3)?camera[i]:native[i]),"position/padding damaged or aim not copied");
 std::map<std::uintptr_t,std::vector<unsigned char>> memory;
 const auto put=[&]<class T>(std::uintptr_t at,T value){auto& b=memory[at];b.resize(sizeof(value));std::memcpy(b.data(),&value,sizeof(value));};
 const auto read=[&](std::uintptr_t at,void* out,std::size_t n){auto p=memory.find(at);if(p==memory.end() || p->second.size()!=n)return false;std::memcpy(out,p->second.data(),n);return true;};
 const std::uintptr_t base=0x140000000,entity=0x20000,collection=0x30000,gesture=0x40000,physics=0x50000;
 put(entity+0x70,collection);put(collection,entity);put(collection+8,std::uint8_t{2});
 put(collection+0x20,physics);put(physics,base+0x6170410);put(collection+0x40,gesture);put(gesture,base+0x6171ae0);put(gesture+0x18,collection);
 require(component(read,base,entity)==gesture,"separate gesture component not resolved");
 put(gesture+0x18,entity);require(!component(read,base,entity),"foreign component accepted");put(gesture+0x18,collection);
 put(physics,base+0x6171ae0);put(physics+0x18,collection);require(!component(read,base,entity),"ambiguous components accepted");put(physics,base+0x6170410);
 put(collection+8,std::uint8_t{129});require(!component(read,base,entity),"unbounded collection accepted");put(collection+8,std::uint8_t{2});
 unsigned reads{};auto replace=[&](std::uintptr_t at,void* out,std::size_t n){if(at==entity+0x70 && ++reads==2){std::uintptr_t other=0x60000;std::memcpy(out,&other,n);return true;}return read(at,out,n);};
 require(!component(replace,base,entity),"replaced collection accepted");
 memory.erase(gesture);require(!component(read,base,entity),"unreadable component accepted");
 std::cout<<"Emote aiming scope, freshness, owner lifecycle, orientation-only output and component ownership PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
