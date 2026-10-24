#include "stock_bridge.h"
#include "stock_contract.h"
#include "native_211.h"
#include "bridge_resolver.h"
#include "comfort_policy.h"
#include "camera_cycle_override.h"
#include "game_camera_ownership.h"
#include "spectator_runtime.h"
#include "settings.h"
#include "stick_runtime.h"
#include "camera_policy.h"
#include "owned_hooks.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/image_identity.h"
#include <cstring>
#include <mutex>
#include <atomic>

namespace jishsfirstpersonmod {
namespace {
using namespace stock_contract;
using ReadModel=void(*)(void*,void*);
using QueueDebug=bool(*)(void*,const Request&,char*,std::size_t);
using QueueConsole=bool(*)(void*,const char*,char*,std::size_t);
ReadModel original_read{};
QueueDebug original_queue{};
QueueConsole original_console{};
void* callback_user{};
std::mutex mirror_mutex;
CameraMirror mirror;
RuntimeSpringSwitch spring_switch;
std::mutex spring_mutex;
std::atomic<bool> spring_managed{};
std::atomic<int> native_spring{-1};
bool initial_stock_sync{};
std::atomic<bool> bridge_ready{},native_active{},native_ready{};
std::atomic<int> activation_intent{-1};
std::atomic<bool> native_view_observed{};
std::atomic<ULONGLONG> intent_time{};
void* stock_state{};
using NativeMutex=int(*)(void*);
NativeMutex lock_stock{},unlock_stock{};
std::string native_string(std::uintptr_t address) {
    using namespace dingosdk;
    std::size_t size{},capacity{};std::uintptr_t pointer{};
    if(!memory::read(address+16,size) || !memory::read(address+24,capacity) || size>4096 || capacity<size) return {};
    if(capacity<16) pointer=address; else if(!memory::read(address,pointer)) return {};
    std::string text(size,'\0');
    return memory::read_bytes(pointer,text.data(),size) ? text:std::string{};
}
NativeControls read_native_controls(void* model) {
    NativeControls native;
    if(!is_v2()) return native;
    native.active=true;
    const auto address=reinterpret_cast<std::uintptr_t>(model);
    std::array<std::uint8_t,3> flags{};
    if(dingosdk::memory::read(address+model_debug_offset+16+44,flags) &&
       flags[0]<=1 && flags[1]<=1 && flags[2]<=1) {
        native.stabilize=flags[0]!=0;native.follow_flips=flags[1]!=0;native.board_only=flags[2]!=0;
    }
    if(uses_v206_layout(selected)) {
        std::uint8_t cycle{};
        if(dingosdk::memory::read(address+model_debug_offset+16+47,cycle) && cycle<=1) {
            native.camera_cycle_available=true;native.camera_cycle=cycle!=0;
        }
    }
    std::array<float,4> look{};
    if(dingosdk::memory::read(address+model_debug_offset+16+48,look) &&
        std::all_of(look.begin(),look.end(),[](float value){return std::isfinite(value) && value>=0 && value<=100;})) {
        native.smoothing=look[0];native.head_pitch=look[1];native.head_roll=look[2];native.bob=look[3];
    }
    std::uint8_t available{};
    const auto bindings_offset=bindings_model_offset();
    if(dingosdk::memory::read(address+bindings_offset,available) && available==1)
        dingosdk::memory::read(address+bindings_offset+4,native.bindings);
    return native;
}
bool queue_one(const Request& request,std::string& error) {
    char result[512]{};
    if(original_queue && original_queue(callback_user,to_native(request),result,sizeof(result))) return true;
    error=result[0] ? result:"Stock settings queue unavailable."; return false;
}
bool send_camera(const Config& before,const Config& after,std::string& error) {
    if(after.look_only) return true;
    { std::lock_guard lock(mirror_mutex); mirror.requested(before,after,GetTickCount64()); }
    bool accepted=true;
    auto send=[&](int action,float value,bool enabled=false) {
        if(!queue_one({action,enabled,value},error)) accepted=false;
    };
    // Stock's original FOV setter rejects zero. Zero remains the DLL-owned
    // game-FOV mode, reflected into both panels by read_model below.
    if(before.fov!=after.fov && after.fov>0) send(fov,after.fov);
    if(before.spring.enabled!=after.spring.enabled) {
        const auto effective=effective_camera_config(after,stock_first_person_requested() && !game_camera_suspended(),comfort_snapshot(),live_camera_movement());
        send(spring,0,is_v2() ? after.spring.enabled:effective.spring.enabled);
    }
    for(unsigned i=0;i<3;++i) {
        if(before.spring.offset[i]!=after.spring.offset[i]) send(offset_x+static_cast<int>(i),after.spring.offset[i]);
        if(before.spring.rotation[i]!=after.spring.rotation[i]) send(pitch+static_cast<int>(i),after.spring.rotation[i]);
    }
    const float a[]{before.spring.up,before.spring.down,before.spring.left,before.spring.right};
    const float b[]{after.spring.up,after.spring.down,after.spring.left,after.spring.right};
    for(unsigned i=0;i<4;++i) if(a[i]!=b[i]) send(up+static_cast<int>(i),b[i]);
    return accepted;
}
CameraCycleRequests cycle_requests;
bool remember_cycle_preference(bool pending,bool enabled) {
    Command command;command.kind=CommandKind::cycle_restore;command.action=pending ? 1:0;command.enabled=enabled;
    return submit_command(command);
}
void update_cycle_override(bool observed) {
    if(!uses_v206_layout(selected)) return;
    auto config=menu_snapshot().config;
    if(config.third_person_camera_switching() && !config.cycle_restore_pending) {
        if(!remember_cycle_preference(true,observed)) return;
        config=menu_snapshot().config;
    }
    if(!config.cycle_restore_pending) return;
    const bool target=config.third_person_camera_switching() ? false:config.cycle_restore_enabled;
    if(cycle_requests.settled(observed,target)) {
        if(!config.third_person_camera_switching()) remember_cycle_preference(false,config.cycle_restore_enabled);
        return;
    }
    if(!cycle_requests.attempt(target,GetTickCount64())) return;
    std::string error;
    if(queue_one({33,target,0},error)) {
        cycle_requests.accepted(target);
        log(target ? "Comfort restored First Person In Camera Cycle preference.":"Comfort disabled First Person In Camera Cycle while automatic third-person bail/recovery switching is enabled.");
    }
    else log("Camera-cycle comfort override request failed: "+error);
}
bool queue_hook(void* user,const Request& wire,char* result,std::size_t size) {
    if(compatibility::enabled() && uses_v206_layout(selected) && wire.action==33) {
        const auto config=menu_snapshot().config;
        if(config.third_person_camera_switching()) {
            if(!remember_cycle_preference(true,wire.enabled)) return false;
            auto effective=wire;effective.enabled=false;
            const bool accepted=original_queue(user,effective,result,size);
            if(accepted) cycle_requests.accepted(false);
            if(accepted && result && size) strncpy_s(result,size,"First Person In Camera Cycle stays off while automatic third-person switching is on.",_TRUNCATE);
            return accepted;
        }
        const bool accepted=original_queue(user,wire,result,size);
        if(accepted) cycle_requests.accepted(wire.enabled);
        if(accepted && config.cycle_restore_pending) remember_cycle_preference(false,wire.enabled);
        return accepted;
    }
    Request request;
    if(!compatibility::enabled() || !from_native(wire,request)) return original_queue(user,wire,result,size);
    if(request.action==first_person) {
        auto effective=request;effective.enabled=comfort_requested_view(request.enabled);
        activation_intent=effective.enabled ? 1:0;intent_time=GetTickCount64();
        const bool accepted=original_queue(user,to_native(effective),result,size);
        if(accepted) comfort_user_request(request.enabled);
        if(!accepted) activation_intent=-1;
        return accepted;
    }
    if(request.action==0 || request.action==3 || request.action==4) {
        const bool accepted=original_queue(user,to_native(request),result,size);
        if(accepted) { activation_intent=0;intent_time=GetTickCount64(); comfort_user_request(false); }
        return accepted;
    }
    const auto before=menu_snapshot().config;
    if(before.look_only) return original_queue(user,wire,result,size);
    auto config=before;
    if(apply(config,request)) {
        Command command; command.kind=CommandKind::camera_settings; command.spring=config.spring; command.value=config.fov;
        if(!submit_stock_command(command)) return false;
        { std::lock_guard lock(mirror_mutex); mirror.requested(before,config,GetTickCount64()); }
        if(request.action==fov && request.value==0) {
            if(result && size) strncpy_s(result,size,"First person uses game FOV.",_TRUNCATE);
            return true;
        }
    }
    auto effective=request;
    if(request.action==spring && !is_v2()) effective.enabled=effective_camera_config(config,stock_first_person_requested() && !game_camera_suspended(),comfort_snapshot(),live_camera_movement()).spring.enabled;
    return original_queue(user,to_native(effective),result,size);
}
bool console_hook(void* user,const char* command,char* result,std::size_t size) {
    // The native hotkey runs this console command against actual view state.
    // Toggle master intent instead, including while comfort/ownership has
    // temporarily released the camera. Explicit on/off and other commands pass.
    if(compatibility::enabled() && command && native_first_person_toggle(command)) {
        if(menu_snapshot().config.spectator_first_person && spectator_available() && game_camera_suspended()) {
            const bool accepted=toggle_spectator_view();
            if(result && size)strncpy_s(result,size,accepted ? "Spectator view changed.":spectator_status().c_str(),_TRUNCATE);
            return accepted;
        }
        const auto state=comfort_snapshot();
        const bool master=state.initialized ? state.master:stock_first_person_requested();
        const bool accepted=request_user_first_person(!master);
        if(result && size) {
            if(accepted) result[0]=0;
            else strncpy_s(result,size,"Could not change view. Check ReSkate's menu for details.",_TRUNCATE);
        }
        return accepted;
    }
    return original_console(user,command,result,size);
}
void read_hook(void* user,void* model) {
    // Our display-only edits must not become the next partial native snapshot.
    // Invalidate only DebugModel; leave every other stock revision intact.
    if(uses_v211_layout(selected) && model) {
        native211::refresh_debug(model);
    }
    original_read(user,model);
    if(!compatibility::enabled()) return;
    // A concurrent poll must never wait for this lock while holding stock's mutex.
    std::unique_lock spring_lock(spring_mutex,std::try_to_lock);
    auto* debug=reinterpret_cast<DebugPrefix*>(static_cast<std::byte*>(model)+model_debug_offset);
    observe_stock_camera_owner(debug->free_camera || (!uses_v212_layout(selected) && debug->drone_follow));
    // No STL object crosses DLL ownership. Only the verified POD prefix is read
    // and updated in the stock-owned model, after its normal callback returns.
    publish_native_controls(read_native_controls(model));
    auto snapshot=menu_snapshot();
    const bool previous_active=native_active.exchange(debug->first_person);
    const bool previously_observed=native_view_observed.exchange(true);
    native_ready=debug->camera_available;
    std::uint8_t cycle{};
    const bool cycle_valid=uses_v206_layout(selected) && dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(debug)+16+47,cycle) && cycle<=1;
    const auto comfort_before=comfort_snapshot();
    if(native_camera_cycle_choice(cycle==1 && !snapshot.config.third_person_camera_switching(),previously_observed,previous_active,debug->first_person,
        debug->camera_available,activation_intent.load()>=0,comfort_before.suppressed,
        game_camera_suspended() || game_camera_managing())) {
        comfort_user_request(debug->first_person);
        log(debug->first_person ? "Native camera cycle entered First Person.":"Native camera cycle left First Person.");
    }
    if(cycle_valid) update_cycle_override(cycle!=0);
    comfort_observe_native(debug->first_person);
    const auto intent=activation_intent.load();
    if(intent>=0 && (debug->first_person==(intent!=0) || GetTickCount64()-intent_time.load()>3000)) activation_intent=-1;
    auto message=native_string(reinterpret_cast<std::uintptr_t>(debug)+(debug->camera_available ? debug_status_offset():camera_issue_offset()));
    if(game_camera_suspended()) message=snapshot.config.spectator_first_person && spectator_available() ? spectator_status():game_camera_status();
    if(message.empty()) message=debug->camera_available ? "Camera ready.":"Waiting for ReSkate's camera.";
    publish_runtime(debug->first_person,debug->camera_available,message);
    if(snapshot.config.look_only) {
        const auto comfort=comfort_snapshot();
        if(snapshot.config.on_board_only || snapshot.config.native.board_only || snapshot.config.third_person_camera_switching() || game_camera_managing()) {
            if(comfort.initialized) debug->first_person=comfort.master;
            if((comfort.suppressed || game_camera_suspended()) && !native_active.load()) acknowledge_master(comfort.master);
        }
        return;
    }
    auto native=snapshot.config;native.fov=debug->fov;native.spring=debug->arm;
    const bool observed_spring=native.spring.enabled;native_spring=observed_spring ? 1:0;
    const auto movement=live_camera_movement();
    const auto camera_state=comfort_snapshot();
    const bool locked=!game_camera_suspended() && head_position_attached(snapshot.config,camera_state,movement);
    if(!is_v2() && (snapshot.config.disable_spring_on_foot || locked || spring_managed.load()))
        native.spring.enabled=snapshot.config.spring.enabled;
    Config merged;bool initial_sync{};
    if(dingosdk::first_person::valid(native.spring) && std::isfinite(native.fov) &&
       (native.fov==0 || (native.fov>=40 && native.fov<=120))) {
        {
            std::lock_guard lock(mirror_mutex);
            initial_sync=!initial_stock_sync && snapshot.camera_available;
            if(initial_sync) { initial_stock_sync=true;mirror.requested(native,snapshot.config,GetTickCount64()); }
            merged=mirror.reconcile(native,snapshot.config,GetTickCount64());
        }
        if(camera_values(merged)!=camera_values(snapshot.config)) {
            Command command;command.kind=CommandKind::camera_settings;command.spring=merged.spring;command.value=merged.fov;
            submit_stock_command(command);snapshot=menu_snapshot();
        }
        if(initial_sync) { std::string error; if(!send_camera(native,snapshot.config,error)) log("Initial stock camera settings sync: "+error); }
    }
    const auto effective=effective_camera_config(snapshot.config,stock_first_person_requested() && !game_camera_suspended(),camera_state,movement);
    if(spring_lock.owns_lock() && !is_v2()) {
        const auto target=spring_switch.update(snapshot.config.disable_spring_on_foot || locked,
            effective.spring.enabled,observed_spring,GetTickCount64());
        spring_managed=spring_switch.protects_preference(false);
        if(target) {
            std::string error;
            const bool accepted=queue_one({spring,*target,0},error);
            spring_switch.queued(*target,accepted);
            if(!accepted) log("Automatic camera Spring update will retry: "+error);
        }
    }
    // Only the UI checkbox reflects master intent during comfort suppression.
    // The native camera state captured above and its ownership stay untouched.
    const auto comfort=comfort_snapshot();
    if(snapshot.config.on_board_only || snapshot.config.native.board_only || snapshot.config.third_person_camera_switching() || game_camera_managing()) {
        if(comfort.initialized) debug->first_person=comfort.master;
        if((comfort.suppressed || game_camera_suspended()) && !native_active.load()) acknowledge_master(comfort.master);
    }
    debug->fov=snapshot.config.fov;
    debug->arm=snapshot.config.spring;
}
bool owned_code(std::uintptr_t address,HMODULE module) {
    HMODULE owner{}; MEMORY_BASIC_INFORMATION page{};
    return address && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(address),&owner) && owner==module && VirtualQuery(reinterpret_cast<void*>(address),&page,sizeof(page)) &&
        (page.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));
}
}
bool start_stock_bridge(HMODULE module) {
    using namespace dingosdk;
    wchar_t path[32768]{};
    if(!GetModuleFileNameW(module,path,32768)) return false;
    const auto file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    std::array<unsigned char,32> hash{};
    const bool hashed=supported_build::file_sha256(file,hash);
    if(file!=INVALID_HANDLE_VALUE) CloseHandle(file);
    const auto base=reinterpret_cast<std::uintptr_t>(module);
    const auto status=reinterpret_cast<std::uintptr_t>(GetProcAddress(module,"DingoSDKOverlayGetStatus"));
    const auto* profile=selected;
    if(!hashed || !profile || status<base || status-base!=profile->status_rva) {
        log("Stock camera settings sync unavailable: installed ReSkate DLL does not match the verified bridge contract."); return false;
    }
    log(std::string("Verified native ReSkate bridge profile selected: ")+profile->version);
    std::uintptr_t state{}; std::array<std::uintptr_t,6> callbacks{};
    if(!memory::read(base+contract().state_pointer_rva,state) || !state || !memory::read(state+callbacks_offset(),callbacks) ||
        !owned_code(callbacks[1],module) || !owned_code(callbacks[3],module)) {
        log("ERROR: stock camera settings callback contract validation failed.");return false;
    }
    if(!compatibility::validate_callbacks(module,callbacks[1],callbacks[3])) {
        log("ERROR: stock read-model/queue callback implementations differ.");return false;
    }
    if(is_v2()) {
        constexpr std::array<unsigned char,28> console_prefix{
            0x48,0x89,0x5c,0x24,0x08,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,
            0x48,0x8d,0x6c,0x24,0xd9,0x48,0x81,0xec,0xd0,0x00,0x00,0x00};
        std::array<unsigned char,28> live{};
        if(callbacks[5]!=base+stock_contract::console_queue_rva() || !owned_code(callbacks[5],module) ||
           !memory::read(callbacks[5],live) || live!=console_prefix) {
            log("ERROR: native console queue contract differs; hotkey coordination unavailable.");return false;
        }
    }
    // Sample the live model while holding the validated recursive mutex.
    std::array<unsigned char,80> mutex_bytes{};MEMORY_BASIC_INFORMATION mutex_page{};
    if(!memory::read(state,mutex_bytes) || !VirtualQuery(reinterpret_cast<void*>(state),&mutex_page,sizeof(mutex_page)) ||
       mutex_page.State!=MEM_COMMIT || (mutex_page.Protect&(PAGE_GUARD|PAGE_NOACCESS)) ||
       !(mutex_page.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))) {
        log("ERROR: stock live mutex allocation is unreadable or not writable.");return false;
    }
    auto lock_fn=reinterpret_cast<NativeMutex>(base+contract().mutex_lock_rva);
    auto unlock_fn=reinterpret_cast<NativeMutex>(base+contract().mutex_unlock_rva);
    if(lock_fn(reinterpret_cast<void*>(state))!=0) { log("ERROR: stock model mutex lock failed.");return false; }
    DebugPrefix prefix{};std::array<unsigned char,sizeof(DebugPrefix)> prefix_bytes{};
    const auto prefix_address=state+model_offset()+model_debug_offset;
    bool flags_valid=memory::read(prefix_address,prefix_bytes);
    for(unsigned i=0;i<5;++i) flags_valid=flags_valid && prefix_bytes[i]<=1;
    flags_valid=flags_valid && prefix_bytes[16]<=1;
    const bool model_valid=flags_valid && memory::read(prefix_address,prefix) &&
        dingosdk::first_person::valid(prefix.arm) && std::isfinite(prefix.fov) && std::isfinite(prefix.free_fov);
    const bool callbacks_stable=memory::read(state+callbacks_offset(),callbacks) &&
        compatibility::validate_callbacks(module,callbacks[1],callbacks[3]) &&
        (!stock_contract::is_v2() || callbacks[5]==base+stock_contract::console_queue_rva());
    const auto native_controls=read_native_controls(reinterpret_cast<void*>(state+model_offset()));
    unlock_fn(reinterpret_cast<void*>(state));
    if(!model_valid || !callbacks_stable) { log("ERROR: stock live model prefix or callbacks differ.");return false; }
    callback_user=reinterpret_cast<void*>(callbacks[0]);
    stock_state=reinterpret_cast<void*>(state);
    lock_stock=reinterpret_cast<NativeMutex>(base+contract().mutex_lock_rva);
    unlock_stock=reinterpret_cast<NativeMutex>(base+contract().mutex_unlock_rva);
    const auto initialized=hook_initialize();
    if(initialized!=HookOk && initialized!=HookAlreadyInitialized) return false;
    void* read{}; void* queue{};
    if(owned_hook_prepare(reinterpret_cast<void*>(callbacks[1]),reinterpret_cast<void*>(&read_hook),&read)!=HookOk ||
       owned_hook_prepare(reinterpret_cast<void*>(callbacks[3]),reinterpret_cast<void*>(&queue_hook),&queue)!=HookOk) return false;
    original_read=reinterpret_cast<ReadModel>(read);original_queue=reinterpret_cast<QueueDebug>(queue);
    if(is_v2()) {
        void* console{};
        if(owned_hook_prepare(reinterpret_cast<void*>(callbacks[5]),reinterpret_cast<void*>(&console_hook),&console)!=HookOk) return false;
        original_console=reinterpret_cast<QueueConsole>(console);
        if(hook_queue_enable(reinterpret_cast<void*>(callbacks[5]))!=HookOk) return false;
    }
    if(hook_queue_enable(reinterpret_cast<void*>(callbacks[1]))!=HookOk || hook_queue_enable(reinterpret_cast<void*>(callbacks[3]))!=HookOk || apply_owned_hooks()!=HookOk) {
        log("ERROR: stock camera settings bridge hook transaction failed.");return false;
    }
    set_camera_sink(send_camera);
    publish_native_controls(native_controls);
    bridge_ready=true;
    set_activation_sink(request_user_first_person);
    log("Stock camera bridge active: native ReSkate owns activation, spring, pose hooks and restoration.");return true;
}
bool stock_bridge_ready() { return bridge_ready.load(); }
void stop_stock_bridge() { bridge_ready=false;native_active=false;native_ready=false;set_camera_sink(nullptr);set_activation_sink(nullptr); }
bool stock_first_person_active() { return native_active.load(); }
bool stock_camera_available() { return native_ready.load(); }
bool stock_first_person_requested() {
    const auto intent=activation_intent.load();return intent<0 ? native_active.load():intent!=0;
}
bool request_user_first_person(bool enabled) {
    const bool accepted=request_stock_first_person(comfort_requested_view(enabled));
    if(accepted) comfort_user_request(enabled);
    return accepted;
}
bool request_stock_first_person(bool enabled) {
    if(!bridge_ready) return false;
    enabled=enabled && !game_camera_suspended();
    activation_intent=enabled ? 1:0;intent_time=GetTickCount64();
    std::string error;const bool accepted=queue_one({first_person,enabled,0},error);
    if(!accepted) { activation_intent=-1;log("Native First Person request rejected: "+error); }
    else log(enabled ? "Native ReSkate First Person enable queued":"Native ReSkate First Person disable queued");
    return accepted;
}
int stock_camera_spring_enabled() { return native_spring.load(); }
void poll_stock_camera(bool force) {
    if(!bridge_ready) return;
    static std::atomic<ULONGLONG> next{};
    const auto now=GetTickCount64();
    auto expected=next.load();
    if(!force && (now<expected || !next.compare_exchange_strong(expected,now+50))) return;
    if(force) next=now+50;
    // Reuse the stock-owned model under its own recursive render mutex. This
    // keeps native readiness/current view fresh even with both menus closed.
    if(lock_stock(stock_state)!=0) return;
    struct Unlock { ~Unlock() { unlock_stock(stock_state); } } unlock;
    try { read_hook(callback_user,static_cast<std::byte*>(stock_state)+model_offset()); }
    catch(const std::exception& error) { log(std::string("Native camera status refresh failed: ")+error.what()); }
    catch(...) { log("Native camera status refresh failed."); }
}
}
