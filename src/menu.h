#pragma once
#include "settings.h"
#include "menu_controller.h"
#include <imgui.h>
#include <vector>

namespace jishsfirstpersonmod {
struct MenuHotkey {
    bool hidden{},recording{};
    std::optional<unsigned> recorded;
    bool toggle(UINT message,unsigned key,unsigned binding,bool repeat) {
        if(!binding || key!=binding) return false;
        if(message==WM_KEYDOWN || message==WM_SYSKEYDOWN) { if(!repeat) hidden=!hidden;return true; }
        return message==WM_KEYUP || message==WM_SYSKEYUP;
    }
    bool key(UINT message,unsigned key,bool repeat) {
        const bool down=message==WM_KEYDOWN || message==WM_SYSKEYDOWN;
        const bool up=message==WM_KEYUP || message==WM_SYSKEYUP;
        if(!down && !up) return false;
        if(recording) {
            if(!down || repeat) return true;
            if(key==VK_ESCAPE) { recording=false;return false; }
            if(key<=VK_XBUTTON2 || key==VK_SHIFT || key==VK_CONTROL || key==VK_MENU || (key>=VK_LSHIFT && key<=VK_RMENU)) return true;
            recorded=key;recording=false;return true;
        }
        return false;
    }
};
bool panel_user_visible();
bool panel_gameplay_blocked();
void close_panel();
void poll_panel_controller();
MenuControllerResult panel_controller_snapshot();
void panel_binding_recording(bool active);
bool panel_recording_active();
GameplayInput menu_controller();
void begin_menu_key_recording();
void cancel_menu_key_recording();
bool menu_key_recording();
std::string menu_key_label(unsigned key);
struct PanelBounds { int left{},top{},right{},bottom{}; bool contains(int x,int y) const noexcept { return x>=left && x<right && y>=top && y<bottom; } };
struct PointerRouter {
    PanelBounds bounds;
    bool visible{},dragging{},keyboard{};
    bool route(UINT message,WPARAM key,int x,int y) noexcept;
};
struct MenuInput { UINT message{}; WPARAM wparam{}; LPARAM lparam{}; HWND window{}; };
struct PanelFrame {
    HWND window{};
    ImVec2 display_size{1280,900};
    float delta_time=1.0f/60.0f,dpi=1;
    bool visible=true,focused=true;
    dingosdk::ControllerInput controller;
    MenuControllerResult navigation;
};
void route_panel_input(HWND window);
bool install_panel_input_hooks();
bool initialize_panel_recording();
void set_panel_bounds(PanelBounds bounds,bool visible,bool dragging,bool keyboard,bool text_input=false);
std::vector<MenuInput> take_panel_input();
void hide_panel();
void cancel_panel_recording();
void load_panel_fonts();
void draw_panel(HWND window,bool visible);
void draw_panel_frame(const PanelFrame& frame);
void publish_menu_error(const std::string& error);
}
