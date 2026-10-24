#include "stick_runtime.h"
#include "spectator_runtime.h"
#include "native_head_look.h"
#include "marker_reset.h"
#include "menu.h"
#include "stock_bridge.h"
#include "native.h"
#include "game_camera_ownership.h"
#include "player_state_contract.h"
#include "camera_movement.h"
#include "jump_state_contract.h"
#include "bail_recovery_contract.h"
#include "getup_contract.h"
#include "settings.h"
#include "camera_policy.h"
#include "head_attachment.h"
#include "body_flip_runtime.h"
#include "Extension/Skater/free_flight.h"
#include "owned_hooks.h"
#include "bridge_resolver.h"
#include <chrono>
#include <mutex>
#include <atomic>
namespace jishsfirstpersonmod {
namespace {
using Matrix=dingosdk::first_person::Matrix;
using Motion=void(*)(std::uintptr_t,std::uintptr_t,const Matrix*,std::uint8_t);
Motion original_motion{};
std::uintptr_t game_base{};
std::atomic<bool> ready{};
std::atomic<bool> camera_suspended{},bail_view_suspended{};
std::mutex hippy_mutex;
HippyJumpDetector hippy_detector;
std::array<std::uintptr_t,4> hippy_owner{};
std::mutex jump_mutex;
JumpEpisodeTracker jump_tracker;
std::atomic<bool> jump_contract{};
std::atomic<bool> recovery_contract{};
std::atomic<bool> getup_verified{};
std::mutex mutex;
StickLookState look;
BoardLookState board_look;
bool recenter_handoff{};
MarkerReset marker_reset;
std::uintptr_t marker_scene{};double marker_return_started{};
std::atomic<std::uint64_t> marker_generation{};
struct Request { ComfortSnapshot owner;StickAim aim;bool turn{};unsigned device{};double time{};float body_heading{};bool head_valid{},recovery{},board{};StickAxes native_axes{};std::uint64_t generation{}; } request;
float native_heading{},displayed_pitch{};double heading_time{};
double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
bool live_foot(const Request& value) {
    if(value.generation!=marker_generation.load() || game_camera_suspended() || panel_gameplay_blocked()) return false;
    if(!compatibility::enabled()) return false;
    if(!(value.board ? value.aim.active:value.aim.capture) || !value.head_valid || now()-value.time>.1 || !stock_first_person_active() || !stock_first_person_requested()) return false;
    bool visible{};if(!overlay_available(visible) || visible) return false;
    const auto foreground=GetForegroundWindow();DWORD pid{};
    if(!foreground || !GetWindowThreadProcessId(foreground,&pid) || pid!=GetCurrentProcessId()) return false;
    const auto current=comfort_snapshot();const auto movement=live_camera_movement();
    return current.core==value.owner.core && current.entity==value.owner.entity && current.context==value.owner.context &&
        current.rig==value.owner.rig && movement.mode==(value.board ? 2:1) &&
        (!value.board || board_free_look_allowed(menu_snapshot().config,current,movement)) &&
        (!value.recovery || !movement.view_hold) &&
        value.recovery==(!menu_snapshot().config.look_only && bail_camera_locked(current,movement)) &&
        camera_look_allowed(menu_snapshot().config,current,movement);
}
Request snapshot() { std::lock_guard lock(mutex);return request; }
void motion_hook(std::uintptr_t rig,std::uintptr_t context,const Matrix* supplied,std::uint8_t flags) {
    const auto error=GetLastError();observe_marker_return(comfort_snapshot());const auto value=snapshot();
    if(menu_snapshot().config.look_only) { original_motion(rig,context,supplied,flags);return; }
    alignas(16) Matrix matrix{};
    if(value.turn && value.aim.active && character_facing_allowed(comfort_snapshot(),live_camera_movement()) && rig==value.owner.rig && context==value.owner.context && live_foot(value) && supplied &&
       dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(supplied),matrix) && dingosdk::valid_flight_transform(matrix)) {
        if(value.generation!=marker_generation.load()) { SetLastError(error);original_motion(rig,context,supplied,flags);return; }
        matrix=face_heading(matrix,value.aim.body_heading);
        SetLastError(error);original_motion(rig,context,&matrix,flags);return;
    }
    SetLastError(error);original_motion(rig,context,supplied,flags);
}
}
bool stick_runtime_ready() { return ready.load(); }
CameraMovementSample live_camera_movement() {
    using namespace dingosdk;
    using namespace dingosdk::client_source::detail;
    namespace pc=player_state_contract;
    // Serialize the observation and history together: sampler, controller and
    // transform callbacks may all inspect the same native state in one frame.
    std::lock_guard jump_lock(jump_mutex);
    const auto unavailable=[&]() {
        CameraMovementSample result;result.jump_episode=jump_tracker.update({},now());return result;
    };
    const auto o=comfort_snapshot();
    if(!o.state_valid || !o.core || !game_base) return unavailable();
    try {
        SourceReader r;
        const auto world=r.value<std::uint32_t>(o.client,0xc4);
        source_require(world==13 || world==21,"Outside gameplay");
        source_require(r.pointer(o.core)==game_base+pc::core_vtable && r.pointer(o.core,0x438)==o.rig &&
            r.pointer(o.core,0x3c0)==o.context && r.pointer(o.rig)==o.context && r.pointer(o.rig,0x4630)==o.core &&
            r.pointer(r.pointer(o.entity,0x628),0x70)==o.core && r.value<std::uint8_t>(o.entity,0x7e0)==0,"Camera owner changed");
        const auto parent=r.pointer(o.core,pc::movement_parent_offset);
        const bool offboard=r.pointer(parent)==game_base+pc::offboard_vtable;
        std::uintptr_t active{},vtable{};
        if(offboard) {
            source_require(r.pointer(parent,8)==o.context && r.pointer(parent,0x18)==o.rig &&
                r.pointer(parent,0x10)==r.pointer(o.core,0x430),"Offboard camera owner changed");
            active=r.pointer(parent,pc::active_substate_offset);vtable=r.pointer(active);
            MEMORY_BASIC_INFORMATION page{};
            source_require(VirtualQuery(reinterpret_cast<void*>(vtable),&page,sizeof(page)) &&
                reinterpret_cast<std::uintptr_t>(page.AllocationBase)==game_base,"Offboard child type is not owned by Skate");
        }
        const auto physics=r.value<std::uint32_t>(o.context,pc::physics_state_offset);
        bool non_bail{},bail_recovery=physics==pc::wipeout,positive_wipeout=physics==pc::wipeout;
        if(o.animation_valid) {
            const auto output=r.pointer(r.pointer(o.core,pc::animation_owner),pc::animation_output);
            const auto wipeout_request=r.value<std::uint8_t>(output,pc::wipeout_request_offset);
            const auto bail=r.value<std::uint8_t>(output,pc::animation_wipeout_offset);
            const auto recovery=r.value<std::uint8_t>(output,pc::recovery_offset);
            source_require(wipeout_request<=1 && bail<=1 && recovery<=1,"Animation flags invalid");
            bail_recovery=bail_recovery || wipeout_request || bail || recovery;
            positive_wipeout=positive_wipeout || wipeout_request || bail;
            non_bail=wipeout_request==0 && bail==0 && recovery==0 && physics!=pc::wipeout;
        }
        const bool mounting=(r.value<std::uint32_t>(o.context,0x13d4)&0x80)!=0;
        const auto stage=jump_state_contract::stage(offboard,parent,active,vtable,game_base,jump_contract.load());
        const auto ground=pc::normal_foot_states[0];
        const bool ground_child=offboard && active==parent+ground.offset && vtable==game_base+ground.vtable;
        const bool verified_control=recovery_contract.load() && o.animation_valid;
        const auto control_flags=verified_control && offboard ? r.value<std::uint32_t>(o.context,bail_recovery_contract::control_flags_offset):0u;
        const auto motion_override=verified_control && ground_child ? r.value<std::uint8_t>(active,bail_recovery_contract::ground_override_offset):0u;
        source_require(motion_override<=1,"Ground motion override invalid");
        Matrix body{};
        const bool body_valid=memory::read(o.rig+0x4720,body) && valid_flight_transform(body);
        r.verify();
        CameraMovementSample result;
        result.mode=offboard ? 1:2;result.custom_mode=camera_movement_mode(true,offboard,parent,active,vtable,game_base,physics);
        result.position_valid=body_valid;result.position={body[12],body[13],body[14]};
        result.non_bail=non_bail;result.bail_recovery=bail_recovery;result.mounting=mounting;
        result.recovery_verified=verified_control;result.recovery_flags=control_flags;result.recovery_override=motion_override!=0;
        result.recovery_controllable=verified_control && bail_recovery_contract::controllable(ground_child,control_flags,
            motion_override!=0,mounting,positive_wipeout);
        result.getting_up=offboard && getup_verified.load() && verified_control &&
            getup_contract::requested(parent,active,vtable,game_base,control_flags);
        result.jumping=stage==JumpStage::falling;
        result.view_hold=bail_view_suspended.load();result.state={physics,parent,active,vtable};
        { std::lock_guard lock(hippy_mutex);
          if(hippy_owner==std::array<std::uintptr_t,4>{o.core,o.entity,o.context,o.rig}) result.hippy=hippy_detector.state(); }
        // Do not expose stale controller confirmation after a live landing/remount.
        // The detector itself is still advanced only by the controller tick.
        if(stage==JumpStage::ground || stage==JumpStage::board || stage==JumpStage::unknown || bail_camera_locked(o,result)) result.hippy={};
        JumpEpisodeEvidence evidence;
        evidence.owner={o.core,o.entity,o.context,o.rig};evidence.valid=non_bail || bail_recovery;
        evidence.bail=bail_recovery || bail_camera_locked(o,result);evidence.stage=stage;
        evidence.position_valid=body_valid;evidence.position={body[12],body[13],body[14]};evidence.hippy=result.hippy;
        result.jump_episode=jump_tracker.update(evidence,now());
        const auto flip=body_flip_status();
        if(flip.owner==std::array<std::uintptr_t,4>{o.core,o.entity,o.context,o.rig}) {
            result.body_flip_head_lock=flip.owns_head();result.body_flip_camera_owned=flip.owns_camera();
            result.body_flip_tilt=flip.tilt;result.body_flip_rate=flip.rate;result.body_flip_phase=static_cast<unsigned>(flip.phase);
        }
        return result;
    } catch(...) { return unavailable(); }
}
void set_bail_view_suspended(bool suspended) { bail_view_suspended=suspended; }
void tick_hippy_jump(const Config& config,const ComfortSnapshot& owner,CameraMovementSample& movement,double time) {
    HippyJumpEvidence evidence;
    evidence.owner=owner.core;evidence.physics=static_cast<std::uint32_t>(movement.state[0]);
    evidence.bail=bail_camera_locked(owner,movement);evidence.falling=movement.jump_episode.stage==JumpStage::falling;
    evidence.on_board=movement.mode==2;
    evidence.grounded=movement.jump_episode.stage==JumpStage::ground;
    if(config.stick.enabled && stock_first_person_requested() && owner.state_valid && movement.mode && movement.non_bail &&
        movement.jump_episode.stage!=JumpStage::unknown) {
        try {
            dingosdk::overlay::DebugModel model;
            const auto pose=dingosdk::client_source::detail::debug_skater(game_base,owner.client,model);
            evidence.valid=model.skater_position_valid && model.skater_identity==owner.entity;
            evidence.position={pose[12],pose[13],pose[14]};
        } catch(...) {}
    }
    std::lock_guard lock(hippy_mutex);
    const std::array<std::uintptr_t,4> identity{owner.core,owner.entity,owner.context,owner.rig};
    if(identity!=hippy_owner) { hippy_detector.reset();hippy_owner=identity; }
    movement.hippy=hippy_detector.update(evidence,time);
}
int live_camera_movement_mode() { return live_camera_movement().custom_mode; }
std::uint64_t observe_marker_return(const ComfortSnapshot& state,std::uintptr_t previous_entity) {
    using namespace dingosdk;
    std::lock_guard lock(mutex);
    if(game_camera_suspended() || (state.initialized ? !state.master:!stock_first_person_requested())) {
        if(marker_reset.owner[0] || marker_reset.pending || marker_reset.neutral) {
            marker_reset.clear();marker_generation=marker_reset.generation;heading_time=0;
            look.reset();board_look.reset();request={};recenter_handoff=false;
        }
        return marker_reset.generation;
    }
    auto owner=marker_reset.owner;
    if(state.state_valid) owner={state.core,state.entity,state.context,state.rig};
    const auto entity=state.state_valid ? state.entity:previous_entity ? previous_entity:owner[1];
    if(!entity || (owner[1] && entity!=owner[1])) return marker_reset.generation;
    const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    std::uintptr_t vtable{},component{},core{},physics{},rig{},rig_context{},scene{};std::uint8_t flag{};
    if(!memory::read(entity,vtable) || vtable!=base+addr::engine::skater_entity_vtable ||
       !memory::read(entity+0x20,scene) || !scene || !memory::read(entity+0x628,component) || !memory::read(component+0x70,core) || core!=owner[0] ||
       !memory::read(core+0x3c0,physics) || physics!=owner[2] ||
       !memory::read(core+0x438,rig) || rig!=owner[3] || !memory::read(rig,rig_context) || rig_context!=physics || !memory::read(entity+0x7e0,flag)) return marker_reset.generation;
    Matrix body{};const bool positioned=memory::read(owner[3]+0x4720,body) && valid_flight_transform(body);
    const auto before=marker_reset.generation;
    const bool continuation=marker_reset.pending && marker_scene==scene && now()-marker_return_started<=5;
    if(continuation && owner!=marker_reset.owner) log("Return to marker: local rig replaced; pending native reset retained within this world.");
    const bool began=marker_reset.observe(owner,flag!=0,positioned,{body[12],body[13],body[14]},continuation);
    marker_scene=scene;
    if(began) marker_return_started=now();
    if(before!=marker_reset.generation) {
        marker_generation=marker_reset.generation;look.reset();board_look.reset();request={};heading_time=0;recenter_handoff=false;
    }
    if(began) log("Return to marker: discarded pre-teleport look; waiting for fresh native view.");
    return marker_reset.generation;
}
bool update_marker_native_view(const ComfortSnapshot& state,const Matrix& pose,bool valid,bool allow_seed) {
    using namespace dingosdk::first_person;
    std::lock_guard lock(mutex);
    const MarkerReset::Owner owner{state.core,state.entity,state.context,state.rig};
    const bool accepted=allow_seed && marker_reset.accept_native(owner,valid);
    // While neutral is pending, follow fresh stock facing instead of freezing
    // the first post-teleport sample if the player moves before centring.
    if(!accepted && !(valid && marker_reset.neutral && !marker_reset.teleport && owner==marker_reset.owner)) return false;
    const auto back=rotate(orientation(pose),{0,0,1});
    native_heading=std::atan2(back[0],back[2])*180/std::numbers::pi_v<float>;
    displayed_pitch=std::asin(std::clamp(-back[1],-1.f,1.f))*180/std::numbers::pi_v<float>;
    heading_time=now();recenter_handoff=false;
    if(accepted) log("Return to marker: fresh native heading accepted; waiting for neutral stick.");
    return accepted;
}
MarkerReturnStatus marker_return_status() {
    std::lock_guard lock(mutex);return {marker_reset.generation,marker_reset.pending || marker_reset.teleport,marker_reset.neutral,marker_reset.teleport};
}
bool accept_marker_native_view(const ComfortSnapshot& state,const Matrix& pose,bool valid) {
    return update_marker_native_view(state,pose,valid,true);
}
void refresh_marker_native_view(const ComfortSnapshot& state,const Matrix& pose,bool valid) {
    update_marker_native_view(state,pose,valid,false);
}
void clear_marker_return() {
    std::lock_guard lock(mutex);marker_scene=0;marker_return_started=0;marker_reset.clear();marker_generation=marker_reset.generation;
    look.reset();board_look.reset();request={};heading_time=0;recenter_handoff=false;
}
void stop_stick_look() { std::lock_guard lock(mutex);look.reset();board_look.reset();request={}; }
void suspend_custom_camera() { stop_stick_look();camera_suspended=true; }
bool take_camera_suspension() { return camera_suspended.exchange(false); }
void note_camera_view(const dingosdk::first_person::Matrix& pose,dingosdk::first_person::Quat glance) {
    using namespace dingosdk::first_person;
    const auto base=normalized(multiply(orientation(pose),conjugate(glance)));
    const auto back=rotate(base,{0,0,1});
    std::lock_guard lock(mutex);if(marker_reset.pending || marker_reset.neutral) return;native_heading=std::atan2(back[0],back[2])*180/std::numbers::pi_v<float>;
    displayed_pitch=std::asin(std::clamp(-back[1],-1.f,1.f))*180/std::numbers::pi_v<float>;
    heading_time=now();
}
void note_native_camera_heading(const dingosdk::first_person::Matrix& pose) { note_camera_view(pose,{0,0,0,1}); }
StickAim stick_camera_aim() { const auto value=snapshot();return live_foot(value) ? value.aim:StickAim{}; }
StickInputPolicy stick_input_policy(unsigned slot) {
    if(const auto spectator=spectator_input_policy(slot);spectator.mask) return spectator;
    const auto value=snapshot();
    if(slot!=~0u && (slot>=4 || value.device>15 || !(value.device&(1u<<slot)))) return {};
    if(!value.aim.capture || !live_foot(value)) return {};
    if(!value.board) {
        const auto config=menu_snapshot().config;
        if(native_head_look_passthrough(config.stick.native_head_look,value.aim.active,config.stick.enabled,
            config.look_only,config.stick.turn_character,value.turn,value.recovery)) return {};
        return {~0u,{}};
    }
    const auto settings=menu_snapshot().config.board;
    return settings.enabled ? StickInputPolicy{settings.modifier,value.native_axes}:StickInputPolicy{};
}
std::uint32_t stick_capture_mask(unsigned slot) { return stick_input_policy(slot).mask; }
bool stick_captures_input(unsigned slot,std::uint32_t buttons) {
    const auto mask=stick_capture_mask(slot);
    return mask && (mask==~0u || (buttons&mask)==mask);
}
void tick_stick_look(const Config& config,const GameplayInput& input,const ComfortSnapshot& owner,bool inhibited) {
    observe_marker_return(owner);
    const double time=now();const auto movement=live_camera_movement();
    const auto head=read_animation_head(owner);
    Matrix body{};const bool body_valid=owner.rig && dingosdk::memory::read(owner.rig+0x4720,body) && dingosdk::valid_flight_transform(body);
    const auto effective=effective_camera_config(config,stock_first_person_requested(),owner,movement);
    const bool eligible=ready && !inhibited && movement.mode==1 && camera_look_allowed(config,owner,movement) &&
        (!bail_camera_locked(owner,movement) || !movement.view_hold) && head.valid && body_valid;
    std::lock_guard lock(mutex);
    if(marker_reset.pending || marker_reset.teleport) {look.reset();board_look.reset();request={};return;}
    const auto axes=radial_stick({input.right_x,input.right_y},movement.mode==2 ? config.board.deadzone:config.stick.deadzone);
    bool seeded=false;
    if(marker_reset.neutral) {
        const bool allowed=ready && !inhibited && input.available && input.device && head.valid && body_valid &&
            camera_look_allowed(config,owner,movement) && (movement.mode==1 || movement.mode==2);
        if(!marker_reset.take_neutral(allowed,axes.x==0 && axes.y==0)) {look.reset();board_look.reset();request={};return;}
        look.seed(config.stick,input.device,time,native_heading,displayed_pitch,matrix_heading(body));
        heading_time=time;recenter_handoff=false;seeded=true;
    }
    if(!seeded && (bail_camera_locked(owner,movement) || movement.mode==2)) recenter_handoff=true;
    if(movement.mode==2) {
        look.reset();
        const bool board_eligible=ready && !inhibited && board_free_look_allowed(config,owner,movement) && head.valid && body_valid;
        if(request.owner.core && (!request.board || request.owner.core!=owner.core || request.owner.entity!=owner.entity ||
            request.owner.context!=owner.context || request.owner.rig!=owner.rig)) board_look.reset();
        const auto aim=board_look.update(config.board,{input.right_x,input.right_y},board_eligible,input,time);
        request={owner,aim,false,input.device,time,0,head.valid,false,true,board_look.native_axes(config.board),marker_reset.generation};
        return;
    }
    board_look.reset();
    if(request.board) look.reset();
    if(inhibited || !config.stick.enabled || !input.available || !input.device) {
        look.reset();board_look.reset();request={};return;
    }
    if(!eligible || time-heading_time>=.5) { look.reset();board_look.reset();request={};return; }
    const float body_heading=matrix_heading(body);
    if(request.owner.core && std::array<std::uintptr_t,4>{request.owner.core,request.owner.entity,request.owner.context,request.owner.rig}!=std::array<std::uintptr_t,4>{owner.core,owner.entity,owner.context,owner.rig}) { look.reset(); }
    const bool turn=!config.look_only && config.stick.turn_character && character_facing_allowed(owner,movement);
    const bool recovery=!config.look_only && bail_camera_locked(owner,movement);
    if(request.owner.core && request.recovery!=recovery) look.reset();
    if(turn && !request.turn && !seeded) {
        if(recenter_handoff) {
            // Camera +Z points backward; after dismount/bail, face away from the body.
            look.recenter_facing();recenter_handoff=false;
            log("On-foot Camera and character look: recentering after board/bail handoff; waiting for neutral stick.");
        }
        else look.rebase_facing(body_heading);
    }
    auto stick=config.stick;if(recovery) stick.turn_character=false;
    auto aim=look.update(stick,{input.right_x,input.right_y},true,input.device,time,recovery ? 0:native_heading,
        recovery ? 0:body_heading,effective.stick_spring,recovery ? 0:displayed_pitch);
    request={owner,aim,turn,input.device,time,body_heading,head.valid,recovery,false,{},marker_reset.generation};
}
bool attach_stick_runtime(std::uintptr_t base) {
    using namespace dingosdk;
    namespace spawn=game::build::v20260929::client_source_spawn;
    // ABI and consumer are already proven by custom.2's noclip motion hook.
    const auto target=resolve_stock_handler(base+spawn::skater_motion);
    if(!target || !stock_bridge_ready()) { log("ERROR: native on-foot facing hook unavailable: verified stock motion handler missing.");return false; }
    game_base=base;void* original{};
    auto status=owned_hook_prepare(reinterpret_cast<void*>(target),reinterpret_cast<void*>(&motion_hook),&original);
    if(status==HookOk) {
        original_motion=reinterpret_cast<Motion>(original);
        status=hook_queue_enable(reinterpret_cast<void*>(target));
        if(status==HookOk) status=apply_owned_hooks();
    }
    if(status!=HookOk) { log("ERROR: native facing hook attach failed: "+std::to_string(status));return false; }
    if(!install_stick_input_filter()) { log("ERROR: stick input filters unavailable; stick look remains disabled.");return false; }
    ready=true;
    log("On-foot stick look ready: native motion/facing chained; on-board stick input captured only while the configured modifier is held.");return true;
}
bool install_stick_runtime(std::uintptr_t base) {
    getup_verified=getup_contract::verified([](std::uintptr_t address,void* data,std::size_t size) {
        return dingosdk::memory::read_bytes(address,data,size);
    },base);
    log(getup_verified ? "Get-up camera lock verified: native ragdoll animation request, camera-only hold.":
        "ERROR: get-up animation contract differs; get-up camera lock unavailable.");
    recovery_contract=bail_recovery_contract::verified([](std::uintptr_t address,void* data,std::size_t size) {
        return dingosdk::memory::read_bytes(address,data,size);
    },base);
    log(recovery_contract ? "Bail recovery control verified: owned Ground child, native selection flags and unoverridden input path.":
        "ERROR: bail recovery input contract differs; bail look remains blocked.");
    jump_contract=jump_state_contract::verified([](std::uintptr_t address,void* data,std::size_t size) {
        return dingosdk::memory::read_bytes(address,data,size);
    },base);
    log(jump_contract ? "Jump episode contracts verified: Trajectory/Falling/SimulatedRagdoll; ground/board/bail terminate attachment.":
        "ERROR: extended jump episode contract differs; Trajectory/SimulatedRagdoll recognition disabled.");
    namespace spawn=dingosdk::game::build::v20260929::client_source_spawn;
    if(resolve_stock_handler(base+spawn::skater_motion)) return attach_stick_runtime(base);
    // Stock installs the motion bridge after its native source initializes.
    // Wait outside the game/loader thread instead of permanently disabling an
    // early-loaded DLL or attaching another hook directly to Skate.exe.
    log("Waiting for stock motion bridge on initialization worker.");
    for(unsigned attempt=0;attempt<3000;++attempt) {
        if(resolve_stock_handler(base+spawn::skater_motion)) return attach_stick_runtime(base);
        Sleep(100);
    }
    log("ERROR: stock motion bridge readiness timed out.");return false;
}
}
