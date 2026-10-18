#include "menu.h"
#include "native_210.h"
#include "owned_hooks.h"
#include <intrin.h>
#include <mutex>
#include <map>
#include <deque>
#include <memory>
#include <windowsx.h>
#include <stdexcept>

namespace jishsfirstpersonmod {
bool PointerRouter::route(UINT message,WPARAM key,int x,int y) noexcept {
    if(!visible) { dragging=keyboard=false; return false; }
    if(message==WM_KEYDOWN || message==WM_KEYUP || message==WM_SYSKEYDOWN || message==WM_SYSKEYUP) {
        if(key==VK_INSERT || key==VK_ESCAPE || message==WM_SYSKEYDOWN || message==WM_SYSKEYUP) return false;
        return keyboard;
    }
    if(message==WM_CHAR) return keyboard;
    if(message==WM_KILLFOCUS || message==WM_ACTIVATEAPP) { dragging=keyboard=false; return false; }
    const bool inside=bounds.contains(x,y);
    if(message==WM_LBUTTONDOWN || message==WM_RBUTTONDOWN || message==WM_MBUTTONDOWN) {
        keyboard=inside; dragging=inside; return inside;
    }
    if(message==WM_LBUTTONUP || message==WM_RBUTTONUP || message==WM_MBUTTONUP) {
        const auto own=inside||dragging; dragging=false; return own;
    }
    if(message==WM_MOUSEWHEEL || message==WM_MOUSEHWHEEL) return inside||dragging;
    if(message==WM_INPUT) return inside||dragging;
    // Both menus keep the same pointer location, but clicks in this panel never
    // reach stock widgets beneath it. Insert/Escape always reach stock.
    return false;
}
namespace {
struct WindowLink { WNDPROC previous{}; };
struct InputState {
    std::mutex mutex;
    std::map<HWND,std::unique_ptr<WindowLink>> links;
    std::deque<MenuInput> pending;
    PointerRouter router;
    HWND selected{};
    bool text_input{};
    bool binding_recording{};MenuControllerState controller;MenuControllerResult controller_frame;
    MenuHotkey hotkey;
};
InputState& state() { static auto* value=new InputState; return *value; }
using AsyncKey=SHORT(WINAPI*)(int);
AsyncKey original_async_key{};
bool owns_mouse() {
    auto& input=state(); std::lock_guard lock(input.mutex);
    if(!input.router.visible || !input.selected || GetAncestor(input.selected,GA_ROOT)!=GetForegroundWindow()) return false;
    CURSORINFO cursor{sizeof(CURSORINFO)};
    if(!GetCursorInfo(&cursor)) return input.router.dragging;
    auto point=cursor.ptScreenPos;
    return ScreenToClient(input.selected,&point) && (input.router.bounds.contains(point.x,point.y)||input.router.dragging);
}
SHORT WINAPI panel_async_key(int key) {
    const auto caller=_ReturnAddress();
    const auto result=original_async_key(key);
    const auto error=GetLastError();
    HMODULE owner{};
    const bool mouse=key==VK_LBUTTON || key==VK_RBUTTON || key==VK_MBUTTON || key==VK_XBUTTON1 || key==VK_XBUTTON2;
    bool keyboard{};
    {auto& input=state();std::lock_guard lock(input.mutex);
     keyboard=key!=VK_INSERT && key!=VK_ESCAPE && input.router.visible && input.router.keyboard && input.selected &&
        GetAncestor(input.selected,GA_ROOT)==GetForegroundWindow();}
    const bool block=(mouse ? owns_mouse():keyboard) && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(caller),&owner) && owner==GetModuleHandleW(L"reskate.dll");
    SetLastError(error);
    return block ? 0:result;
}
LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    auto& input=state(); WNDPROC previous{}; bool swallow{},hotkey_owned{};
    std::optional<unsigned> recorded;
    {
        std::lock_guard lock(input.mutex);
        const auto found=input.links.find(window);
        if(found!=input.links.end()) previous=found->second->previous;
        if(window==input.selected) {
            const bool focused=GetForegroundWindow()==GetAncestor(window,GA_ROOT);
            if(!focused || message==WM_KILLFOCUS || (message==WM_ACTIVATEAPP && !wp)) input.hotkey.recording=false;
            if(focused && (message==WM_KEYDOWN || message==WM_SYSKEYDOWN || message==WM_KEYUP || message==WM_SYSKEYUP)) {
                const bool was_recording=input.hotkey.recording;
                swallow=input.hotkey.key(message,static_cast<unsigned>(wp),(lp&(1LL<<30))!=0);
                recorded=input.hotkey.recorded;input.hotkey.recorded.reset();
                const auto config=menu_snapshot().config;
                const auto key=VK_F9;
                if(!was_recording && wp==VK_INSERT && (message==WM_KEYDOWN || message==WM_SYSKEYDOWN) && !(lp&(1LL<<30))) input.hotkey.hidden=false;
                if(!was_recording && !input.text_input && input.hotkey.toggle(message,static_cast<unsigned>(wp),key,(lp&(1LL<<30))!=0)) {
                    swallow=true;
                    if(input.hotkey.hidden) { input.router={};input.pending.clear(); }
                }
                hotkey_owned=swallow;
            }
            POINT pointer{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
            if(message==WM_MOUSEWHEEL || message==WM_MOUSEHWHEEL) ScreenToClient(window,&pointer);
            if(message==WM_INPUT) {
                CURSORINFO cursor{sizeof(CURSORINFO)};
                if(GetCursorInfo(&cursor)) { pointer=cursor.ptScreenPos; ScreenToClient(window,&pointer); }
            }
            swallow=swallow || input.router.route(message,wp,pointer.x,pointer.y);
            const bool relevant=(message>=WM_MOUSEFIRST && message<=WM_MOUSELAST) ||
                (message>=WM_KEYFIRST && message<=WM_KEYLAST) || message==WM_KILLFOCUS;
            if(input.router.visible && relevant && !hotkey_owned) {
                if(message==WM_MOUSEMOVE && !input.pending.empty() && input.pending.back().message==WM_MOUSEMOVE) input.pending.back()={message,wp,lp,window};
                else { if(input.pending.size()>=256) input.pending.pop_front(); input.pending.push_back({message,wp,lp,window}); }
            }
            if(message==WM_NCDESTROY) { input.router={}; input.selected=nullptr; input.pending.clear(); }
        }
    }
    if(recorded) { Command command;command.kind=CommandKind::menu_key;command.action=*recorded;submit_command(command); }
    if(swallow) return message==WM_INPUT ? DefWindowProcW(window,message,wp,lp):0;
    return previous ? CallWindowProcW(previous,window,message,wp,lp):DefWindowProcW(window,message,wp,lp);
}
}
bool install_panel_input_hooks() {
    using namespace dingosdk;
    void* original{};
    const auto prepared=owned_hook_prepare(reinterpret_cast<void*>(&GetAsyncKeyState),reinterpret_cast<void*>(&panel_async_key),&original);
    if(prepared!=HookOk) return false;
    original_async_key=reinterpret_cast<AsyncKey>(original);
    return hook_queue_enable(reinterpret_cast<void*>(&GetAsyncKeyState))==HookOk;
}
void route_panel_input(HWND window) {
    auto& input=state(); std::lock_guard lock(input.mutex);
    input.selected=window;
    if(input.links.contains(window)) return;
    auto link=std::make_unique<WindowLink>();
    link->previous=reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window,GWLP_WNDPROC));
    input.links.emplace(window,std::move(link));
    SetLastError(0);
    const auto old=SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(&procedure));
    if(!old && GetLastError()) { input.links.erase(window); throw std::runtime_error("Could not attach companion-menu input."); }
    if(old) input.links.at(window)->previous=reinterpret_cast<WNDPROC>(old);
}
void set_panel_bounds(PanelBounds bounds,bool visible,bool dragging,bool keyboard,bool text_input) {
    auto& input=state(); std::lock_guard lock(input.mutex);
    input.router.bounds=bounds; input.router.visible=visible;
    // The HWND receives down/up before ImGui's queued events are processed.
    // Keep its capture latch until the actual release; a render frame must not
    // erase a new down event before ImGui sees it.
    (void)dragging;
    if(!visible) input.router.dragging=false;
    input.router.keyboard=keyboard;input.text_input=visible && text_input;
    if(!visible) input.pending.clear();
}
std::vector<MenuInput> take_panel_input() {
    auto& input=state(); std::lock_guard lock(input.mutex);
    std::vector<MenuInput> result(input.pending.begin(),input.pending.end()); input.pending.clear(); return result;
}
void hide_panel() { set_panel_bounds({},false,false,false); }
void close_panel() {auto& input=state();{std::lock_guard lock(input.mutex);input.hotkey.hidden=true;}hide_panel();}
bool panel_gameplay_blocked() {auto& input=state();std::lock_guard lock(input.mutex);return input.router.visible;}
bool panel_recording_active() {
    auto& input=state();std::lock_guard lock(input.mutex);
    return input.router.visible && input.selected && GetAncestor(input.selected,GA_ROOT)==GetForegroundWindow() && input.binding_recording;
}
void panel_binding_recording(bool active) {auto& input=state();std::lock_guard lock(input.mutex);input.binding_recording=active;}
MenuControllerResult panel_controller_snapshot() {auto& input=state();std::lock_guard lock(input.mutex);return input.controller_frame;}
void poll_panel_controller() {
    const GameplayInput pad{};auto& input=state();std::lock_guard lock(input.mutex);
    const bool focused=input.selected && GetAncestor(input.selected,GA_ROOT)==GetForegroundWindow();
    static_assert(!native210::panel_controller_navigation);
    (void)pad;(void)focused;input.controller_frame={}; // Stock owns controller menu navigation.
}
bool panel_user_visible() {
    const auto config=menu_snapshot().config;
    // A hotkey claimed by ReSkate must not leave a previously hidden panel
    // inaccessible. Show it with the stock menu so the user can change the key.

    auto& input=state();std::lock_guard lock(input.mutex);return !input.hotkey.hidden;
}
void begin_menu_key_recording() { auto& input=state();std::lock_guard lock(input.mutex);input.hotkey.recording=true; }
void cancel_menu_key_recording() { auto& input=state();std::lock_guard lock(input.mutex);input.hotkey.recording=false; }
bool menu_key_recording() { auto& input=state();std::lock_guard lock(input.mutex);return input.hotkey.recording; }
std::string menu_key_label(unsigned key) {
    if(!key) return "Unbound";
    char name[128]{};const auto scan=MapVirtualKeyA(key,MAPVK_VK_TO_VSC);
    if(GetKeyNameTextA(static_cast<LONG>(scan<<16),name,sizeof(name))) return name;
    return "Key "+std::to_string(key);
}
}
