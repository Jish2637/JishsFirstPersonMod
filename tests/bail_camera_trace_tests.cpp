#include "bail_camera_trace.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path data_directory() { return std::filesystem::current_path()/"bail-trace-test-data"; }
bool stock_first_person_requested() { return true; }
bool stock_first_person_active() { return true; }
int stock_camera_spring_enabled() { return 0; }
}
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
std::vector<std::string> fields(const std::string& text) {
    std::vector<std::string> result;std::istringstream input(text);std::string value;
    while(std::getline(input,value,',')) result.push_back(value);
    return result;
}
int main() {
    using namespace jishsfirstpersonmod;using namespace dingosdk::first_person;
    const auto path=data_directory()/"logs/bail-camera-trace-v12.csv";
    std::filesystem::remove(path);
    const Matrix pose{1,0,0,0,0,1,0,0,0,0,1,0,1,2,3,1};
    ComfortSnapshot state;state.core=1; // invalid animation identity must not be dereferenced
    CameraMovementSample movement;movement.mode=2;movement.state[0]=100;
    Config config;config.spring.enabled=true;
    record_bail_camera(pose,pose,state,movement,config,{},0,0);
    movement.bail_recovery=true;
    record_bail_camera(pose,pose,state,movement,config,{},0,0);
    check(!std::filesystem::exists(path),"camera callback performed file I/O");
    flush_bail_camera_trace();
    std::ifstream input(path);std::string line;std::getline(input,line);const auto header=fields(line);
    const auto column=[&](const char* name) { auto it=std::find(header.begin(),header.end(),name);check(it!=header.end(),"CSV column missing");return static_cast<std::size_t>(it-header.begin()); };
    column("reconciling");column("reconcile_remaining_deg");column("manual_takeover");column("facing_eligible");
    column("native_child");column("episode_type");column("episode_reset_reason");
    column("bail_view_phase");column("recovery_verified");column("recovery_controllable");column("recovery_flags");
    column("body_flip_tilt");column("body_flip_rate");column("body_flip_phase");column("body_flip_head_lock");
    column("getup_camera_phase");column("getup_blend_progress");column("head_target_x");
    column("head_target_qw");column("recovery_look_yaw");column("recovery_look_pitch");
    unsigned count=0;std::vector<std::string> row;
    while(std::getline(input,line)) { row=fields(line);check(row.size()==header.size(),"CSV field count mismatch");++count; }
    check(count==2,"pre-bail context not captured");
    check(row[column("phase")]=="1" && row[column("effective_spring")]=="0" && row[column("saved_spring")]=="1",
        "trace lost effective/preferred Spring distinction");
    check(row[column("head_valid")]=="0" && row[column("camera_head_distance_m")]=="-1","missing animation data looked valid");
    check(row[column("custom_position_delta_m")]=="0" && row[column("custom_angle_delta_deg")]=="0","native passthrough was logged as a custom change");
    input.close();
    // Ordinary jumps, mounting, live settings revisions and calibration failures
    // must be captured even when there is no bail or confirmed hippy event.
    state.core=2;state.state_valid=state.animation_valid=true;
    movement={};movement.mode=movement.custom_mode=1;movement.non_bail=true;
    config.stick.enabled=true;config.disable_spring_on_foot=true;
    HeadPose head;head.valid=true;head.root=head.head=pose;
    BailViewInfo view;view.calibrated=view.attached=true;
    auto final_pose=pose;
    const auto capture=[&](std::uint64_t revision) {
        record_bail_camera(pose,final_pose,state,movement,config,{},0,0,view,head,revision);
        flush_bail_camera_trace();
        std::ifstream latest(path);std::getline(latest,line);
        count=0;
        while(std::getline(latest,line)) { row=fields(line);check(row.size()==header.size(),"action CSV row malformed");++count; }
    };
    capture(1);movement.jumping=true;movement.jump_episode={true,JumpStage::trajectory,"ordinary_jump","none"};final_pose[12]+=1;capture(1);
    check(row[column("detected_action")]=="jumping" && row[column("attachment_reason")]=="jumping", "ordinary jump was not traced");
    check(row[column("custom_position_delta_m")]=="1","final attachment position change missing");
    check(row[column("native_child")]=="trajectory" && row[column("episode_type")]=="ordinary_jump","native stage/episode missing");
    config.stick.lock_mount_jump=false;view.attached=false;final_pose=pose;capture(2);
    check(row[column("settings_revision")]=="2" && row[column("settings_changed")]=="1" &&
        row[column("attachment_requested")]=="0" && row[column("attachment_status")]=="disabled", "live switch off or precedence missing in trace");
    movement.jumping=false;movement.mounting=true;config.stick.lock_mount_jump=true;view.attached=true;capture(3);
    check(row[column("detected_action")]=="mounting" && row[column("attachment_status")]=="attached","mounting was not traced");
    movement.hippy.active=true;config.stick.lock_hippy_jump=true;view.calibrated=view.attached=false;capture(4);
    check(row[column("detected_action")]=="hippy_jump" && row[column("attachment_status")]=="uncalibrated","hippy calibration failure missing");
    head.valid=false;capture(4);
    check(row[column("attachment_status")]=="head_invalid","invalid head sample missing");
    head.valid=true;view.calibrated=true;state.suppressed=true;capture(4);
    check(row[column("attachment_status")]=="camera_ineligible","camera eligibility failure missing");
    state.suppressed=false;state.bailing=true;movement.hippy={};movement.mounting=false;
    movement.recovery_verified=movement.recovery_controllable=true;movement.recovery_flags=0x6000;
    view.collision.obstruction_seconds=.1;view.collision.head_distance=.24f;
    view.collision.escape_attempts=2;view.collision.escape_queries=6;
    view.collision.query_valid=true;view.collision.initial_fraction=0;
    view.collision.escape_result="nearby";view.collision.fallback_reason="time_limit";
    view.phase=2;view.target_rotation=from_rotation_vector({.2f,.3f,.4f});view.recovery_yaw=-20;view.recovery_pitch=10;
    capture(4);
    check(row[column("bail_view_phase")]=="controllable_recovery" && row[column("recovery_controllable")]=="1" &&
        row[column("recovery_flags")]=="24576" && row[column("recovery_look_yaw")]=="-20" &&
        row[column("recovery_look_pitch")]=="10","recovery evidence or head-relative look missing");
    check(row[column("collision_obstruction_ms")]=="100" && row[column("collision_escape_queries")]=="6" &&
        row[column("collision_escape_attempts")]=="2" && row[column("collision_fallback_reason")]=="time_limit" &&
        row[column("collision_escape_result")]=="nearby" && row[column("collision_initial_fraction")]=="0" &&
        row[column("collision_query_valid")]=="1" && std::abs(std::stof(row[column("collision_head_distance_m")])-.24f)<.00001f,
        "collision escape evidence missing from trace");
    check(std::abs(std::stof(row[column("head_target_qw")])-view.target_rotation[3])<.00001f,"head target rotation missing");
    state.bailing=false;movement.recovery_verified=movement.recovery_controllable=false;view.phase=0;
    state.suppressed=false;config.look_only=true;capture(5);
    check(row[column("attachment_reason")]=="compatibility" && row[column("attachment_requested")]=="0","compatibility switch missing in trace");
    movement={};movement.mode=movement.custom_mode=2;movement.non_bail=true;capture(5);
    config.look_only=false;config.stick.lock_mount_jump=false;capture(6);
    check(row[column("detected_action")]=="none" && row[column("settings_changed")]=="1" &&
        row[column("settings_revision")]=="6","idle settings change did not open a trace window");
    state.core=1;state.state_valid=state.animation_valid=false;movement={};movement.mode=2;
    config.look_only=false;
    const auto before_queue_rows=count;
    const auto size=std::filesystem::file_size(path);
    for(int i=0;i<600;++i) {
        movement.bail_recovery=false;record_bail_camera(pose,pose,state,movement,config,{},0,0);
        movement.bail_recovery=true;record_bail_camera(pose,pose,state,movement,config,{},0,0);
    }
    check(std::filesystem::file_size(path)==size,"queued diagnostics wrote on camera thread");
    flush_bail_camera_trace();
    input.open(path);std::getline(input,line);count=0;
    while(std::getline(input,line)) { row=fields(line);check(row.size()==header.size(),"buffered CSV row malformed");++count; }
    check(count<=before_queue_rows+1024 && std::stoul(row[column("dropped")])>0,"diagnostic queue was unbounded");
    input.close();
    // Oversized existing traces rotate; recording continues in a fresh CSV.
    std::filesystem::resize_file(path,8*1024*1024+1);
    movement.bail_recovery=false;record_bail_camera(pose,pose,state,movement,config,{},0,0);
    movement.bail_recovery=true;record_bail_camera(pose,pose,state,movement,config,{},0,0);
    check(std::filesystem::file_size(path)==8*1024*1024+1,"rotation ran on camera thread");
    flush_bail_camera_trace();
    check(std::filesystem::exists(std::filesystem::path(path.string()+".1")),"large trace not archived");
    check(std::filesystem::file_size(path)<8*1024*1024,"new trace not bounded");
    input.open(path);std::getline(input,line);check(fields(line)==header,"new CSV lacks correct header");
    check(bool(std::getline(input,line)) && fields(line).size()==header.size(),"new trace lost samples");input.close();
    const auto before=std::filesystem::file_size(path);
    movement.bail_recovery=false;record_bail_camera(pose,pose,state,movement,config,{},0,0);
    movement.bail_recovery=true;record_bail_camera(pose,pose,state,movement,config,{},0,0);
    flush_bail_camera_trace();check(std::filesystem::file_size(path)>before,"capture stopped after rotation");
    std::cout<<"Bail trace pre-context, deferred I/O, queue bounds and rotation and continued capture passed\n";
}
