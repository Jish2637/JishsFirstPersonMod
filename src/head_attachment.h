#pragma once
#include "comfort_policy.h"
#include "Engine/Game/Skater/first_person_spring.h"
namespace jishsfirstpersonmod {
struct HeadPose { bool valid{};dingosdk::first_person::Matrix root{},head{}; };
// The animation Head joint uses +X up, +Y face-forward and -Z right.
// Convert those physical axes directly; native Spring/look must not calibrate
// a permanent facing error into an uncontrolled bail.
inline dingosdk::first_person::Quat head_camera_rotation(const HeadPose& head) {
    using namespace dingosdk::first_person;
    const auto q=orientation(head.head);
    const auto right=rotate(q,{0,0,-1}),up=rotate(q,{1,0,0}),back=rotate(q,{0,-1,0});
    Matrix camera{};camera[15]=1;
    for(unsigned i=0;i<3;++i) {camera[i]=right[i];camera[4+i]=up[i];camera[8+i]=back[i];}
    return orientation(camera);
}
HeadPose read_animation_head(const ComfortSnapshot& state) noexcept;
HeadPose read_animation_head_component(std::uintptr_t component,std::uintptr_t base) noexcept;
}
