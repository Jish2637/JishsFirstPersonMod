#include "emote_aim_runtime.h"
#include "native.h"
#include "Extension/Skater/free_flight.h"
#include "comfort_policy.h"
#include "menu.h"
#include "stock_bridge.h"
#include "bridge_resolver.h"
#include "game_camera_ownership.h"
#include "player_state_contract.h"
#include "owned_hooks.h"
#include <atomic>
#include <mutex>
namespace jishsfirstpersonmod {
namespace {
using emote_aim::Matrix;
// The native wrapper tail-calls a camera service with a hidden matrix output in RCX.
// Preserve the complete integer return register from the chained implementation.
using Getter=std::uintptr_t(*)(Matrix*);
Getter original{};std::uintptr_t game{};bool installed{};
std::mutex guard;emote_aim::Frame frame;
std::atomic<std::uint64_t> calls{},overrides{},active_samples{},live_calls{},components{};
thread_local bool reentered{};
bool write_matrix(Matrix* out,const Matrix& value) noexcept {
    __try {std::memcpy(out,&value,sizeof(value));return true;}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
std::uintptr_t camera_hook(Matrix* output) {
    if(reentered) return original(output);
    reentered=true;
    const auto result=original(output);
    const auto error=GetLastError();
    ++calls;
    try {
        const auto config=menu_snapshot().config;
        if(config.stick.native_emote_aim && compatibility::enabled() && stock_first_person_active() &&
           stock_first_person_requested() && !game_camera_suspended() && !panel_gameplay_blocked()) {
            emote_aim::Frame view;
            {std::unique_lock lock(guard,std::try_to_lock);if(lock.owns_lock()) view=frame;}
            const auto state=comfort_snapshot();
            const emote_aim::Owner owner{state.core,state.entity,state.context,state.rig};
            bool visible=true;
            DWORD pid{};const auto foreground=GetForegroundWindow();
            const bool focused=foreground && GetWindowThreadProcessId(foreground,&pid) && pid==GetCurrentProcessId();
            const bool live=state.state_valid && state.animation_valid && state.normal_foot &&
                state.physics==player_state_contract::offboard && !state.bailing && !state.recovery &&
                !state.wipeout_request && !state.animation_wipeout && !state.suspended && focused &&
                overlay_available(visible) && !visible && view.fresh(owner,GetTickCount64(),GetCurrentThreadId());
            if(live) {
                ++live_calls;
                using namespace dingosdk::client_source::detail;
                SourceReader r;
                source_require(r.pointer(state.core)==game+player_state_contract::core_vtable &&
                    r.pointer(state.core,0x3c0)==state.context && r.pointer(state.core,0x438)==state.rig &&
                    r.pointer(state.rig)==state.context && r.pointer(state.rig,0x4630)==state.core &&
                    r.value<std::uint8_t>(state.entity,0x7e0)==0,"Emote owner changed");
                const auto parent=r.pointer(state.core,0x3b0);
                const bool ground=r.pointer(parent)==game+player_state_contract::offboard_vtable &&
                    r.pointer(parent,0x48)==parent+0x60 &&
                    r.pointer(parent+0x60)==game+player_state_contract::normal_foot_states[0].vtable &&
                    !(r.value<std::uint32_t>(state.context,0x13d4)&0x80);
                const auto component=emote_aim::component([&](auto a,void* p,auto n){return r.raw(a,p,n);},game,state.entity);
                if(component) ++components;
                const auto gesture=component ? r.value<std::uint32_t>(component,0x550):0;
                if(gesture) ++active_samples;
                Matrix native{};
                if(emote_aim::allowed(true,true,config.stick.enabled,config.look_only || !config.stick.turn_character,ground,gesture!=0) &&
                   output && dingosdk::memory::peek(reinterpret_cast<std::uintptr_t>(output),native) &&
                   dingosdk::valid_flight_transform(native) && dingosdk::valid_flight_transform(view.pose)) {
                    r.verify();
                    // Reject replacement of the gesture component before touching the temporary output.
                    if(component==emote_aim::component([&](auto a,void* p,auto n){return r.raw(a,p,n);},game,state.entity) &&
                       r.value<std::uint32_t>(component,0x550)==gesture &&
                       write_matrix(output,emote_aim::orientation_only(native,view.pose))) ++overrides;
                }
            }
        }
    } catch(...) {}
    reentered=false;SetLastError(error);return result;
}
}
void publish_emote_camera(const Matrix& pose,emote_aim::Owner owner,bool eligible) noexcept {
    static std::atomic<ULONGLONG> next_log{};
    try {
        const auto enabled=menu_snapshot().config.stick.native_emote_aim;
        {std::lock_guard lock(guard);frame={pose,owner,GetTickCount64(),GetCurrentThreadId(),installed && enabled && eligible};}
        const auto now=GetTickCount64();auto next=next_log.load();
        if(enabled && now>=next && next_log.compare_exchange_strong(next,now+5000)) {
            log("Experimental emote aim: hook="+std::to_string(installed)+" camera_ready="+std::to_string(eligible)+
                " getter_calls="+std::to_string(calls.load())+" live_calls="+std::to_string(live_calls.load())+" components="+std::to_string(components.load())+
                " native_gesture_samples="+std::to_string(active_samples.load())+
                " aim_overrides="+std::to_string(overrides.load()));
        }
    } catch(...) {}
}
void install_emote_aim(std::uintptr_t base) noexcept {
    try {
        const auto check=[&](std::uintptr_t at,const char* hex) {
            for(std::size_t i=0;hex[2*i];++i) {
                const auto nibble=[](char c){return c<='9'?c-'0':c-'a'+10;};
                unsigned char b{};if(!dingosdk::memory::read(base+at+i,b) || b!=nibble(hex[2*i])*16+nibble(hex[2*i+1])) return false;
            }return true;
        };
        if(!check(0x85eac0,"488bd14533c9488b0d7bd1d6064533c0488b0148ff6060") ||
           !check(0xf94c40,"83b950050000000f95c0c3") || !check(0xf90410,"8b8150050000c3") ||
           !check(0xf906f0,"488d05e9fd2e06c3")) {
            log("Experimental emote aim unavailable: native contracts differ.");return;
        }
        std::uintptr_t type{};
        if(!dingosdk::memory::read(base+0x6171ae0,type) || type!=base+0xf906f0) return;
        game=base;void* target=reinterpret_cast<void*>(base+0x85eac0);void* next{};
        auto status=dingosdk::owned_hook_prepare(target,reinterpret_cast<void*>(&camera_hook),&next);
        if(status==dingosdk::HookOk) {
            original=reinterpret_cast<Getter>(next);
            status=dingosdk::enable_owned_hook(target);
        }
        installed=status==dingosdk::HookOk;
        log(installed ? "Experimental emote aim hook ready; option defaults off. Local gesture gate and authored-camera use need in-game validation."
            :"Experimental emote aim unavailable: "+dingosdk::hook_failure_description(status));
    } catch(...) {log("Experimental emote aim initialization failed; option remains inactive.");}
}
}
