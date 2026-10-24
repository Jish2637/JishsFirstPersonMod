#include "embedded_menu.h"
#include <imgui_internal.h>
#include "bridge_resolver.h"
#include <iostream>
#include <stdexcept>
#include <vector>
static ULONGLONG tick=100;static bool focus=true,menu_visible=false,enabled=true;
static unsigned polls{},draws{},originals{},cancels{};static bool throw_draw{};
static HWND WINAPI foreground() {return reinterpret_cast<HWND>(focus ? 1:2);}
static HWND WINAPI ancestor(HWND h,UINT) {return h;}
static ULONGLONG WINAPI tick_now() {return tick;}
#define GetForegroundWindow foreground
#define GetAncestor ancestor
#define GetTickCount64 tick_now
#include "../src/embedded_runtime.cpp"
#undef GetForegroundWindow
#undef GetAncestor
#undef GetTickCount64
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
void publish_menu_error(const std::string&) {}
void cancel_panel_recording() {++cancels;embedded_recording_state(false);}
void draw_embedded_page(void*,const EmbeddedWidgets&,dingosdk::ControllerInput,bool) {
 ++draws;
 if(throw_draw) {ImGui::PushID("unfinished");throw std::runtime_error("injected UI failure");}
 ImGui::TextUnformatted("Embedded mock controls");
}
dingosdk::ControllerInput recording_controller() {return {};}
bool initialize_panel_recording() {return true;}
void poll_stock_camera(bool) {++polls;}
bool overlay_available(bool& visible) noexcept {visible=menu_visible;return true;}
namespace compatibility {bool enabled(){return ::enabled;}}
}
void require(bool v,const char* message) {if(!v) throw std::runtime_error(message);}
int main(){try {
 using namespace jishsfirstpersonmod;
 // Exercise the real worker body with no UI/context and Insert closed.
 polling_step();require(polls==1 && draws==0,"camera refresh depends on open Insert");
 menu_visible=true;polling_step();require(polls==2,"open-menu poll");
 enabled=false;polling_step();require(polls==2,"disabled integration polled");enabled=true;
 std::vector<std::byte> state(70000),module(embedded211::userdata_slot+0x1000);
 overlay_state=reinterpret_cast<std::uintptr_t>(state.data());stock_base=reinterpret_cast<std::uintptr_t>(module.data());
 auto store=[](std::uintptr_t at,const auto& value){std::memcpy(reinterpret_cast<void*>(at),&value,sizeof(value));};
 store(overlay_state+embedded211::state_menu+embedded211::menu_page,3);
 store(overlay_state+embedded211::state_menu+embedded211::menu_tab,1);
 HWND window=reinterpret_cast<HWND>(1);store(overlay_state+embedded211::state_window,window);game_window=window;
 ui_failed=false;last_draw=tick;recording=true;
 require(panel_recording_active(),"embedded page capture missing");
 focus=false;require(!panel_recording_active() && cancel_pending,"focus loss resumes stale recorder");focus=true;
 cancel_pending=false;recording=true;store(overlay_state+embedded211::state_menu+embedded211::menu_tab,0);
 require(!panel_recording_active() && cancel_pending,"page exit resumes stale recorder");
 store(overlay_state+embedded211::state_menu+embedded211::menu_tab,1);
 original_draw=[](void*,const void*,const void*){++originals;};
 auto* local=ImGui::CreateContext();auto* host=ImGui::CreateContext();ImGui::SetCurrentContext(host);
 ImGuiMemAllocFunc alloc{};ImGuiMemFreeFunc free{};void* data{};ImGui::GetAllocatorFunctions(&alloc,&free,&data);
 store(overlay_state+embedded211::state_context,host);store(stock_base+embedded211::context_slot,host);
 store(stock_base+embedded211::alloc_slot,alloc);store(stock_base+embedded211::free_slot,free);store(stock_base+embedded211::userdata_slot,data);
 auto& io=ImGui::GetIO();io.DisplaySize={800,600};io.DeltaTime=1.f/60;io.IniFilename=nullptr;
 unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
 const auto frame=[&](bool fail) {
  ImGui::SetCurrentContext(host);ImGui::NewFrame();ImGui::Begin("Stock");
  auto* host_window=host->CurrentWindow;ImGui::SetCurrentContext(local);
  throw_draw=fail;draw_hook(reinterpret_cast<void*>(overlay_state+embedded211::state_menu),nullptr,nullptr);
  require(ImGui::GetCurrentContext()==local,"DLL context leaked");
  require(host->WithinFrameScope && host->CurrentWindow==host_window,"host frame/window ownership changed");
  ImGui::SetCurrentContext(host);ImGui::End();ImGui::Render();
 };
 io.AddKeyEvent(ImGuiKey_F9,true);
 ui_failed=false;frame(false);require(draws==1 && originals==0,"duplicate page rendering");
 ImGuiContext* wrong=nullptr;store(stock_base+embedded211::context_slot,wrong);
 frame(false);require(ui_failed && originals==1 && draws==1,"invalid context did not retain stock page");
 ui_failed=false;store(stock_base+embedded211::context_slot,host);
 host->IO.ConfigErrorRecoveryEnableAssert=false;host->IO.ConfigErrorRecoveryEnableDebugLog=false;host->IO.ConfigErrorRecoveryEnableTooltip=false;
 frame(true);require(ui_failed && draws==2 && originals==1,"exception recovery");
 frame(false);require(originals==2 && draws==2,"failed UI did not remain stock");
 ImGui::DestroyContext(host);ImGui::SetCurrentContext(local);ImGui::DestroyContext(local);
 std::cout<<"Real embedded hook/worker: closed-menu refresh, rejection, no duplicate draw, recovery and recording isolation PASS\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
