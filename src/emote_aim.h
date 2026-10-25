#pragma once
#include "Engine/Game/Skater/first_person_spring.h"
#include <cstdint>
#include <cstring>
namespace jishsfirstpersonmod::emote_aim {
using Matrix=dingosdk::first_person::Matrix;
struct Owner {
    std::uintptr_t core{},entity{},context{},rig{};
    bool operator==(const Owner&) const=default;
};
struct Frame {
    Matrix pose{};Owner owner{};std::uint64_t time{};unsigned thread{};bool eligible{};
    bool fresh(Owner current,std::uint64_t now,unsigned caller) const {
        return eligible && owner.core && owner==current && caller==thread && now>=time && now-time<100;
    }
};
inline bool allowed(bool enabled,bool first_person,bool stick,bool camera_only,bool gameplay,bool emote) {
    return enabled && first_person && stick && camera_only && gameplay && emote;
}
// Preserve the native camera position, affine padding and return value. Only aim changes.
inline Matrix orientation_only(Matrix native,const Matrix& camera) {
    for(unsigned col:{0u,4u,8u}) for(unsigned row=0;row<3;++row) native[col+row]=camera[col+row];
    return native;
}
// Resolve the separate gesture component, never the physics component at entity+628.
template<class Read> std::uintptr_t component(Read read,std::uintptr_t base,std::uintptr_t entity) {
    const auto get=[&]<class T>(std::uintptr_t a,T& value) {
        return a>=0x10000 && a<=0x7fffffffffffULL-sizeof(T) && read(a,&value,sizeof(T));
    };
    if(entity<0x10000 || entity>0x7fffffffffffULL-0x1000) return 0;
    std::uintptr_t collection{},back{};std::uint8_t count{};
    if(!get(entity+0x70,collection) || !get(collection,back) || back!=entity ||
       !get(collection+8,count) || count>128) return 0;
    std::uintptr_t result{};
    for(unsigned i=0;i<count;++i) {
        std::uintptr_t p{},vt{},owner{};
        if(!get(collection+0x20+i*0x20,p)) return 0;
        if(!p) continue;
        if(!get(p,vt)) return 0;
        if(vt!=base+0x6171ae0) continue;
        if(result || !get(p+0x18,owner) || owner!=collection) return 0;
        result=p;
    }
    std::uintptr_t after{};
    if(!get(entity+0x70,after) || after!=collection) return 0;
    return result;
}
}
