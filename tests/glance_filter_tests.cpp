#include "stick_runtime.h"
#include "stock_bridge.h"
#include "game_camera_ownership.h"
#include <Xinput.h>
#include <stdexcept>
#include <iostream>
#include <future>
ULONGLONG qa_now=10;bool qa_focus=true,qa_menu=false,qa_active=true,qa_spectator=false;bool qa_capture=false;bool qa_panel=false;std::uint32_t qa_board_modifier{};jishsfirstpersonmod::StickAxes qa_native_axes{};
void (*qa_overlay_probe)(){};
HWND qa_window(){return qa_focus?reinterpret_cast<HWND>(1):nullptr;}
DWORD qa_pid(HWND,DWORD* pid){*pid=GetCurrentProcessId();return 1;}
ULONGLONG qa_clock(){return qa_now;}
#define GetForegroundWindow qa_window
#define GetWindowThreadProcessId qa_pid
#define GetTickCount64 qa_clock
#include "../src/stick_input_filter.cpp"
#undef GetForegroundWindow
#undef GetWindowThreadProcessId
#undef GetTickCount64
namespace jishsfirstpersonmod {
thread_local bool raw_controller_read{};
void log(const std::string& text) noexcept {
    write_diagnostic_log(std::filesystem::temp_directory_path()/ (L"jish-startup-log-"+std::to_wstring(GetCurrentProcessId())),text);
}
bool panel_gameplay_blocked(){return qa_panel;}
bool stock_first_person_active(){return qa_active;}
bool stock_first_person_requested(){return qa_active;}
bool game_camera_suspended(){return qa_spectator;}
ComfortSnapshot comfort_snapshot(){ComfortSnapshot value;value.core=1;value.entity=2;value.context=3;value.rig=4;return value;}
bool overlay_available(bool& visible) noexcept{if(qa_overlay_probe)qa_overlay_probe();visible=qa_menu;return true;}
bool stick_captures_input(unsigned,std::uint32_t buttons){return qa_capture && (!qa_board_modifier || (buttons&qa_board_modifier)==qa_board_modifier);}
std::uint32_t stick_capture_mask(unsigned){return qa_capture ? (qa_board_modifier ? qa_board_modifier:~0u):0;}
StickInputPolicy stick_input_policy(unsigned slot){return {stick_capture_mask(slot),qa_native_axes};}
std::uintptr_t resolve_stock_handler(std::uintptr_t){return 0;}
}
using namespace jishsfirstpersonmod;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
XINPUT_STATE sample;
DWORD WINAPI input(DWORD,XINPUT_STATE* out){*out=sample;return ERROR_SUCCESS;}
unsigned hid_calls{};
void hid(const StockHidRead& read,DWORD n){++hid_calls;if(qa_menu&&read.buffer&&n)static_cast<unsigned char*>(read.buffer)[0]=0;}
#include "direct_input_adapter_tests.inl"
#include "native_hid_adapter_tests.inl"
#include "board_input_adapter_tests.inl"
int main(int argc,char** argv) try {
 std::cout<<std::unitbuf;
 emote_ready=true;originals[0]=input;original_hid=hid;
 dingosdk::FirstPersonLookControls bindings;for(unsigned i=0;i<4;++i)bindings[i].combo=0x100|(1u<<i);
 auto publish=[&](){publish_glance_filter(bindings,true,{1,2,3,4},1);};
 auto poll=[&](unsigned buttons,unsigned slot=0){sample.Gamepad.wButtons=buttons;publish();XINPUT_STATE out;SetLastError(123);check(filtered_state<0>(slot,&out)==ERROR_SUCCESS&&GetLastError()==123,"API result/error changed");return out.Gamepad.wButtons;};
 poll(0);qa_now=20;check(poll(0x1101)==0x1100,"unrelated button or LB consumed");
 qa_now=30;check(poll(0x1101)==0x1100&&!glance_emote_active(),"held first press duplicated");
 qa_now=40;poll(0x100);qa_now=50;check(poll(0x101)==0x101&&glance_emote_active(),"double tap not forwarded");
 qa_now=70;check(poll(0x101)==0x101&&glance_emote_active(),"second hold lost");qa_now=80;poll(0x100);check(!glance_emote_active(),"release stuck");
 clear_glance_filter();qa_now=100;poll(0x101,0);qa_now=110;poll(0x100,0);qa_now=120;check(poll(0x101,1)==0x100&&!glance_emote_active(),"different slots paired");
 for(unsigned interruption=0;interruption<4;++interruption){clear_glance_filter();qa_now+=100;poll(0x101);qa_now+=10;poll(0x100);
  if(interruption==0)qa_menu=true;if(interruption==1)qa_focus=false;if(interruption==2)qa_active=false;if(interruption==3)qa_spectator=true;
  qa_now+=10;check(poll(0x101)==0x101,"interrupted gameplay filtered");qa_menu=false;qa_focus=qa_active=true;qa_spectator=false;qa_now+=10;check(poll(0x101)==0x100&&!glance_emote_active(),"interruption retained tap");}
 clear_glance_filter();publish();raw_controller_read=true;check(poll(0x101)==0x101,"mod sampler was filtered");raw_controller_read=false;
 for(auto kind:{dingosdk::PlayStationPad::dualshock4,dingosdk::PlayStationPad::dualsense})for(unsigned length:{64u,78u,547u}){
  if(length==547 && kind!=dingosdk::PlayStationPad::dualshock4) continue;
  const bool bluetooth=length>64;
  clear_glance_filter();std::vector<std::uint8_t> report(length,0);const unsigned start=bluetooth?(kind==dingosdk::PlayStationPad::dualsense?2:3):1;
  if(length==547) std::fill(report.begin()+78,report.end(),0xa5);
  const unsigned button=start+(kind==dingosdk::PlayStationPad::dualsense?7:4);report[0]=bluetooth?(kind==dingosdk::PlayStationPad::dualsense?0x31:0x11):1;
  for(unsigned i=0;i<4;++i)report[start+i]=128;report[button+1]=1; // L1
  StockHidRead read{reinterpret_cast<void*>(10),report.data(),static_cast<unsigned>(report.size()),kind};
  auto submit=[&](unsigned hat,unsigned sequence){report[button]=0x20|hat; // preserve Cross
    if(kind==dingosdk::PlayStationPad::dualsense)report[start+6]=sequence;else report[start+6]=sequence<<2;
    sony_report_crc(report);qa_now+=10;publish();filtered_stock_hid(read,static_cast<DWORD>(report.size()));};
  submit(0,1);check((report[button]&15)==8&&(report[button]&0xf0)==0x20&&report[button+1]==1,"Sony first tap damaged other buttons");
  if(bluetooth) {
    std::uint32_t checksum{};std::memcpy(&checksum,report.data()+74,4);
    // Independent expected values computed with Python zlib.crc32 over 0xa1 + payload.
    check(checksum==(kind==dingosdk::PlayStationPad::dualsense ? 0x350f57b5u:0x797b0b63u),"Bluetooth first-press checksum invalid");
    if(length==547)check(std::all_of(report.begin()+78,report.end(),[](auto v){return v==0xa5;}),"DS4 Windows padding changed");
  }
  const auto edited=report;filtered_stock_hid(read,static_cast<DWORD>(report.size()));check(report==edited&&!glance_emote_active(),"duplicate shared completion became tap");
  submit(8,2);submit(0,3);check((report[button]&15)==0&&glance_emote_active(),"Sony double tap blocked");submit(0,4);check((report[button]&15)==0,"Sony second hold blocked");submit(8,5);check(!glance_emote_active(),"Sony release stuck");
  clear_glance_filter();private_controller_handle(read.file,true);submit(0,6);check((report[button]&15)==0,"private Sony reader filtered");private_controller_handle(read.file,false);
  if(bluetooth){auto expected=report;sony_report_crc(expected);check(expected==report,"Bluetooth checksum stale");}
 }
 auto changed=bindings;clear_glance_filter();qa_now+=10;poll(0x101);qa_now+=10;poll(0x100);changed[0].combo=0;
 publish_glance_filter(changed,true,{1,2,3,4},1);auto bypass=classify(1,0,0x101);check(!bypass.consume&&!bypass.emote,"changed binding still blocked");
 // Raw Input uses the same selective edit, including when existing stick filtering also edits the report.
 for(bool capture:{false,true}) {
  clear_glance_filter();qa_capture=capture;
  const unsigned bytes=offsetof(RAWINPUT,data.hid.bRawData)+64;
  std::vector<std::uint64_t> storage((bytes+7)/8);auto* record=reinterpret_cast<RAWINPUT*>(storage.data());
  record->header.dwType=RIM_TYPEHID;record->header.dwSize=bytes;record->header.hDevice=reinterpret_cast<HANDLE>(30);
  record->data.hid.dwSizeHid=64;record->data.hid.dwCount=1;devices[record->header.hDevice]=dingosdk::PlayStationPad::dualsense;
  auto* report=record->data.hid.bRawData;
  auto raw=[&](unsigned hat,unsigned sequence){std::fill_n(report,64,0);report[0]=1;report[1]=report[2]=128;report[3]=210;report[4]=150;report[7]=sequence;report[8]=hat;report[9]=1;qa_now+=10;publish();filter_record(record,bytes);};
  raw(0,1);check((report[8]&15)==8&&report[9]==1,"Raw Input glance leaked or LB changed");
  if(capture)check(report[3]==128&&report[4]==128,"existing stick filter lost");else check(report[3]==210&&report[4]==150,"glance filter changed sticks");
  filter_record(record,bytes);check(!glance_emote_active(),"duplicate Raw Input advanced tap state");
  raw(0,2);check((report[8]&15)==8&&!glance_emote_active(),"fresh held Raw Input became double tap");
  raw(8,3);raw(0,4);check((report[8]&15)==0&&glance_emote_active(),"Raw Input double tap blocked");
  const auto saved=std::vector<unsigned char>(report,report+64);raw_controller_read=true;filter_record(record,bytes);raw_controller_read=false;check(std::equal(saved.begin(),saved.end(),report),"private raw report changed");
 }
 qa_capture=false;
 clear_glance_filter();qa_now+=10;publish();classify(1,0,0x101);qa_now+=10;classify(1,0,0x100);qa_now+=101;
 check(!classify(1,0,0x101).consume,"stale camera eligibility consumed input");
 publish_glance_filter(bindings,true,{9,2,3,4},1);check(!classify(1,0,0x101).consume,"stale skater owner consumed input");
 publish();qa_now+=10;classify(1,0,0x101);qa_now+=10;classify(1,0,0x100);
 publish_glance_filter(bindings,true,{1,2,3,4},2);check(!classify(1,0,0x101).emote,"controller-set change paired taps");
 check(hid_calls>0,"original shared HID handler bypassed");
 // Camera toggles block emotes in third person too, with no double-tap bypass.
 clear_glance_filter();qa_active=false;
 auto publish_toggle=[&](){publish_glance_filter(bindings,qa_active,{1,2,3,4},1,0x101,true);};
 auto toggle_poll=[&](unsigned buttons,unsigned slot=0){sample.Gamepad.wButtons=buttons;qa_now+=10;publish_toggle();XINPUT_STATE out{};filtered_state<0>(slot,&out);return out.Gamepad.wButtons;};
 check(toggle_poll(0x1101)==0x1100,"Third-person toggle leaked emote or other buttons");
 qa_active=true;check(toggle_poll(0x1101)==0x1100,"Entering first person lost held toggle suppression");
 qa_active=false;check(toggle_poll(0x1001)==0x1000,"Releasing LB leaked held D-pad during exit");
 toggle_poll(0);check(toggle_poll(0x101)==0x100&&!glance_emote_active(),"Toggle second tap bypassed emote block");
 check(toggle_poll(0x102)==0x102,"Unbound D-pad direction blocked");
 raw_controller_read=true;check(toggle_poll(0x101)==0x101,"Toggle sampler was filtered");raw_controller_read=false;
 qa_menu=true;check(toggle_poll(0x101)==0x101,"Menu capture was overridden");qa_menu=false;
 publish_glance_filter(bindings,false,{1,2,3,4},1,0x101,false);
 check(!classify(1,0,0x101).consume,"Paused toggle consumed input");
 for(auto kind:{dingosdk::PlayStationPad::dualshock4,dingosdk::PlayStationPad::dualsense})for(bool bluetooth:{false,true}){
  clear_glance_filter();std::vector<std::uint8_t> report(bluetooth?78:64,0);
  const unsigned start=bluetooth?(kind==dingosdk::PlayStationPad::dualsense?2:3):1;
  const unsigned button=start+(kind==dingosdk::PlayStationPad::dualsense?7:4);
  report[0]=bluetooth?(kind==dingosdk::PlayStationPad::dualsense?0x31:0x11):1;
  for(unsigned i=0;i<4;++i)report[start+i]=128;
  report[button+1]=1;StockHidRead read{reinterpret_cast<void*>(40),report.data(),static_cast<unsigned>(report.size()),kind};
  auto submit_toggle=[&](unsigned hat,unsigned sequence){report[button]=0x20|hat;report[start+6]=static_cast<std::uint8_t>(sequence<<2);sony_report_crc(report);qa_now+=10;publish_toggle();filtered_stock_hid(read,static_cast<DWORD>(report.size()));};
  submit_toggle(0,1);check((report[button]&15)==8 && report[button+1]==1 && (report[button]&0xf0)==0x20,"Sony third-person toggle leaked or altered other buttons");
  const auto duplicate=report;filtered_stock_hid(read,static_cast<DWORD>(report.size()));check(report==duplicate,"Duplicate Sony toggle report changed");
  qa_now+=1200;report[button]=0x20;report[button+1]=0;sony_report_crc(report);
  filtered_stock_hid(read,static_cast<DWORD>(report.size()));
  check((report[button]&15)==8,"Long Sony polling gap leaked held direction after L1 release");
  if(bluetooth){const auto edited=report;sony_report_crc(report);check(report==edited,"Long-gap Sony checksum invalid");}
  report[button+1]=1;
  submit_toggle(8,2);submit_toggle(0,3);check((report[button]&15)==8&&!glance_emote_active(),"Sony toggle double tap bypassed suppression");
  if(bluetooth){const auto edited=report;sony_report_crc(report);check(report==edited,"Sony toggle checksum invalid");}
 }
 qa_active=true;clear_glance_filter();
 // Transient camera readiness/ownership changes must never route a toggle
 // second tap into the glance policy's emote bypass.
 qa_active=false;qa_now+=10;
 publish_glance_filter(bindings,false,{1,2,3,4},1,0x101,true,99);
 check(classify(1,0,0x101).consume==1,"Initial queued toggle was not consumed");
 qa_now+=10;classify(1,0,0x100);
 qa_active=true;qa_spectator=true;qa_now+=10;
 publish_glance_filter(bindings,true,{0,0,0,0},1,0x101,true,99);
 sample.Gamepad.wButtons=0x101;XINPUT_STATE transitioning{};
 filtered_state<0>(0,&transitioning);
 check(transitioning.Gamepad.wButtons==0x100&&!glance_emote_active(),"Transition toggle became a double-tap emote");
 qa_now+=10;publish_glance_filter(bindings,false,{1,2,3,4},1,0x101,true,99);
 check(classify(1,0,1).consume==1,"Transient owner change lost consumed hold after LB release");
 qa_now+=150;check(classify(1,0,0x101).consume==1&&!glance_emote_active(),"Short camera rebuild stall leaked a toggle emote");
 qa_now+=1200;sample.Gamepad.wButtons=0x101;filtered_state<0>(0,&transitioning);
 check(transitioning.Gamepad.wButtons==0x100&&!glance_emote_active(),"Long camera/hair update leaked a toggle emote");
 qa_now+=1200;sample.Gamepad.wButtons=1;filtered_state<0>(0,&transitioning);
 check(transitioning.Gamepad.wButtons==0&&!glance_emote_active(),"Long polling gap forgot held D-pad after LB release");
 sample.Gamepad.wButtons=0;filtered_state<0>(0,&transitioning);
 qa_now+=1200;sample.Gamepad.wButtons=0x101;filtered_state<0>(0,&transitioning);
 check(transitioning.Gamepad.wButtons==0x100&&!glance_emote_active(),"Second toggle after long producer gap bypassed suppression");
 auto edited_bindings=bindings;edited_bindings[1].combo=0;
 publish_glance_filter(edited_bindings,false,{0,0,0,0},1,0x101,true,99);
 check(classify(1,0,1).consume==1,"Glance edit erased toggle release latch");
 qa_focus=false;sample.Gamepad.wButtons=0x101;filtered_state<0>(0,&transitioning);
 check(transitioning.Gamepad.wButtons==0x101,"Focus loss kept consuming toggle");qa_focus=true;
 publish_glance_filter(bindings,false,{1,2,3,4},1,0x101,true,99);
 check(!classify(1,0,1).consume,"Focus loss retained a stale held direction");
 qa_spectator=false;qa_now+=10;classify(1,0,0);
 publish_glance_filter(bindings,true,{1,2,3,4},1,0x101,true,100);
 check(!classify(1,0,1).consume,"New world inherited toggle consumption");
 ToggleEmoteBlock extra;
 check(extra.update(0x1101,0x101)==0 && extra.update(0x1101,0x1101)==1 && extra.update(0x1101,1)==1 && extra.update(0x1101,0)==0,"Additional toggle buttons or release latch failed");
 clear_glance_filter();
 qa_active=true;qa_now+=10;publish_glance_filter(bindings,true,{1,2,3,4},1,0x108,true,101);
 check(classify(1,0,0x101).consume==1,"Glance first tap failed beside toggle filter");
 qa_now+=10;classify(1,0,0x100);
 publish_toggle_filter(0x108,true,1,101);qa_now+=10;
 check(classify(1,0,0x101).emote,"Early toggle publication erased glance double-tap state");
 clear_glance_filter();
 // Stock owns LB+RB+D-pad down, so companion activation is deliberately
 // blocked. Its game-facing emote suppression must remain active in both views.
 Config native_config;native_config.toggle=0x302;native_config.native.active=true;native_config.native.bindings[10]=0x302;
 const auto native_route=toggle_emote_bindings(native_config,false);
 check(native_route.companion==0 && native_route.native==0x302,"native toggle routing regression fixture invalid");
 auto publish_native=[&] {publish_glance_filter(bindings,qa_active,{1,2,3,4},1,native_route.companion,native_route.enabled(),555,native_route.native);};
 auto native_poll=[&](unsigned buttons) {qa_now+=10;publish_native();sample.Gamepad.wButtons=buttons;XINPUT_STATE out{};filtered_state<0>(0,&out);return out.Gamepad.wButtons;};
 for(bool entering:{true,false}) {
  clear_glance_filter();qa_active=!entering;
  check(native_poll(0x300)==0x300,"native toggle modifiers were consumed");
  check(native_poll(0x1302)==0x1300,"native LB+RB+D-pad toggle leaked an emote or consumed other buttons");
  qa_active=entering;
  check(native_poll(0x1302)==0x1300,"camera handover lost native toggle suppression");
  check(native_poll(0x1002)==0x1000,"native toggle shoulder-first release leaked a held D-pad");
  native_poll(0x300);
  check(native_poll(0x302)==0x300 && !glance_emote_active(),"native toggle second tap became a double-tap emote");
  native_poll(0);
 }
 raw_controller_read=true;check(native_poll(0x302)==0x302,"native toggle's physical sampler was blocked");raw_controller_read=false;
 // Both Sony report formats use the same native-owned chord and hold latch.
 for(auto kind:{dingosdk::PlayStationPad::dualshock4,dingosdk::PlayStationPad::dualsense})for(bool bluetooth:{false,true}) {
  clear_glance_filter();qa_active=false;publish_native();
  std::vector<std::uint8_t> report(bluetooth?78:64,0);
  const unsigned start=bluetooth?(kind==dingosdk::PlayStationPad::dualsense?2:3):1;
  const unsigned button=start+(kind==dingosdk::PlayStationPad::dualsense?7:4);
  report[0]=bluetooth?(kind==dingosdk::PlayStationPad::dualsense?0x31:0x11):1;
  for(unsigned i=0;i<4;++i)report[start+i]=128;
  StockHidRead read{reinterpret_cast<void*>(70),report.data(),static_cast<unsigned>(report.size()),kind};
  auto send=[&](unsigned hat,unsigned shoulders){report[button]=0x20|hat;report[button+1]=shoulders;sony_report_crc(report);qa_now+=10;publish_native();filtered_stock_hid(read,static_cast<DWORD>(report.size()));};
  send(4,3);check((report[button]&15)==8 && report[button+1]==3,"native Sony toggle leaked or altered shoulders");
  qa_active=true;send(4,0);check((report[button]&15)==8,"native Sony shoulder-first release leaked");
  send(8,3);qa_active=false;send(4,3);check((report[button]&15)==8 && !glance_emote_active(),"native Sony exit/double tap leaked");
  if(bluetooth){const auto saved=report;sony_report_crc(report);check(saved==report,"native Sony toggle CRC changed");}
 }
 clear_glance_filter();
 check(dingosdk::hook_initialize()==dingosdk::HookOk,"Hook backend initialization failed");
 test_board_input_adapters();test_direct_input_adapters(argc>1 && std::string_view(argv[1])=="--windows-drivers");
 test_native_hid_adapters();
 std::cout<<"Actual XInput/shared HID adapters, private reads, interruptions and duplicated reports passed\n";
}

catch(const std::exception& error) { std::cerr<<error.what()<<"\n";return 1; }
