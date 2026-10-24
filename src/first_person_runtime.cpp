#include "native.h"
#include "menu.h"
#include "mod.h"
#include "settings.h"
#include "stock_bridge.h"
#include "comfort_policy.h"
#include "game_camera_ownership.h"
#include "stick_runtime.h"
#include "camera_policy.h"
#include "head_attachment.h"
#include "body_flip_runtime.h"
#include "spectator_runtime.h"
#include <chrono>
#include "glance_controls.h"
#include "marker_glance.h"
#include "stick_input_filter.h"
namespace jishsfirstpersonmod {
void tick_mod(std::uintptr_t client) noexcept {
    using namespace dingosdk;
    using namespace dingosdk::client_source::detail;
    static ControllerComboLatch toggle;
    static GlanceControlsState look;
    static MarkerGlanceMemory marker_glance;
    static std::uint64_t glance_marker_generation{};
    static std::array<std::uintptr_t,4> glance_owner{};
    static std::uint64_t revision{};
    static Config input_config;
    static std::uintptr_t context{},camera{};
    static float saved_game_fov=90;
    static std::string last_wait;
    static bool reported{};
    static std::mutex input_mutex;
    std::unique_lock input_lock(input_mutex,std::try_to_lock);
    if(!input_lock.owns_lock()) return;
    try {
        const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        SourceReader reader;
        source_require(reader.pointer(client)==base+addr::engine::client_vtable,"Waiting for native client input.");
        const auto next_context=reader.pointer(client,8);
        const auto state=reader.value<std::uint32_t>(client,0xc4);reader.verify();
        if(!reported) { reported=true;log("Controller tick reached native client; state="+std::to_string(state)); }
        if(context!=next_context) { context=next_context;camera=0;toggle={};look={};marker_glance.clear();clear_comfort_state();clear_marker_return(); }
        const auto settings=menu_snapshot();
        if(settings.revision!=revision) {
            if(!revision || input_config.stick.lock_mount_jump!=settings.config.stick.lock_mount_jump ||
                input_config.stick.lock_hippy_jump!=settings.config.stick.lock_hippy_jump ||
                input_config.disable_spring_on_foot!=settings.config.disable_spring_on_foot ||
                input_config.look_only!=settings.config.look_only || input_config.stick.enabled!=settings.config.stick.enabled)
                log("Attachment settings reached live camera tick: revision="+std::to_string(settings.revision)+
                    " jump="+std::to_string(settings.config.stick.lock_mount_jump)+" hippy="+std::to_string(settings.config.stick.lock_hippy_jump)+
                    " disable_foot="+std::to_string(settings.config.disable_spring_on_foot)+" stick="+std::to_string(settings.config.stick.enabled)+
                    " look_only="+std::to_string(settings.config.look_only));
            bool changed=input_config.look_only!=settings.config.look_only || input_config.toggle!=settings.config.toggle ||
                input_config.toggle_glances!=settings.config.toggle_glances ||
                input_config.remember_marker_glance!=settings.config.remember_marker_glance;
            for(unsigned i=0;i<4;++i) changed=changed || input_config.look[i].combo!=settings.config.look[i].combo ||
                input_config.look[i].angle!=settings.config.look[i].angle;
            if(changed) { toggle={};look={};marker_glance.clear();stop_stick_look();clear_stock_camera_blends(); }
            input_config=settings.config; revision=settings.revision;
        }
        update_game_camera_ownership(client,base);
        update_comfort(client,base);
        const bool world=state==13 || state==21;
        if(!world) clear_marker_return();
        if(world) {
            try {
                SourceTrial trial;trial.base=base;
                const auto snapshot=source_camera_snapshot(trial,client);
                camera=snapshot.identity.camera;
                if(snapshot.mode==0 && snapshot.active) {
                    float fov{};
                    if(memory::read(snapshot.active+camera_fov_offset,fov) && std::isfinite(fov) && fov>1 && fov<175) saved_game_fov=fov;
                }
            } catch(const SourceGuard&) { /* Stock owns readiness; retain only the same world's last input target. */ }
        } else camera=0;
        bool visible{};
        const bool menu_ready=overlay_available(visible);
        const bool ready=world && stock_bridge_ready() && stock_camera_available();
        const auto controller=read_controller();
        DWORD foreground_pid{};
        const auto foreground=GetForegroundWindow();
        const bool focused=foreground && GetWindowThreadProcessId(foreground,&foreground_pid) && foreground_pid==GetCurrentProcessId();
        const bool blocked=panel_gameplay_blocked() || !ready || !menu_ready || visible || !controller.available || !focused;
        // The same entity teleport flag used by the comfort reader. Validate
        // the previous local entity before observing its transient transition.
        std::uintptr_t entity_vtable{},entity_context{};std::uint8_t teleport_flag{};
        const bool returning_to_marker=world && glance_owner[1] &&
            memory::read(glance_owner[1],entity_vtable) && entity_vtable==base+addr::engine::skater_entity_vtable &&
            memory::read(glance_owner[1]+0x20,entity_context) && entity_context==context &&
            memory::read(glance_owner[1]+0x7e0,teleport_flag) && teleport_flag!=0;
        const bool remember_glance=settings.config.remember_marker_glance && settings.config.toggle_glances &&
            world && menu_ready && !visible && !panel_gameplay_blocked() && focused && controller.available &&
            !game_camera_suspended() && stock_first_person_requested();
        marker_glance.observe(remember_glance,returning_to_marker,look.selection(),controller.device,
            context,source_flight_now());
        const auto input_route=route_controller_input(settings.config,blocked);
        const auto comfort=comfort_snapshot();
        observe_marker_return(comfort,glance_owner[1]);
        const auto marker=marker_return_status();
        if(marker.generation!=glance_marker_generation) {
            glance_marker_generation=marker.generation;
            if(marker.pending || marker.neutral) {
                // Position-only detection must obey the same optional glance
                // memory as a flag-observed return, including camera-first order.
                marker_glance.observe(remember_glance,true,look.selection(),controller.device,context,source_flight_now());
                look.reset();
            }
        }
        const std::array<std::uintptr_t,4> current_owner{comfort.core,comfort.entity,comfort.context,comfort.rig};
        // Publish toggle suppression before any head, camera or hair-related
        // work. Reconcile actual binding/world changes here without opening an
        // input gap just because glance settings or camera ownership changed.
        const bool toggle_filter_blocked=panel_gameplay_blocked() || !world || !stock_bridge_ready() || !menu_ready || visible || !controller.available || !focused;
        const auto toggle_filter_route=route_controller_input(settings.config,toggle_filter_blocked);
        const auto emote_toggles=toggle_emote_bindings(settings.config,toggle_filter_blocked);
        update_spectator_session(world && game_camera_suspended(),settings.config,context,
            !world || !menu_ready || visible || !focused,client);
        update_spectator_input(settings.config,controller,toggle_filter_blocked);
        publish_toggle_filter(emote_toggles.companion,emote_toggles.enabled(),controller.device,context,emote_toggles.native);
        if(current_owner!=glance_owner) { glance_owner=current_owner;look.reset(); }
        auto movement=live_camera_movement();
        if(!settings.config.look_only) tick_hippy_jump(settings.config,comfort,movement,source_flight_now());
        if(!stock_first_person_requested()) { set_bail_view_suspended(false);movement.view_hold=false; }
        tick_body_flip(settings.config,comfort,movement,
            std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(),
            !world || !ready || !menu_ready || visible || !focused || game_camera_suspended() ||
            !stock_first_person_requested() || comfort.suppressed);
        const auto flip=body_flip_status();movement.body_flip_head_lock=flip.owns_head();movement.body_flip_camera_owned=flip.owns_camera();
        const bool bail=bail_camera_locked(comfort,movement);
        if(!bail) { set_bail_view_suspended(false);movement.view_hold=false; }
        const bool native_camera=(settings.config.native.board_only && movement.mode==1) || game_camera_suspended() || !camera_look_allowed(settings.config,comfort,movement) || !read_animation_head(comfort).valid;
        static std::optional<bool> last_spring_target;
        const auto spring_target=effective_camera_config(settings.config,stock_first_person_requested(),comfort,movement).spring.enabled;
        if(!settings.config.look_only && ready && last_spring_target!=spring_target) {
            poll_stock_camera(true);last_spring_target=spring_target;
        }
        const bool camera_interrupted=take_camera_suspension();
        tick_stick_look(settings.config,controller,comfort,input_route.stick_blocked || native_camera || camera_interrupted || !stock_first_person_active() || comfort.suppressed);
        const bool spectator_switch=settings.config.spectator_first_person && spectator_available() && game_camera_suspended();
        const bool pressed=toggle.update(settings.config.toggle,controller,spectator_switch ? toggle_filter_route.toggle_blocked:input_route.toggle_blocked);
        if(pressed) log("Controller First Person chord detected.");
        const bool script_toggle=take_toggle_request();
        const bool spectator_toggle=(pressed || script_toggle) && spectator_switch && !toggle_filter_route.toggle_blocked;
        if(spectator_toggle)toggle_spectator_view();
        const bool glance_blocked=blocked || movement.body_flip_camera_owned || bail || native_camera || camera_interrupted || !stock_first_person_active() || comfort_snapshot().suppressed;
        // Camera readiness can briefly drop while the queued toggle executes.
        // Its emote suppression belongs to live gameplay, not camera readiness.
        publish_glance_filter(input_route.glance,!glance_blocked,current_owner,controller.device,
            emote_toggles.companion,emote_toggles.enabled(),context,emote_toggles.native);
        const bool emote=glance_emote_active();
        if(emote) {look.centre_for_emote(15);marker_glance.clear();}
        const auto shared_modifier=settings.config.board.enabled && movement.mode==2 ? settings.config.board.modifier:0;
        auto angles=look.update(input_route.glance,controller,glance_blocked,settings.config.toggle_glances,shared_modifier);
        if(bail && !returning_to_marker) marker_glance.clear();
        const int remembered=marker_glance.take(!glance_blocked && !returning_to_marker && !marker.pending);
        if(remembered>=0 && look.restore_selection(remembered)) {
            angles=look.selection_angles();log("Return to marker: restored toggled glance.");
        }
        set_stock_glance(camera,saved_game_fov,angles[0],angles[1]);
        // Every activation uses stock's existing request queue. This DLL never
        // switches modes, takes a camera lease, hides a head or restores a view.
        if(ready) {
            if(const auto request=take_enabled_command()) {
                request_user_first_person(*request);
            } else if((pressed || script_toggle) && !spectator_switch && !blocked) {
                const bool enabled=!(comfort.initialized ? comfort.master:stock_first_person_requested());
                request_user_first_person(enabled);
            }
        }
        if(take_reload_request()) { Command retry;retry.kind=CommandKind::retry_save;submit_command(retry); }
    } catch(const SourceGuard& error) { look.reset();marker_glance.clear();clear_glance_filter();stop_stick_look();clear_comfort_state();set_stock_glance(0,0,0,0);if(last_wait!=error.message) { last_wait=error.message;log(std::string("Controller input waiting: ")+error.message); } }
      catch(const std::exception& error) { look.reset();marker_glance.clear();clear_glance_filter();stop_stick_look();clear_comfort_state();set_stock_glance(0,0,0,0);log(std::string("Controller input bridge: ")+error.what()); }
      catch(...) { look.reset();marker_glance.clear();clear_glance_filter();stop_stick_look();clear_comfort_state();set_stock_glance(0,0,0,0); }
}
}
