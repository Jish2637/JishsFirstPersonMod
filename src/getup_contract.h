#pragma once
#include "player_state_contract.h"
namespace jishsfirstpersonmod::getup_contract {
// Native ragdoll update derives bit2 from flags20/21, then selects animation
// action6 instead of action8. The latest gameplay trace changes to these flags
// before the recovery spin and before the AnimatedRagdoll child transition.
inline constexpr player_state_contract::FootState ragdoll{0x720,0x65e8df8,0x4832c80},animated{0x8e0,0x65e8e60,0x4830a70};
inline constexpr std::uintptr_t request_site=0x4832e8f;
inline constexpr std::array<unsigned char,44> request_prefix{
    0x45,0x8b,0x81,0xc0,0x9,0x0,0x0,0x41,0xf,0xba,0xe0,0xd,0x72,0x1e,0x41,0x8b,
    0xc8,0x81,0xe1,0x0,0x0,0x30,0x0,0xf7,0xd9,0x1b,0xd2,0x41,0x83,0xe0,0xfb,0x83,
    0xe2,0x4,0x41,0xb,0xd0,0x41,0x89,0x91,0xc0,0x9,0x0,0x0
};
inline constexpr std::uintptr_t animation_site=0x4833228;
inline constexpr std::array<unsigned char,29> animation_prefix{
    0xba,0x8,0x0,0x0,0x0,0x48,0x8b,0x4f,0x38,0xf6,0x80,0xc0,0x9,0x0,0x0,0x4,
    0xb8,0x6,0x0,0x0,0x0,0xf,0x45,0xd0,0xe8,0x9b,0xe,0xf2,0xff
};
template<class Read> bool verified(Read read,std::uintptr_t base) {
    const auto matches=[&](std::uintptr_t site,const auto& expected) {
        auto actual=expected;return read(base+site,actual.data(),actual.size()) && actual==expected;
    };
    std::uintptr_t update{};
    return matches(request_site,request_prefix) && matches(animation_site,animation_prefix) &&
        read(base+ragdoll.vtable+0x18,&update,sizeof(update)) && update==base+ragdoll.update &&
        read(base+animated.vtable+0x18,&update,sizeof(update)) && update==base+animated.update;
}
inline bool requested(std::uintptr_t parent,std::uintptr_t child,std::uintptr_t vtable,std::uintptr_t base,std::uint32_t flags) {
    const auto matches=[&](const auto& state) { return child==parent+state.offset && vtable==base+state.vtable; };
    return parent && (matches(ragdoll) || matches(animated)) && (flags&0x300000)!=0 && (flags&4)!=0;
}
}
