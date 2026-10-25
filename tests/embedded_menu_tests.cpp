#include "embedded_menu.h"
#include "embedded_context.h"
#include "embedded_policy.h"
#include "native_211.h"
#include <imgui_internal.h>
#include "comfort_policy.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory() { return std::filesystem::current_path(); }
std::filesystem::path data_directory() { return module_directory()/"embedded-test-data"; }
bool captured{};
void embedded_recording_state(bool active) {captured=active;}
ComfortSnapshot comfort_snapshot() { ComfortSnapshot s;s.initialized=true;s.board_available=true;s.bail_available=true;return s; }
bool stick_runtime_ready() { return true; }
bool glance_emote_filter_ready() { return true; }
bool bail_collision_available() noexcept { return true; }
bool game_camera_suspended() { return false; }
const char* game_camera_status() { return ""; }
bool spectator_ready=true;
bool spectator_available() noexcept { return spectator_ready; }
std::string spectator_status() { return spectator_ready ? "Spectator view: follow camera. Use the First Person binding to switch.":"First-person spectator view requires verified ReSkate 2.0.7."; }
}
void require(bool value,const char* why) {if(!value) throw std::runtime_error(why);}
namespace {
std::string fire,field_label;unsigned toggles{},sliders{};
void begin(void*,const char* id,const char*,const char*) {ImGui::PushID(id);ImGui::BeginGroup();}
void end() {ImGui::EndGroup();ImGui::PopID();}
void field(void*,const char* label,const char*) {field_label=label;ImGui::TextUnformatted(label);}
void note(const char* text) {ImGui::TextUnformatted(text);}
bool toggle(void*,const char* label,const char* help,bool* value,bool,const char*) {
 if(std::strcmp(label,"Native head movement")==0)
  if(!help || std::string(help).find("changes how the camera feels")==std::string::npos) throw std::runtime_error("native head tooltip missing feel explanation");
 ++toggles;ImGui::Checkbox(label,value);
 if(fire==label) {*value=!*value;fire.clear();return true;}return false;
}
bool slider(const char* id,float* value,float low,float high,const char* fmt,ImGuiSliderFlags flags) {
 ++sliders;ImGui::SliderFloat(id,value,low,high,fmt,flags);
 if(fire==field_label) {*value=(low+high)*.5f;fire.clear();return true;}return false;
}
void* allocate(size_t n,void*) {return std::malloc(n);}
void release(void* p,void*) {std::free(p);}
}
int main() {try {
 using namespace jishsfirstpersonmod;
 const auto isolated=std::filesystem::current_path()/"embedded-test-data";
 std::filesystem::create_directories(isolated);
 SetEnvironmentVariableW(L"LOCALAPPDATA",isolated.c_str());
 Config initial;initial.stick.enabled=true;initial.board.enabled=true;initial.toggle_glances=true;
 write_config(data_directory()/"settings.ini",initial);
 require(initialize_settings(),"settings startup");
 set_camera_sink([](const Config&,const Config&,std::string&) {return true;});
 set_activation_sink([](bool) {return true;});
 auto* previous=ImGui::CreateContext();
 ImGuiMemAllocFunc old_a{};ImGuiMemFreeFunc old_f{};void* old_u{};
 ImGui::GetAllocatorFunctions(&old_a,&old_f,&old_u);
 auto* host=ImGui::CreateContext();ImGui::SetCurrentContext(previous);
 EmbeddedWidgets widgets{begin,end,field,note,toggle,[](int,bool,float){return true;},slider};
 for(const char* action:{"","Enable First Person","Native head movement","Native emote aiming (experimental)","Right-stick look on foot","First person on board only","Horizontal sensitivity"}) {
  ScopedHostImGui scope(host,allocate,release,nullptr);
  auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={1000,800};io.DeltaTime=1.f/60;
  unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
  ImGui::NewFrame();ImGui::Begin("Host page");
  auto* window=ImGui::GetCurrentWindow();
  ImGui::PushID("first-person");
  for(const char* label:{"Mod updates","Advanced","View and stabilization","Automatic camera switching","Glances","Attachment and transitions","On-foot right stick look","On-board right stick look"}) {
   window->StateStorage.SetInt(window->GetID(label),1);
  }
  ImGui::PopID();
  fire=action;ImGuiErrorRecoveryState before{},after{};
  ImGui::ErrorRecoveryStoreState(&before);
  draw_embedded_page(reinterpret_cast<void*>(1),widgets,{true,0,1},true);
  ImGui::ErrorRecoveryStoreState(&after);
  require(std::memcmp(&before,&after,sizeof(before))==0,"unbalanced embedded scopes");
  require(fire.empty(),"embedded action was not routed");
  require(host->WithinFrameScope,"embedded page ended the host frame");
  require(host->NextWindowData.HasFlags==0,"embedded popup leaked next-window state");
  ImGui::End();ImGui::Render();
 }
 require(ImGui::GetCurrentContext()==previous,"context not restored");
 ImGuiMemAllocFunc a{};ImGuiMemFreeFunc f{};void* u{};ImGui::GetAllocatorFunctions(&a,&f,&u);
 require(a==old_a && f==old_f && u==old_u,"allocator not restored");
 try {ScopedHostImGui scope(host,allocate,release,nullptr);throw 7;}catch(int) {}
 require(ImGui::GetCurrentContext()==previous,"exception context restoration");
 require(toggles>20 && sliders>20,"advanced controls omitted");
 require(menu_snapshot().config.on_board_only,"comfort command route");
 require(menu_snapshot().config.stick.native_emote_aim,"emote option command route");
 require(menu_snapshot().config.stick.native_head_look,"native head option command route");
 require(menu_snapshot().config.stick.yaw_speed==360.5f,"native slider command route");
 require(save_personal_preset("Embedded"),"INI preset save");
 require(personal_presets()==std::vector<std::string>{"Embedded"},"preset refresh");
 require(load_personal_preset("Embedded"),"INI preset load");
 require(menu_snapshot().config.stick.native_emote_aim,"emote option lost through personal preset");
 require(menu_snapshot().config.stick.native_head_look,"native head option lost through personal preset");
 require(delete_personal_preset("Embedded") && personal_presets().empty(),"preset deletion");
 EmbeddedVisibility visible{true,false,true,true,3,1,100,90};
 require(visible.capture(true),"focused page capture");
 visible.tab=0;require(!visible.capture(true),"different page captures");visible.tab=1;
 visible.focused=false;require(!visible.capture(true),"unfocused captures");visible.focused=true;
 visible.menu=false;require(!visible.capture(true),"closed Insert captures");visible.menu=true;
 visible.failed=true;require(!visible.capture(true),"rejected contract captures");
 native211::RecordingCapture quarantine;
 require(quarantine.consume(false,true,true),"recording isolation");
 require(quarantine.consume(false,false,true),"release quarantine");
 require(!quarantine.consume(false,false,false),"navigation did not resume");
 require(!quarantine.consume(true,true,true),"raw recording blocked");
 {ScopedHostImGui scope(host,allocate,release,nullptr);ImGui::DestroyContext(host);}
 ImGui::DestroyContext(previous);
 std::cout<<"Embedded commands, presets, UI scopes, context/allocators and recording PASS\n"<<std::flush;
 ExitProcess(0);
 }catch(const std::exception& e) {std::cerr<<e.what()<<std::endl;ExitProcess(1);}}
