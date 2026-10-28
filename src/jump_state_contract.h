#pragma once
#include "jump_episode.h"
namespace jishsfirstpersonmod::jump_state_contract {
inline constexpr player_state_contract::FootState trajectory{0x460,0x65e8d40,0x4833a00},
    simulated_ragdoll{0xaa0,0x65e8ed0,0x48332f0};
inline constexpr std::uintptr_t trajectory_constructor_site=0x48109db,ragdoll_constructor_site=0x4810b4a,
    trajectory_update_site=0x4833a00,ragdoll_update_site=0x48332f0;
inline constexpr std::array<unsigned char,28> trajectory_constructor_prefix{
    0x48,0x8d,0x5,0x5e,0x83,0xdd,0x1,0x66,0x89,0xb7,0xa0,0x5,0x0,0x0,
    0x48,0x8d,0x9f,0xb0,0x5,0x0,0x0,0x48,0x89,0x87,0x60,0x4,0x0,0x0};
inline constexpr std::array<unsigned char,35> ragdoll_constructor_prefix{
    0x48,0x8d,0x9f,0xa0,0xa,0x0,0x0,0x48,0x8b,0xcb,0xc5,0xf8,0x77,0xe8,0xc4,0xf3,
    0xff,0xff,0x66,0x89,0xb3,0x40,0x1,0x0,0x0,0x48,0x8d,0x5,0x66,0x83,0xdd,0x1,0x48,0x89,0x3};
inline constexpr std::array<unsigned char,32> trajectory_update_prefix{
    0x4c,0x8b,0xdc,0x57,0x48,0x81,0xec,0x70,0x2,0x0,0x0,0xc4,0xc1,0x78,0x29,0x73,
    0xd8,0xc4,0xc1,0x78,0x29,0x7b,0xc8,0xc4,0x41,0x78,0x29,0x43,0xb8,0xc4,0x41,0x78};
inline constexpr std::array<unsigned char,32> ragdoll_update_prefix{
    0x48,0x8b,0xc4,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x81,
    0xec,0xe0,0x1,0x0,0x0,0xc5,0xf8,0x29,0x70,0xb8,0xc5,0xf8,0x29,0x78,0xa8,0xc5};
template<class Read> bool verified(Read read,std::uintptr_t base) {
    const auto matches=[&](std::uintptr_t site,const auto& expected) {
        auto bytes=expected;return read(base+site,bytes.data(),bytes.size()) && bytes==expected;
    };
    std::uintptr_t update{};
    return matches(trajectory_constructor_site,trajectory_constructor_prefix) &&
        matches(ragdoll_constructor_site,ragdoll_constructor_prefix) &&
        matches(trajectory_update_site,trajectory_update_prefix) && matches(ragdoll_update_site,ragdoll_update_prefix) &&
        read(base+trajectory.vtable+0x18,&update,sizeof(update)) && update==base+trajectory.update &&
        read(base+simulated_ragdoll.vtable+0x18,&update,sizeof(update)) && update==base+simulated_ragdoll.update;
}
inline JumpStage stage(bool offboard,std::uintptr_t parent,std::uintptr_t active,std::uintptr_t vtable,std::uintptr_t base,bool expanded) {
    if(!offboard) return JumpStage::board;
    const auto matches=[&](const auto& child) { return active==parent+child.offset && vtable==base+child.vtable; };
    if(matches(player_state_contract::normal_foot_states[0]) || matches(player_state_contract::normal_foot_states[1])) return JumpStage::ground;
    if(matches(player_state_contract::normal_foot_states[2])) return JumpStage::falling;
    if(expanded && matches(trajectory)) return JumpStage::trajectory;
    if(expanded && matches(simulated_ragdoll)) return JumpStage::simulated_ragdoll;
    return JumpStage::unknown;
}
}
