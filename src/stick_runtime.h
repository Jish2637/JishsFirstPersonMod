#pragma once
#include "mod.h"
#include "comfort_policy.h"
#include "hippy_jump.h"
#include "jump_episode.h"
namespace jishsfirstpersonmod {
bool install_stick_runtime(std::uintptr_t base);
void tick_stick_look(const Config&,const GameplayInput&,const ComfortSnapshot&,bool inhibited);
void stop_stick_look();
void clear_marker_return();
struct MarkerReturnStatus {std::uint64_t generation{};bool pending{},neutral{},teleport{};};
MarkerReturnStatus marker_return_status();
// Observe before producing/consuming aim, including camera-first callback order.
std::uint64_t observe_marker_return(const ComfortSnapshot&,std::uintptr_t previous_entity=0);
bool accept_marker_native_view(const ComfortSnapshot&,const dingosdk::first_person::Matrix&,bool valid);
void refresh_marker_native_view(const ComfortSnapshot&,const dingosdk::first_person::Matrix&,bool valid);
void note_emote_camera(const dingosdk::first_person::Matrix&);
void note_native_camera_heading(const dingosdk::first_person::Matrix& pose);
void note_camera_view(const dingosdk::first_person::Matrix&,dingosdk::first_person::Quat glance);
StickAim stick_camera_aim();
bool stick_captures_input(unsigned slot=~0u,std::uint32_t buttons=~0u);
std::uint32_t stick_capture_mask(unsigned slot=~0u);
StickInputPolicy stick_input_policy(unsigned slot=~0u);
bool stick_runtime_ready();
struct CameraMovementSample {
    int mode{}, custom_mode{};
    bool non_bail{},bail_recovery{},mounting{},jumping{},view_hold{};
    bool recovery_verified{},recovery_controllable{},recovery_override{};
    std::uint32_t recovery_flags{};
    bool getting_up{};
    bool body_flip_head_lock{},body_flip_camera_owned{};
    float body_flip_tilt{},body_flip_rate{};unsigned body_flip_phase{};
    bool position_valid{};dingosdk::first_person::Vec3 position{};
    HippyJumpState hippy;
    JumpEpisodeState jump_episode;
    std::array<std::uintptr_t,4> state{};
};
CameraMovementSample live_camera_movement();
void tick_hippy_jump(const Config&,const ComfortSnapshot&,CameraMovementSample&,double now);
void set_bail_view_suspended(bool suspended);
int live_camera_movement_mode();
void suspend_custom_camera();
bool take_camera_suspension();
bool install_stick_input_filter();
}
