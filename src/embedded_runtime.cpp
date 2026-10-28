#include "embedded_menu.h"
#include "embedded_context.h"
#include "embedded_policy.h"
#include "embedded_contract.h"
#include "bridge_resolver.h"
#include "owned_hooks.h"
#include "spectator_contract.h"
#include "stock_bridge.h"
#include "Engine/Core/Platform/memory.h"
#include <imgui_internal.h>
#include <atomic>
#include <cmath>

namespace jishsfirstpersonmod {
namespace {
using Draw=void(*)(void*,const void*,const void*);
Draw original_draw{};
std::uintptr_t stock_base{},overlay_state{};
EmbeddedWidgets widgets{};
thread_local const void* frame_callbacks{};
bool request_native(int action,bool enabled,float value) {
    if(!frame_callbacks || action<26 || action>33 || !std::isfinite(value) || value<0 || value>100) return false;
    std::uintptr_t cb[6]{};
    if(!dingosdk::memory::read_bytes(reinterpret_cast<std::uintptr_t>(frame_callbacks),cb,sizeof(cb)) ||
       cb[3]!=stock_base+native211::queue.rva || cb[1]!=stock_base+native211::read.rva) return false;
    using Queue=bool(*)(void*,const stock_contract::Request&,char*,std::size_t);
    char feedback[512]{};
    const bool accepted=reinterpret_cast<Queue>(cb[3])(reinterpret_cast<void*>(cb[0]),{action,enabled,value},feedback,sizeof(feedback));
    if(!accepted) publish_menu_error(feedback[0] ? feedback:"Native camera option was rejected.");
    return accepted;
}
std::atomic<bool> recording{},ui_failed{},cancel_pending{};
std::atomic<ULONGLONG> last_draw{};
std::atomic<HWND> game_window{};
static_assert(IMGUI_VERSION_NUM==19191 && sizeof(ImGuiContext)==10576 && sizeof(ImGuiIO)==3032);
static_assert(sizeof(ImDrawVert)==20 && sizeof(ImDrawIdx)==2);

bool page_visible() {
    bool visible{};int page{},tab{};
    if(!compatibility::enabled() || ui_failed || !overlay_state || !overlay_available(visible) || !visible) return false;
    using namespace embedded211;
    if(!dingosdk::memory::read(overlay_state+state_menu+menu_page,page) ||
       !dingosdk::memory::read(overlay_state+state_menu+menu_tab,tab)) return false;
    return EmbeddedVisibility{true,false,visible,true,page,tab}.page_visible();
}
bool focused() {
    const auto window=game_window.load();return window && GetAncestor(window,GA_ROOT)==GetForegroundWindow();
}
void draw_hook(void* menu,const void* model,const void* callbacks) {
    if(!compatibility::enabled() || ui_failed) {original_draw(menu,model,callbacks);return;}
    using namespace embedded211;
    ImGuiContext* context{},*current{};ImGuiMemAllocFunc alloc{};ImGuiMemFreeFunc free{};void* data{};
    const auto read=dingosdk::memory::read<ImGuiContext*>;
    const bool valid=reinterpret_cast<std::uintptr_t>(menu)==overlay_state+state_menu &&
        read(overlay_state+state_context,context) && read(stock_base+context_slot,current) && context && context==current &&
        dingosdk::memory::read(stock_base+alloc_slot,alloc) && dingosdk::memory::read(stock_base+free_slot,free) &&
        dingosdk::memory::read(stock_base+userdata_slot,data) && alloc && free;
    if(!valid) {
        if(!ui_failed.exchange(true)) log("ERROR: embedded UI context/allocator contract rejected; stock First Person page retained.");
        recording=false;original_draw(menu,model,callbacks);return;
    }
    ScopedHostImGui scope(context,alloc,free,data);
    if(!context->WithinFrameScope || !context->CurrentWindow) {
        if(!ui_failed.exchange(true)) log("ERROR: embedded UI called outside a stock frame; stock First Person page retained.");
        recording=false;original_draw(menu,model,callbacks);return;
    }
    HWND window{};dingosdk::memory::read(overlay_state+state_window,window);game_window=window;
    const auto now=GetTickCount64();
    // No page/focus continuity means an old capture must not resume on return.
    if(cancel_pending.exchange(false) || !focused() || now-last_draw.load()>250) cancel_panel_recording();
    last_draw=now;
    ImGuiErrorRecoveryState before{};ImGui::ErrorRecoveryStoreState(&before);
    frame_callbacks=callbacks;
    struct CallbacksScope {~CallbacksScope(){frame_callbacks=nullptr;}} callbacks_scope;
    try { draw_embedded_page(menu,widgets,recording_controller(),focused()); }
    catch(...) {
        recording=false;ui_failed=true;
        // Recover only our scopes, then let the host finish its own frame.
        ImGui::ErrorRecoveryTryToRecoverState(&before);
        log("ERROR: embedded page failed; stock page will be used on following frames.");
    }
}
void polling_step() {
    if(!compatibility::enabled()) return;
    poll_stock_camera();
    if(!page_visible() || !focused() || GetTickCount64()-last_draw.load()>250) {recording=false;cancel_pending=true;}
}
DWORD WINAPI polling_worker(void*) {
    for(;;) {polling_step();Sleep(50);}
}
}
bool panel_gameplay_blocked() {bool visible{};return overlay_available(visible) && visible;}
bool panel_recording_active() {
    const bool active=EmbeddedVisibility{compatibility::enabled(),ui_failed,page_visible(),focused(),3,1,GetTickCount64(),last_draw}.capture(recording);
    if(recording && !active) {recording=false;cancel_pending=true;}
    return active;
}
void embedded_recording_state(bool active) {recording=active;}

bool start_menu() {
    // The worker has process lifetime: bootstrap pins this DLL before starting it.
    // It never accesses ImGui, and guarded polling remains active with Insert closed.
    const auto worker=CreateThread(nullptr,0,polling_worker,nullptr,0,nullptr);
    if(!worker) {log("ERROR: camera settings polling worker could not start.");return false;}
    CloseHandle(worker);
    const auto module=GetModuleHandleW(L"reskate.dll");stock_base=reinterpret_cast<std::uintptr_t>(module);
    const auto reject=[]() {log("ERROR: embedded UI contracts unavailable; stock First Person page retained. No companion renderer.");return true;};
    if(!stock_contract::uses_v211_layout(stock_contract::selected)) return reject();
    for(const auto& entry:embedded211::entries)
        if(!spectator::signature(dingosdk::memory::read_bytes,stock_base+entry.rva,entry.prefix)) return reject();
    if(!dingosdk::memory::read(stock_base+stock_contract::contract().state_pointer_rva,overlay_state) || !overlay_state) return reject();
    widgets={reinterpret_cast<decltype(widgets.begin_card)>(stock_base+embedded211::begin_card.rva),
        reinterpret_cast<decltype(widgets.end_card)>(stock_base+embedded211::end_card.rva),
        reinterpret_cast<decltype(widgets.field)>(stock_base+embedded211::field.rva),
        reinterpret_cast<decltype(widgets.note)>(stock_base+embedded211::note.rva),
        reinterpret_cast<decltype(widgets.toggle)>(stock_base+embedded211::toggle_row.rva),
        &request_native,
        reinterpret_cast<decltype(widgets.slider)>(stock_base+embedded211::slider_float.rva)};
    using namespace dingosdk;
    // Capture readers are prepared in the same transaction as the page hook.
    if(!initialize_panel_recording()) return false;
    void* previous{};const auto target=reinterpret_cast<void*>(stock_base+embedded211::draw.rva);
    if(owned_hook_prepare(target,reinterpret_cast<void*>(&draw_hook),&previous)!=HookOk) return false;
    original_draw=reinterpret_cast<Draw>(previous);
    if(hook_queue_enable(target)!=HookOk || apply_owned_hooks()!=HookOk) return false;
    log("Verified official 2.1.3 Insert > Skater > First Person page replacement active; stock owns UI rendering and navigation.");
    return true;
}
}
