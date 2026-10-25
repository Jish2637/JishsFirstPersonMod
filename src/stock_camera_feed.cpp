#include "stock_bridge.h"
#include "game_camera_ownership.h"
#include "stock_contract.h"
#include "bridge_resolver.h"
#include "settings.h"
#include "look_smoothing.h"
#include "stick_runtime.h"
#include "camera_transition.h"
#include "camera_policy.h"
#include "bail_camera_trace.h"
#include "bail_view.h"
#include "bail_collision.h"
#include "body_flip_runtime.h"
#include "spectator_runtime.h"
#include "owned_hooks.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/20260929/client_source_spawn.h"
#include "Extension/Skater/free_flight.h"
#include "Extension/Skater/client_source_spawn_internal.h"
#include <atomic>
#include <intrin.h>
#include <chrono>
#include <mutex>
namespace jishsfirstpersonmod {
namespace {
using Matrix=dingosdk::first_person::Matrix;
using Transform=void(*)(std::uintptr_t,const Matrix*);
using WriteFov=bool(*)(std::uintptr_t,float);
Transform original_transform{};WriteFov original_fov{};HMODULE stock_module{};
std::uintptr_t game_base{};
std::atomic<bool> feed_prepared{},transform_installed{};
std::atomic<std::uintptr_t> watched_camera{};
std::atomic<float> look_pitch{},look_yaw{},game_fov{90};
std::atomic<double> look_frame{};
std::mutex smoothing_mutex;
LookSmoothing smoothing;
CameraTransition transition;
BailView bail_view;
body_flip::Camera flip_camera;
BailCollision collision;
CollisionPoint collision_seed{},collision_player{};
bool collision_seed_ready{},collision_player_ready{};
std::uintptr_t collision_world{};
unsigned collision_wipeout{};bool collision_bailing{};
std::uintptr_t smoothing_camera{};
std::uint64_t smoothing_marker_generation{};
std::array<std::uintptr_t,5> transition_owner{};
bool from_stock(void* caller) {
    HMODULE owner{};return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(caller),&owner) && owner==stock_module;
}
void reset_camera_smoothing() {
    collision_wipeout=0;collision_bailing=false;
    reset_body_flip();flip_camera.reset();
    collision.reset();collision_seed_ready=collision_player_ready=false;collision_world=0;
    smoothing.reset();transition.reset();bail_view.reset();set_bail_view_suspended(false);smoothing_camera=0;
}
bool write_spectator_render(std::uintptr_t client,std::uintptr_t camera,const Matrix& pose,float fov) {
    auto rejected=[](const std::string& reason) {
        static ULONGLONG last{};
        const auto tick=GetTickCount64();
        if(!last || tick-last>=1000) { last=tick;log("Spectator render camera rejected: "+reason); }
        return false;
    };
    try {
        dingosdk::client_source::detail::SourceTrial trial;trial.base=game_base;
        const auto current=dingosdk::client_source::detail::source_camera_snapshot(trial,client);
        // ReSkate checks input inactivity before entering party spectate, not
        // after activating mode 1. Match its owned-camera check here: the
        // activation flag is not evidence that another camera took ownership.
        if(current.mode!=1 || current.active!=camera || !current.view_enabled || !dingosdk::valid_flight_transform(pose))
            return rejected("mode="+std::to_string(current.mode)+" active="+std::to_string(current.active)+
                " expected="+std::to_string(camera)+" enabled="+std::to_string(current.view_enabled));
        original_transform(camera,&pose);
        if(fov>1 && fov<175)original_fov(camera,fov);
        return true;
    } catch(const dingosdk::client_source::detail::SourceGuard& issue) { return rejected(issue.message); }
      catch(...) { return rejected("Native camera snapshot unavailable."); }
}
void transform_hook(std::uintptr_t camera,const Matrix* input) {
    if(spectator_transform_call(reinterpret_cast<std::uintptr_t>(_ReturnAddress()))) {
        { std::lock_guard lock(smoothing_mutex);reset_camera_smoothing(); }
        Matrix normal{},output{};
        if(input && dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(input),normal) && dingosdk::valid_flight_transform(normal) &&
            spectator_transform(camera,normal,output)) { original_transform(camera,&output);return; }
        original_transform(camera,input);return;
    }
    if(game_camera_suspended()) {
        { std::lock_guard lock(smoothing_mutex);reset_camera_smoothing(); }
        original_transform(camera,input);return;
    }
    const auto marker_generation=observe_marker_return(comfort_snapshot());
    const auto watched=watched_camera.load();
    if(compatibility::enabled() && input && (!watched || camera==watched) && stock_first_person_requested() && from_stock(_ReturnAddress())) {
        watched_camera=camera;Matrix native{};
        if(dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(input),native) && dingosdk::valid_flight_transform(native)) {
            auto matrix=native;auto movement=live_camera_movement();
            const auto state=comfort_snapshot();const auto settings=menu_snapshot();const auto& config=settings.config;
            // Consume only the stock pose, never the previously composed look.
            refresh_marker_native_view(state,native,state.state_valid && movement.mode &&
                camera_look_allowed(config,state,movement) && read_animation_head(state).valid);
            {
                std::lock_guard lock(smoothing_mutex);
                if(smoothing_marker_generation!=marker_generation) {
                    reset_camera_smoothing();smoothing_marker_generation=marker_generation;
                    if(marker_return_status().pending || marker_return_status().neutral) {look_pitch=0;look_yaw=0;}
                }
            }
            if(config.look_only) {
                dingosdk::first_person::Quat glance{0,0,0,1};
                {
                    std::lock_guard lock(smoothing_mutex);
                    const std::array<std::uintptr_t,5> identity{state.core,state.client,state.entity,state.context,state.rig};
                    if(smoothing_camera!=camera || identity!=transition_owner) { reset_camera_smoothing();smoothing_camera=camera;transition_owner=identity; }
                    bail_view.reset();
                    collision.reset();collision_seed_ready=collision_player_ready=false;collision_world=0;
                    if(camera_look_allowed(config,state,movement)) {
                        const Matrix identity_pose{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
                        glance=dingosdk::first_person::orientation(smoothing.apply(identity_pose,glance_settings(config),{look_pitch.load(),look_yaw.load()},look_frame.load(),config.link_smoothing));
                        const auto stick_aim=stick_camera_aim();
                        matrix=movement.mode==2 ? aim_head_relative(native,stick_aim):aim_camera(native,stick_aim);
                        dingosdk::first_person::write(matrix,dingosdk::first_person::normalized(dingosdk::first_person::multiply(dingosdk::first_person::orientation(matrix),glance)),{native[12],native[13],native[14]});
                    } else { smoothing.reset();stop_stick_look(); }
                }
                note_camera_view(matrix,glance);
                note_emote_camera(matrix);
                record_bail_camera(native,matrix,state,movement,config,{},look_pitch.load(),look_yaw.load(),{},read_animation_head(state),settings.revision);
                original_transform(camera,&matrix);return;
            }
            const auto head=read_animation_head(state);const auto pitch=look_pitch.load(),yaw=look_yaw.load();
            bool suspended{},changed{};StickAim aim{};BailViewInfo view;
            dingosdk::first_person::Quat glance{0,0,0,1};
            const double frame=look_frame.load();
            {
                std::lock_guard lock(smoothing_mutex);
                const std::array<std::uintptr_t,5> identity{state.core,state.client,state.entity,state.context,state.rig};
                if(smoothing_camera!=camera || identity!=transition_owner) { reset_camera_smoothing();smoothing_camera=camera;transition_owner=identity; }
                bool menu_visible{};DWORD foreground_pid{};
                const auto foreground=GetForegroundWindow();
                const bool focused=foreground && GetWindowThreadProcessId(foreground,&foreground_pid) && foreground_pid==GetCurrentProcessId();
                const bool interrupted=state.suppressed || !focused || (overlay_available(menu_visible) && menu_visible);
                if(interrupted) reset_camera_smoothing();
                if(!movement.mode || !state.state_valid || interrupted) { reset_camera_smoothing();suspended=true; }
                else {
                    auto flip=body_flip_status();
                    const bool bail=bail_camera_locked(state,movement);
                    if(bail || flip.owner!=std::array<std::uintptr_t,4>{state.core,state.entity,state.context,state.rig}) {
                        reset_body_flip();flip=body_flip_status();flip_camera.reset();
                    }
                    const bool flip_owned=flip.owns_camera();
                    movement.body_flip_head_lock=flip.owns_head();movement.body_flip_camera_owned=flip_owned;
                    movement.body_flip_tilt=flip.tilt;movement.body_flip_rate=flip.rate;movement.body_flip_phase=static_cast<unsigned>(flip.phase);
                    const bool attached=head_position_attached(config,state,movement);
                    const bool normal=character_facing_allowed(state,movement) ||
                        (camera_overrides_allowed(state,movement) && movement.mode==2 && movement.custom_mode==2);
                    const bool recovery=bail && camera_look_allowed(config,state,movement) && head.valid;
                    if(config.third_person_bailing) bail_view.reset();
                    const unsigned wipeout=unsigned(state.animation_wipeout)|(unsigned(state.wipeout_request)<<1);
                    if((bail && !collision_bailing) || (movement.recovery_verified && state.animation_valid && (wipeout&~collision_wipeout))) {
                        collision.reset();collision_seed_ready=false;
                    }
                    collision_bailing=bail;
                    if(movement.recovery_verified && state.animation_valid) collision_wipeout=wipeout;
                    const bool previous_getup=bail_view.getup_episode();
                    const auto* player=movement.position_valid ? &movement.position:nullptr;
                    const bool control=movement.recovery_controllable || (!bail && normal);
                    CollisionPoint normal_position{native[12],native[13],native[14]};
                    const bool renewed=bail_view.observe_wipeout_request(movement.recovery_verified && state.animation_valid,
                        unsigned(state.animation_wipeout) | (unsigned(state.wipeout_request)<<1));
                    matrix=bail_view.apply(native,head,bail,normal,frame,control,
                        !config.third_person_bailing && movement.getting_up,player,renewed);view=bail_view.info();
                    const bool detached=bail_view.detached();
                    const bool owns_return=detached || previous_getup;
                    movement.view_hold=view.blocked || (bail && !view.attached);set_bail_view_suspended(movement.view_hold);
                    const bool valid=camera_overrides_allowed(state,movement) && head.valid;
                    const bool eligible=valid && camera_look_allowed(config,state,movement);
                    if(bail) {
                        smoothing.reset();changed=true;
                        suspended=!recovery || !head.valid || !view.calibrated;
                        if(bail_view.getup_episode() && control) {
                            // Recover toward ordinary native facing, not animated head rotation.
                            matrix=native;
                            if(detached && head.valid && view.calibrated) matrix=bail_view.attach(matrix,head);
                        }
                        if(recovery && head.valid && view.calibrated) {
                            aim=stick_camera_aim();
                            // Recovery aim is a local offset, never absolute world-up yaw.
                            matrix=aim_head_relative(matrix,aim);
                            view.recovery_yaw=aim.active ? aim.heading:0;view.recovery_pitch=aim.active ? aim.pitch:0;
                        }
                        dingosdk::first_person::Settings handoff;
                        // Keep the displayed bail history for the normal-camera release blend.
                        matrix=bail_view.finish(matrix,head,player,frame,control,&normal_position);
                        if(owns_return) transition.reset();
                        else matrix=transition.apply(matrix,handoff,state.core,movement.mode,aim.active,frame,movement.state,
                            nullptr,nullptr,view.attached,recovery,true,false);
                        const auto final_view=bail_view.info();
                        view.getup_camera_phase=final_view.getup_camera_phase;view.getup_blend=final_view.getup_blend;
                        view.head_target=final_view.head_target;view.attached=final_view.attached;view.getup_locked=final_view.getup_locked;
                    } else if(!valid) {
                        reset_body_flip();flip_camera.reset();
                        smoothing.reset();transition.reset();suspended=true;
                        collision.reset();collision_seed_ready=collision_player_ready=false;collision_world=0;
                        bail_view.reset();view=bail_view.info();
                    } else {
                        if(!eligible || flip_owned) { smoothing.reset();suspended=!eligible; }
                        else {
                            const auto effective=effective_camera_config(config,stock_first_person_requested(),state,movement);
                            const Matrix identity_pose{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
                            glance=dingosdk::first_person::orientation(smoothing.apply(identity_pose,glance_settings(effective),{pitch,yaw},frame,config.link_smoothing));
                            aim=stick_camera_aim();
                            matrix=movement.mode==2 ? aim_head_relative(matrix,aim):aim_camera(matrix,aim);
                            dingosdk::first_person::write(matrix,dingosdk::first_person::normalized(dingosdk::first_person::multiply(dingosdk::first_person::orientation(matrix),glance)),{matrix[12],matrix[13],matrix[14]});
                        }
                        if(attached && !flip_owned) matrix=bail_view.attach(matrix,head);
                        dingosdk::first_person::Settings handoff;handoff.enabled=config.stick.smooth_transitions;
                        const bool mount_handoff=config.stick.enabled && movement.mounting;
                        handoff.up=handoff.down=handoff.left=handoff.right=(mount_handoff || movement.mode==2) ? config.stick.mount_smoothing:config.stick.dismount_smoothing;
                        // Native attachment disables positional Spring, not saved handoff strengths.
                        const auto look_settings=glance_settings(config);
                        normal_position={matrix[12],matrix[13],matrix[14]};
                        matrix=bail_view.finish(matrix,head,player,frame,control,&normal_position);
                        if(owns_return || flip_owned) transition.reset();
                        else matrix=transition.apply(matrix,handoff,state.core,movement.mode,aim.active,frame,movement.state,
                            &config.spring,&look_settings,attached,eligible,!eligible,mount_handoff,
                            camera_handoff_direction_only(config));
                        const auto flip_target=body_flip::head_target(head.head,config.spring);
                        matrix=flip_camera.apply(matrix,flip_target,flip,frame);
                        view=bail_view.info();
                        if(flip_owned) {
                            view.head_target=body_flip::position(flip_target);
                            view.target_rotation=dingosdk::first_person::orientation(flip_target);
                            view.offset={config.spring.offset[0],config.spring.offset[1],-config.spring.offset[2]};
                            view.calibrated=true;view.attached=flip.phase==body_flip::Phase::locked;
                        }
                        changed=true;
                    }
                    if(changed) {
                        const CollisionPoint desired{matrix[12],matrix[13],matrix[14]};
                        if(player && collision_player_ready && dingosdk::first_person::length(dingosdk::first_person::subtract(*player,collision_player))>5) {
                            collision.reset();collision_seed_ready=false;
                        }
                        if(player) { collision_player=*player;collision_player_ready=true; }
                        const bool protect=config.bail_camera_collision && !config.third_person_bailing && bail_view.collision_owned(bail);
                        const auto world=protect ? bail_collision_world():0;
                        if(protect && world) {
                            if(collision_world!=world) { collision.reset();if(collision_world) collision_seed_ready=false;collision_world=world; }
                            const CollisionPoint seed=collision_seed_ready ? collision_seed:CollisionPoint{native[12],native[13],native[14]};
                            // During normal control this is the final composed
                            // normal position (including head attachment), not
                            // the stock camera position saved before composition.
                            const CollisionPoint normal_target=normal_position;
                            CollisionEscape escape;
                            const CollisionEscape* escape_frame=nullptr;
                            if(head.valid && view.calibrated) {
                                escape.head=view.head_target;
                                const auto rotation=head_camera_rotation(head);
                                escape.axes={dingosdk::first_person::rotate(rotation,{0,1,0}),
                                    dingosdk::first_person::rotate(rotation,{1,0,0}),dingosdk::first_person::rotate(rotation,{0,0,-1})};
                                escape_frame=&escape;
                            }
                            const auto corrected=collision.resolve(desired,seed,frame,[&](auto from,auto to,float radius) { return native_bail_sweep(world,from,to,radius); },
                                control && bail_view.detached(),&normal_target,bail_view.position_return_remaining(frame),
                                bail && !control && !bail_view.detached() && head.valid && view.calibrated,escape_frame);
                            for(unsigned i=0;i<3;++i) matrix[12+i]=corrected[i];
                            bail_view.accept_displayed_position(corrected);
                            view.collision=collision.info();
                            if(std::string_view(view.collision.fallback_reason)!="none") {
                                static ULONGLONG last_report{};const auto tick=GetTickCount64();
                                if(!last_report || tick-last_report>=5000) {
                                    last_report=tick;log(std::string("Bail collision head fallback: ")+view.collision.fallback_reason+
                                        "; obstruction_ms="+std::to_string(view.collision.obstruction_seconds*1000));
                                }
                            }
                        } else { collision.reset();collision_world=0; }
                        collision_seed={matrix[12],matrix[13],matrix[14]};collision_seed_ready=true;
                        if(!bail && valid) bail_view.remember(matrix,native,head,normal && !flip_owned);
                    }
                }
            }
            if(suspended) { suspend_custom_camera();look_pitch=0;look_yaw=0; }
            note_camera_view(matrix,glance);
            note_emote_camera(matrix);
            record_bail_camera(native,matrix,state,movement,config,aim,pitch,yaw,view,head,settings.revision);
            original_transform(camera,changed ? &matrix:input);return;
        }
    }
    if(!stock_first_person_requested() || (camera==watched && from_stock(_ReturnAddress()))) { std::lock_guard lock(smoothing_mutex);reset_camera_smoothing(); }
    original_transform(camera,input);
}
bool fov_hook(std::uintptr_t camera,float value) {
    if(spectator_fov_call(reinterpret_cast<std::uintptr_t>(_ReturnAddress())))return original_fov(camera,spectator_fov(camera,value));
    if(!menu_snapshot().config.look_only && !game_camera_suspended() && camera==watched_camera.load() && stock_first_person_requested() && menu_snapshot().config.fov==0 && from_stock(_ReturnAddress()))
        value=game_fov.load();
    return original_fov(camera,value);
}
bool activate_transform_feed() {
    if(!feed_prepared.load(std::memory_order_acquire)) return false;
    if(transform_installed) return true;
    if(!stock_module || !game_base || !original_transform) return false;
    using namespace dingosdk;
    const auto stock=reinterpret_cast<std::uintptr_t>(stock_module);
    std::uint8_t initialized{};std::uintptr_t base{},callback{};
    const auto& profile=stock_contract::contract();
    if(!memory::read(stock+profile.source_initialized_rva,initialized) || !initialized ||
       !memory::read(stock+profile.source_base_rva,base) || base!=game_base ||
       !memory::read(stock+profile.source_transform_rva,callback) || callback!=reinterpret_cast<std::uintptr_t>(original_transform)) return false;
    auto* slot=reinterpret_cast<void* volatile*>(stock+profile.source_transform_rva);
    MEMORY_BASIC_INFORMATION page{};
    if(!VirtualQuery(const_cast<void*>(reinterpret_cast<const volatile void*>(slot)),&page,sizeof(page)) || page.State!=MEM_COMMIT ||
        (page.Protect&0xff)!=PAGE_READWRITE) return false;
    if(InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(&transform_hook),reinterpret_cast<void*>(original_transform))!=reinterpret_cast<void*>(original_transform)) return false;
    transform_installed=true;
    log("Stock SourceTrial transform callback chained: look feed active; game code and camera ownership unchanged.");return true;
}
}
void set_stock_glance(std::uintptr_t camera,float fov,float pitch,float yaw) {
    if(compatibility::enabled()) activate_transform_feed();
    if(!stock_first_person_requested()) {
        std::lock_guard lock(smoothing_mutex);reset_camera_smoothing();
    }
    watched_camera=camera;look_pitch=pitch;look_yaw=yaw;
    look_frame=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if(std::isfinite(fov) && fov>1 && fov<175) game_fov=fov;
}
bool install_stock_camera_feed(std::uintptr_t base,HMODULE stock) {
    using namespace dingosdk;
    namespace addresses=game::build::v20260929::client_source_spawn;
    std::array<unsigned char,24> prefix{};
    if(!memory::read(base+addresses::camera_transform_setter,prefix) || prefix!=addresses::camera_transform_prefix) {
        log("ERROR: native camera transform prefix differs; look input feed not installed.");return false;
    }
    if(!stock_bridge_ready()) return false;
    stock_module=stock;game_base=base;
    original_transform=reinterpret_cast<Transform>(base+addresses::camera_transform_setter);
    feed_prepared.store(true,std::memory_order_release);
    set_background_flush(flush_bail_camera_trace);
    log("Camera trace v12 ready: smoothed get-up head position and seamless 300 ms return; no get-up character writes.");
    bool source_ready=false;
    for(unsigned tries=0;tries<6000;++tries) {
        std::uint8_t initialized{};std::uintptr_t live_base{},callback{};
        const auto image=reinterpret_cast<std::uintptr_t>(stock);
        const auto& profile=stock_contract::contract();
        if(!memory::read(image+profile.source_initialized_rva,initialized) ||
           !memory::read(image+profile.source_base_rva,live_base) ||
           !memory::read(image+profile.source_transform_rva,callback)) {
            log("ERROR: SourceTrial live camera slots unreadable.");return false;
        }
        const auto readiness=compatibility::camera_readiness(initialized,live_base,base,callback,reinterpret_cast<std::uintptr_t>(original_transform));
        if(readiness==compatibility::Readiness::incompatible) { log("ERROR: SourceTrial live camera slots changed.");return false; }
        if(readiness==compatibility::Readiness::ready) { source_ready=true;break; }
        Sleep(100);
    }
    if(!source_ready) { log("ERROR: SourceTrial readiness timed out; no compatibility conclusion.");return false; }
    if(!activate_transform_feed()) { log("ERROR: SourceTrial callback ownership or page protection changed.");return false; }
    void* fov{};
    const auto fov_target=reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(stock)+stock_contract::contract().write_fov_rva);
    const auto prepared=owned_hook_prepare(fov_target,reinterpret_cast<void*>(&fov_hook),&fov);
    if(prepared!=HookOk) { log("ERROR: stock FOV hook preparation failed: "+std::to_string(prepared));return false; }
    original_fov=reinterpret_cast<WriteFov>(fov);
    auto result=hook_queue_enable(fov_target);
    if(result==HookOk) result=apply_owned_hooks();
    if(result!=HookOk) { log("ERROR: stock FOV hook attach failed: "+std::to_string(result));return false; }
    initialize_spectator(base,reinterpret_cast<std::uintptr_t>(stock),write_spectator_render);
    log("Native camera input feed prepared without patching Skate camera code.");return true;
}
bool restore_stock_camera_feed() {
    reset_spectator_view();
    feed_prepared=false;
    if(!transform_installed) return true;
    auto* slot=reinterpret_cast<void* volatile*>(reinterpret_cast<std::uintptr_t>(stock_module)+stock_contract::contract().source_transform_rva);
    const auto restored=dingosdk::restore_owned_slot(slot,reinterpret_cast<void*>(&transform_hook),reinterpret_cast<void*>(original_transform));
    // A replacement installed by someone else belongs to them; leave it untouched.
    transform_installed=false;return restored;
}
void clear_stock_camera_blends() {
    std::lock_guard lock(smoothing_mutex);
    reset_camera_smoothing();look_pitch=0;look_yaw=0;
}
}
