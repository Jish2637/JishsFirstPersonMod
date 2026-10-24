#include "bail_camera_trace.h"
#include "stock_bridge.h"
#include "Engine/Core/Platform/memory.h"
#include "Extension/Skater/free_flight.h"

#include <deque>
#include <vector>
#include <mutex>
#include <atomic>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "rotating_log.h"
namespace jishsfirstpersonmod {
namespace {
using namespace dingosdk::first_person;
struct Sample {
    std::uint64_t time{},event{},revision{};
    int phase{},native_spring{};
    ComfortSnapshot state;CameraMovementSample movement;
    Matrix native_pose{},final_pose{},facing{},root{},head{};
    bool lock_hippy_jump{},facing_valid{},head_valid{},saved_spring{},effective_spring{},requested{},active{},
        stick_enabled{},stick_active{},turn_character{},disable_foot{},lock_mount_jump{};
    float pitch{},yaw{};BailViewInfo view;
    bool settings_changed{},attachment_requested{},camera_eligible{};
    const char* action="none";const char* attachment_reason="none";const char* attachment_status="disabled";
};
std::mutex trace_mutex;
std::deque<Sample> pre;
std::vector<Sample> pending;
std::uint64_t event{},start{},until{},next_sample{};
std::uintptr_t owner{};
bool previous_bail{};
bool previous_ready{};
unsigned previous_settings{};
std::uint64_t previous_revision{};
std::string_view previous_action="none",previous_reason="none",previous_status="disabled";
std::string_view previous_episode="none",previous_reset="none";
JumpStage previous_stage=JumpStage::unknown;
int previous_bail_phase{},previous_getup_phase{};
bool previous_control{};
std::string_view previous_collision_result="none",previous_collision_fallback="none";
std::atomic<unsigned> dropped{};
void matrix_fields(std::ostream& out,const Matrix& pose) {
    for(unsigned i=12;i<15;++i) out<<','<<pose[i];
    for(const auto value:orientation(pose)) out<<','<<value;
}
}
void record_bail_camera(const Matrix& native_pose,const Matrix& final_pose,const ComfortSnapshot& state,
    const CameraMovementSample& movement,const Config& config,const StickAim& aim,float pitch,float yaw,const BailViewInfo& view,const HeadPose& head,std::uint64_t settings_revision) noexcept {
    try {
        std::unique_lock lock(trace_mutex,std::try_to_lock);if(!lock.owns_lock()) { ++dropped;return; }
        const auto now=GetTickCount64();
        const bool bail=bail_camera_locked(state,movement) || view.blocked || jump_camera_action(movement);
        if(owner!=state.core) { owner=state.core;pre.clear();previous_bail=false;previous_ready=false;previous_collision_result=previous_collision_fallback="none";start=until=next_sample=0; }
        const unsigned settings=unsigned(config.look_only) | (unsigned(config.stick.enabled)<<1) |
            (unsigned(config.disable_spring_on_foot)<<2) | (unsigned(config.stick.lock_mount_jump)<<3) |
            (unsigned(config.stick.lock_hippy_jump)<<4) | (unsigned(config.spring.enabled)<<5) |
            (unsigned(config.stick.look_during_parkour)<<6);
        const bool settings_changed=previous_ready && (settings!=previous_settings || settings_revision!=previous_revision);
        const auto action=camera_action(movement),reason=head_attachment_reason(config,state,movement);
        const bool attachment_requested=head_position_attached(config,state,movement);
        const bool eligible=camera_overrides_allowed(state,movement) || bail_camera_locked(state,movement);
        const char* status=!attachment_requested ? "disabled":!eligible ? "camera_ineligible":
            !head.valid ? "head_invalid":!view.calibrated ? "uncalibrated":view.attached ? "attached":"not_applied";
        const bool changed=bail!=previous_bail || settings_changed || (previous_ready &&
            (action!=previous_action || reason!=previous_reason || status!=previous_status ||
             movement.jump_episode.stage!=previous_stage || movement.jump_episode.type!=previous_episode ||
             movement.jump_episode.reset_reason!=previous_reset || view.phase!=previous_bail_phase || view.getup_camera_phase!=previous_getup_phase ||
             movement.recovery_controllable!=previous_control || view.collision.escape_result!=previous_collision_result ||
             view.collision.fallback_reason!=previous_collision_fallback));
        if(now<next_sample && !changed) return;
        next_sample=now+50;
        if(changed) {
            ++event;start=now;
            for(auto sample:pre) { sample.event=event;sample.phase=0;if(pending.size()<1024) pending.push_back(sample);else ++dropped; }
            pre.clear();
        }
        if(bail || changed) until=now+2000;
        previous_bail=bail;previous_ready=true;previous_settings=settings;previous_revision=settings_revision;
        previous_action=action;previous_reason=reason;previous_status=status;
        previous_stage=movement.jump_episode.stage;previous_episode=movement.jump_episode.type;previous_reset=movement.jump_episode.reset_reason;
        previous_bail_phase=view.phase;previous_getup_phase=view.getup_camera_phase;previous_control=movement.recovery_controllable;
        previous_collision_result=view.collision.escape_result;previous_collision_fallback=view.collision.fallback_reason;
        Sample sample;sample.time=now;sample.event=event;sample.phase=bail ? 1:2;
        sample.revision=settings_revision;sample.settings_changed=settings_changed;sample.action=action;
        sample.attachment_reason=reason;sample.attachment_requested=attachment_requested;
        sample.camera_eligible=eligible;sample.attachment_status=status;
        sample.state=state;sample.movement=movement;sample.view=view;
        sample.head_valid=head.valid;sample.root=head.root;sample.head=head.head;sample.native_pose=native_pose;sample.final_pose=final_pose;
        sample.saved_spring=config.spring.enabled;sample.requested=stock_first_person_requested();sample.active=stock_first_person_active();
        sample.effective_spring=effective_camera_config(config,sample.requested,state,movement).spring.enabled;
        sample.native_spring=stock_camera_spring_enabled();sample.pitch=pitch;sample.yaw=yaw;
        sample.stick_enabled=config.stick.enabled;sample.stick_active=aim.active;sample.turn_character=config.stick.turn_character;
        sample.lock_hippy_jump=config.stick.lock_hippy_jump;sample.disable_foot=config.disable_spring_on_foot;sample.lock_mount_jump=config.stick.lock_mount_jump;
        if(state.state_valid && state.rig) {
            sample.facing_valid=dingosdk::memory::read(state.rig+0x4720,sample.facing) && dingosdk::valid_flight_transform(sample.facing);
        }
        if(start && now<until && now-start<=15000) {
            if(pending.size()<1024) pending.push_back(sample);else ++dropped;
        } else if(!bail) { pre.push_back(sample);while(pre.size()>20) pre.pop_front(); }
    } catch(...) { ++dropped; }
}
void flush_bail_camera_trace() noexcept {
    try {
        std::vector<Sample> batch;
        { std::lock_guard lock(trace_mutex);batch.swap(pending); }
        if(batch.empty()) return;
        const auto path=data_directory()/"logs/bail-camera-trace-v12.csv";
        std::filesystem::create_directories(path.parent_path());
        const std::string_view header="time_ms,event,phase,mode,custom_mode,physics,active_child,child_vtable,state_valid,animation_valid,wipeout_request,animation_wipeout,recovery,bail_latched,selector_valid,selector_current,selector_next,requested,active,saved_spring,effective_spring,native_spring,disable_foot,lock_mount_jump,mounting,jumping,stick_enabled,stick_active,turn_character,look_pitch,look_yaw,native_x,native_y,native_z,native_qx,native_qy,native_qz,native_qw,final_x,final_y,final_z,final_qx,final_qy,final_qz,final_qw,facing_valid,facing_x,facing_y,facing_z,facing_qx,facing_qy,facing_qz,facing_qw,head_valid,root_x,root_y,root_z,root_qx,root_qy,root_qz,root_qw,head_x,head_y,head_z,head_qx,head_qy,head_qz,head_qw,camera_head_distance_m,custom_position_delta_m,custom_angle_delta_deg,dropped,hold_phase,view_blocked,head_attached,offset_calibrated,offset_x,offset_y,offset_z,lock_hippy_jump,hippy_candidate,hippy_active,hippy_rising_samples,hippy_up_speed,reconciling,reconcile_remaining_deg,manual_takeover,facing_eligible\n";
        std::string extended_header(header);extended_header.pop_back();
        extended_header+=",settings_revision,settings_changed,detected_action,attachment_reason,attachment_requested,camera_eligible,attachment_status,native_child,episode_type,episode_reset_reason,bail_view_phase,recovery_verified,recovery_controllable,recovery_flags,recovery_motion_override,head_target_qx,head_target_qy,head_target_qz,head_target_qw,recovery_look_yaw,recovery_look_pitch,getup_requested,getup_locked,getup_camera_phase,getup_blend_progress,head_target_x,head_target_y,head_target_z,body_flip_tilt,body_flip_rate,body_flip_phase,body_flip_head_lock,collision_obstruction_ms,collision_head_distance_m,collision_escape_attempts,collision_escape_queries,collision_query_valid,collision_initial_fraction,collision_escape_result,collision_fallback_reason\n";
        std::ostringstream out;
        out<<std::setprecision(9);
        for(const auto& s:batch) {
            const auto& o=s.state;const auto& m=s.movement;
            out<<s.time<<','<<s.event<<','<<s.phase<<','<<m.mode<<','<<m.custom_mode<<','<<m.state[0]<<','<<m.state[2]<<','<<m.state[3]
                <<','<<o.state_valid<<','<<o.animation_valid<<','<<o.wipeout_request<<','<<o.animation_wipeout<<','<<o.recovery<<','<<o.bailing
                <<','<<o.selector_valid<<','<<o.selector_current<<','<<o.selector_next<<','<<s.requested<<','<<s.active
                <<','<<s.saved_spring<<','<<s.effective_spring<<','<<s.native_spring<<','<<s.disable_foot<<','<<s.lock_mount_jump
                <<','<<m.mounting<<','<<m.jumping<<','<<s.stick_enabled<<','<<s.stick_active<<','<<s.turn_character<<','<<s.pitch<<','<<s.yaw;
            matrix_fields(out,s.native_pose);matrix_fields(out,s.final_pose);
            out<<','<<s.facing_valid;matrix_fields(out,s.facing);
            out<<','<<s.head_valid;matrix_fields(out,s.root);matrix_fields(out,s.head);
            const Vec3 native{s.native_pose[12],s.native_pose[13],s.native_pose[14]},final{s.final_pose[12],s.final_pose[13],s.final_pose[14]};
            out<<','<<(s.head_valid ? length(subtract(final,{s.head[12],s.head[13],s.head[14]})):-1.f)
                <<','<<length(subtract(final,native))<<','<<length(rotation_vector(multiply(conjugate(orientation(s.native_pose)),orientation(s.final_pose))))*180/std::numbers::pi_v<float>
                <<','<<dropped.load()<<','<<s.view.phase<<','<<s.view.blocked<<','<<s.view.attached<<','<<s.view.calibrated
                <<','<<s.view.offset[0]<<','<<s.view.offset[1]<<','<<s.view.offset[2]<<','<<s.lock_hippy_jump
                <<','<<m.hippy.candidate<<','<<m.hippy.active<<','<<m.hippy.rising_samples<<','<<m.hippy.up_speed
                <<",0,0,0"
                <<','<<character_facing_allowed(o,m)<<','<<s.revision<<','<<s.settings_changed
                <<','<<s.action<<','<<s.attachment_reason<<','<<s.attachment_requested<<','<<s.camera_eligible
                <<','<<s.attachment_status<<','<<jump_stage_name(m.jump_episode.stage)<<','<<m.jump_episode.type
                <<','<<m.jump_episode.reset_reason<<','<<(s.view.phase==1 ? "uncontrolled":s.view.phase==2 ? "controllable_recovery":s.view.phase==3 ? "getting_up":"finished")
                <<','<<m.recovery_verified<<','<<m.recovery_controllable<<','<<m.recovery_flags<<','<<m.recovery_override;
            for(const auto value:s.view.target_rotation) out<<','<<value;
            out<<','<<s.view.recovery_yaw<<','<<s.view.recovery_pitch<<','<<m.getting_up<<','<<s.view.getup_locked
                <<','<<(s.view.getup_camera_phase==1 ? "smoothed_head_follow":s.view.getup_camera_phase==2 ? "blending":s.view.getup_camera_phase==3 ? "attached":"normal")
                <<','<<s.view.getup_blend;
            for(const auto value:s.view.head_target) out<<','<<value;
            out<<','<<m.body_flip_tilt<<','<<m.body_flip_rate<<','<<m.body_flip_phase<<','<<m.body_flip_head_lock;
            const auto& collision=s.view.collision;
            out<<','<<collision.obstruction_seconds*1000<<','<<collision.head_distance<<','<<collision.escape_attempts
                <<','<<collision.escape_queries<<','<<collision.query_valid<<','<<collision.initial_fraction
                <<','<<collision.escape_result<<','<<collision.fallback_reason;
            out<<'\n';
        }
        if(!append_rotating_log(path,out.str(),trace_log_limit,log_archives,extended_header))
            log("Could not write bail-camera-trace-v12.csv; later batches will retry.");
    } catch(...) { log("Bail camera trace flush failed."); }
}
}
