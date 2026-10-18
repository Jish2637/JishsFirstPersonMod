#include "diagnostic_log.h"
#include "stick_runtime.h"
#include "stick_input_filter.h"
#include "stock_bridge.h"
#include "owned_hooks.h"
#include "Engine/Core/Platform/memory.h"
#include "glance_emote_policy.h"
#include "stock_hid_contract.h"
#include "stock_contract.h"
#include "settings.h"
#include "game_camera_ownership.h"
#include <atomic>
#include <Xinput.h>
#include <vector>
#include <set>
#include <map>
#include <mutex>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <deque>
#include <hidsdi.h>
#include <hidpi.h>
#include <intrin.h>
namespace jishsfirstpersonmod {
extern thread_local bool raw_controller_read;
namespace {
using GetState=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);
GetState originals[10]{};
std::mutex glance_mutex;
struct GlanceSource {
    GlanceEmotePolicy policy;
    ToggleEmoteBlock toggle;
    GlanceEmotePolicy::Result result;
    ULONGLONG seen{};
    const void* buffer{};
    std::vector<std::uint8_t> edited;
};
std::map<std::pair<unsigned,std::uintptr_t>,GlanceSource> glance_sources;
std::set<void*> private_handles;
dingosdk::FirstPersonLookControls glance_bindings{};
std::array<std::uintptr_t,4> glance_owner{};
unsigned glance_device{};
bool glance_eligible{},emote_event{};
std::atomic<bool> toggle_eligible{};
std::uint32_t toggle_binding{};
std::uintptr_t toggle_context{};
ULONGLONG glance_published{};
std::atomic<bool> emote_ready{};
std::atomic<ULONGLONG> direct_epoch{};
bool live_glances() {
    if(panel_gameplay_blocked() || !emote_ready || raw_controller_read) return false;
    DWORD pid{};const auto window=GetForegroundWindow();bool visible{};
    const auto comfort=comfort_snapshot();
    return window && GetWindowThreadProcessId(window,&pid) && pid==GetCurrentProcessId() &&
        overlay_available(visible) && !visible && (toggle_eligible.load() ||
        (!game_camera_suspended() && stock_first_person_active() && stock_first_person_requested() && !comfort.suppressed && !comfort.bailing));
}
GlanceEmotePolicy::Result classify(unsigned path,std::uintptr_t source,std::uint32_t buttons,ULONGLONG input_time=GetTickCount64()) {
    const auto now=GetTickCount64();
    const auto owner=comfort_snapshot();
    const auto age=now-glance_published;
    // Toggle ownership lasts until an explicit context/input invalidation.
    // Camera/hair work can stall the producer for more than half a second;
    // a wall-clock timeout must never turn that same press into an emote.
    const bool toggle_live=toggle_eligible;
    const bool glance_live=glance_eligible && age<=100;
    // Ordinary glance eligibility still expires independently.
    if(!glance_live && !toggle_live) { glance_sources.clear();emote_event=false;return {}; }
    const bool same_owner=glance_owner==std::array<std::uintptr_t,4>{owner.core,owner.entity,owner.context,owner.rig};
    if(!same_owner && !toggle_live) { glance_sources.clear();emote_event=false;return {}; }
    if(glance_sources.size()>64) glance_sources.clear();
    auto& state=glance_sources[{path,source}];
    if(state.seen && now-state.seen>500) {
        state.policy.reset();state.result={};state.buffer=nullptr;state.edited.clear();
        // Keep a consumed D-pad held until its actual release, even if LB
        // releases first or the game has not polled this controller recently.
        if(!toggle_live) state.toggle={};
    }
    state.seen=now;
    const auto consumed=state.toggle.update(toggle_live ? toggle_binding:0,buttons);
    if(consumed) { state.policy.reset();emote_event=false;state.result={consumed,0,false};return state.result; }
    if(!same_owner || !glance_live || game_camera_suspended() || !stock_first_person_active() || !stock_first_person_requested() || owner.suppressed || owner.bailing) { state.policy.reset();state.result={};return {}; }
    state.result=state.policy.update(glance_bindings,buttons,input_time);
    if(state.result.emote) emote_event=true;
    return state.result;
}
#include "direct_input_filter.inl"
using StockHid=void(*)(const StockHidRead&,DWORD);
StockHid original_hid{};
void filter_sony_report(unsigned path,std::uintptr_t source,dingosdk::PlayStationPad kind,std::span<std::uint8_t> report,bool eligibility_checked=false,StickInputPolicy policy={},bool menu_capture=false) {
    if(!eligibility_checked) menu_capture=panel_gameplay_blocked();
    if(menu_capture) {neutral_sony_pad(kind,report);return;}
    if(!eligibility_checked && !live_glances()) { clear_glance_filter();return; }
    const auto parsed=dingosdk::parse_playstation_report(kind,report.size()>64,report.data(),report.size());
    if(!parsed) return;
    std::uint32_t buttons=parsed->buttons;
    if(parsed->left_trigger>30) buttons|=0x10000;
    if(parsed->right_trigger>30) buttons|=0x20000;
    if(!eligibility_checked) policy=stick_input_policy();
    if(policy.captures(buttons)) set_sony_stick(kind,report,policy.axes);
    std::lock_guard lock(glance_mutex);
    auto& state=glance_sources[{path,source}];
    // Stock may send one completed read through the shared handler twice.
    // Never interpret our already edited copy as a physical release.
    if(state.buffer==report.data() && state.edited.size()==report.size() &&
        std::equal(state.edited.begin(),state.edited.end(),report.begin())) return;
    const auto result=classify(path,source,buttons);
    if(result.consume) {
        filter_sony_dpad(kind,report,result.consume);
        auto& updated=glance_sources[{path,source}];
        updated.buffer=report.data();updated.edited.assign(report.begin(),report.end());
    } else { auto found=glance_sources.find({path,source});if(found!=glance_sources.end()) { found->second.buffer=nullptr;found->second.edited.clear(); } }
}
void filter_native_sony_read(const StockHidRead& read,DWORD size);
void capture_stock_sony_read(const StockHidRead& read,DWORD size);
void filtered_stock_hid(const StockHidRead& read,DWORD size) {
    const auto error=GetLastError();
    capture_stock_sony_read(read,size); // Keep menu capture, using the device's report boundaries.
    if(diagnostic_file_io) {SetLastError(error);return;}
    bool private_read{};{ std::lock_guard lock(glance_mutex);private_read=private_handles.contains(read.file); }
    if(!diagnostic_file_io && !private_read && read.buffer && size<=read.capacity && size>=10 && size<=512)
        filter_native_sony_read(read,size);
    SetLastError(error);
}
#include "native_hid_filter.inl"
struct FilteredPacket { bool ready{};DWORD physical{},packet{};XINPUT_GAMEPAD pad{}; };
std::mutex packet_mutex;
std::array<std::array<FilteredPacket,4>,10> filtered_packets;
template<unsigned Index> DWORD WINAPI filtered_state(DWORD slot,XINPUT_STATE* output) {
    const auto result=originals[Index](slot,output);const auto error=GetLastError();
    HMODULE caller{};
    if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(_ReturnAddress()),&caller) && caller==GetModuleHandleW(L"reskate.dll")) {SetLastError(error);return result;}
    if(result==ERROR_SUCCESS && output && !raw_controller_read) {
        std::uint32_t buttons=output->Gamepad.wButtons;
        if(output->Gamepad.bLeftTrigger>30) buttons|=0x10000;
        if(output->Gamepad.bRightTrigger>30) buttons|=0x20000;
        const auto policy=stick_input_policy(slot);
        if(policy.captures(buttons)) {
            output->Gamepad.sThumbRX=stick_short(policy.axes.x);output->Gamepad.sThumbRY=stick_short(policy.axes.y);
        } // Release passes the current physical axes verbatim, with no blend.
    }
    if(result==ERROR_SUCCESS && output && !raw_controller_read) {
        if(live_glances()) {
            std::uint32_t buttons=output->Gamepad.wButtons;
            if(output->Gamepad.bLeftTrigger>30) buttons|=0x10000;
            if(output->Gamepad.bRightTrigger>30) buttons|=0x20000;
            std::lock_guard lock(glance_mutex);
            const auto decision=classify(1,slot,buttons);
            output->Gamepad.wButtons&=static_cast<WORD>(~decision.consume);
        } else clear_glance_filter();
    } else if(result!=ERROR_SUCCESS) { std::lock_guard lock(glance_mutex);glance_sources.erase({1,slot}); }
    if(result==ERROR_SUCCESS && output && !raw_controller_read && panel_gameplay_blocked()) output->Gamepad={};
    if(slot<4 && !raw_controller_read) {
        std::lock_guard lock(packet_mutex);auto& packet=filtered_packets[Index][slot];
        if(result==ERROR_SUCCESS && output) {
            if(!packet.ready) { packet.ready=true;packet.packet=output->dwPacketNumber; }
            else if(packet.physical!=output->dwPacketNumber || std::memcmp(&packet.pad,&output->Gamepad,sizeof(packet.pad))) ++packet.packet;
            packet.physical=output->dwPacketNumber;packet.pad=output->Gamepad;output->dwPacketNumber=packet.packet;
        } else packet={};
    }
    SetLastError(error);return result;
}
using RawData=UINT(WINAPI*)(HRAWINPUT,UINT,LPVOID,PUINT,UINT);
using RawBuffer=UINT(WINAPI*)(PRAWINPUT,PUINT,UINT);
RawData original_data{};RawBuffer original_buffer{};
std::mutex devices_mutex;
std::map<HANDLE,dingosdk::PlayStationPad> devices;
void filter_record(RAWINPUT* record,UINT bytes) {
    constexpr auto header=offsetof(RAWINPUT,data.hid.bRawData);
    if(!record || bytes<header || record->header.dwType!=RIM_TYPEHID || raw_controller_read) return;
    dingosdk::PlayStationPad kind{};
    {
        std::lock_guard lock(devices_mutex);
        const auto found=devices.find(record->header.hDevice);
        if(found!=devices.end()) kind=found->second;
        else {
            RID_DEVICE_INFO info{};info.cbSize=sizeof(info);UINT size=sizeof(info);
            if(GetRawInputDeviceInfoW(record->header.hDevice,RIDI_DEVICEINFO,&info,&size)!=UINT(-1) && info.dwType==RIM_TYPEHID)
                kind=dingosdk::playstation_pad(static_cast<std::uint16_t>(info.hid.dwVendorId),static_cast<std::uint16_t>(info.hid.dwProductId));
            if(devices.size()>32) devices.clear();devices.emplace(record->header.hDevice,kind);
        }
    }
    if(kind==dingosdk::PlayStationPad::none) return;
    auto& hid=record->data.hid;
    if(!hid.dwSizeHid || hid.dwCount>(bytes-header)/hid.dwSizeHid) return;
    for(UINT i=0;i<hid.dwCount;++i) {
        const std::span<std::uint8_t> report{hid.bRawData+i*hid.dwSizeHid,hid.dwSizeHid};
        filter_sony_report(3,reinterpret_cast<std::uintptr_t>(record->header.hDevice),kind,report);

    }
}
UINT WINAPI filtered_data(HRAWINPUT raw,UINT command,LPVOID data,PUINT size,UINT header) {
    const auto result=original_data(raw,command,data,size,header);const auto error=GetLastError();
    if(result!=UINT(-1) && data && command==RID_INPUT) filter_record(static_cast<RAWINPUT*>(data),result);
    SetLastError(error);return result;
}
UINT WINAPI filtered_buffer(PRAWINPUT data,PUINT size,UINT header) {
    const auto capacity=size ? *size:0;
    const auto result=original_buffer(data,size,header);const auto error=GetLastError();
    if(result!=UINT(-1) && data) {
        auto* cursor=reinterpret_cast<unsigned char*>(data);UINT remaining=capacity;
        for(UINT i=0;i<result && remaining>=sizeof(RAWINPUTHEADER);++i) {
            auto* record=reinterpret_cast<RAWINPUT*>(cursor);
            const auto bytes=record->header.dwSize;if(bytes<sizeof(RAWINPUTHEADER) || bytes>remaining) break;
            filter_record(record,bytes);
            const auto aligned=(bytes+sizeof(void*)-1)&~(sizeof(void*)-1);
            if(aligned>remaining) break;cursor+=aligned;remaining-=static_cast<UINT>(aligned);
        }
    }
    SetLastError(error);return result;
}
}
void private_controller_handle(void* handle,bool add) {
    std::lock_guard lock(glance_mutex);
    if(add) private_handles.insert(handle);else private_handles.erase(handle);
}
void clear_glance_filter() { std::lock_guard lock(glance_mutex);glance_eligible=false;toggle_eligible=false;glance_sources.clear();emote_event=false;++direct_epoch; }
void publish_toggle_filter(std::uint32_t toggle,bool allowed,unsigned device,std::uintptr_t context) {
    std::lock_guard lock(glance_mutex);
    if(toggle!=toggle_binding || allowed!=toggle_eligible.load() || device!=glance_device || context!=toggle_context) {
        glance_sources.clear();emote_event=false;
        ++direct_epoch;
    }
    toggle_binding=toggle;toggle_eligible=allowed;glance_device=device;toggle_context=context;
}
void publish_glance_filter(const dingosdk::FirstPersonLookControls& bindings,bool eligible,
    const std::array<std::uintptr_t,4>& owner,unsigned device,std::uint32_t toggle,bool toggle_allowed,std::uintptr_t context) {
    std::lock_guard lock(glance_mutex);const bool changed=device!=glance_device || toggle!=toggle_binding || toggle_allowed!=toggle_eligible.load() || context!=toggle_context;
    bool glance_changed{};
    for(unsigned i=0;i<4;++i) glance_changed=glance_changed || bindings[i].combo!=glance_bindings[i].combo;
    if(changed || (!eligible && !toggle_allowed)) { glance_sources.clear();emote_event=false;++direct_epoch; }
    else if(glance_changed || eligible!=glance_eligible || owner!=glance_owner) {
        for(auto& [key,state]:glance_sources) { state.policy.reset();state.result={}; }
        emote_event=false;
        ++direct_epoch;
    }
    toggle_binding=toggle;toggle_eligible=toggle_allowed;toggle_context=context;
    glance_bindings=bindings;glance_owner=owner;glance_device=device;glance_eligible=eligible;glance_published=GetTickCount64();
}
bool glance_emote_active() {
    std::lock_guard lock(glance_mutex);const auto now=GetTickCount64();
    bool active=emote_event;emote_event=false;
    for(const auto& [key,state]:glance_sources) if(now-state.seen<=100 && state.result.forward) active=true;
    return active;
}
bool glance_emote_filter_ready() { return emote_ready.load(); }
bool install_stick_input_filter() {
    using namespace dingosdk;
    const void* detours[]{reinterpret_cast<void*>(&filtered_state<0>),reinterpret_cast<void*>(&filtered_state<1>),
        reinterpret_cast<void*>(&filtered_state<2>),reinterpret_cast<void*>(&filtered_state<3>),
        reinterpret_cast<void*>(&filtered_state<4>),reinterpret_cast<void*>(&filtered_state<5>),
        reinterpret_cast<void*>(&filtered_state<6>),reinterpret_cast<void*>(&filtered_state<7>),
        reinterpret_cast<void*>(&filtered_state<8>),reinterpret_cast<void*>(&filtered_state<9>)};
    const wchar_t* names[]{L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll",L"xinput1_2.dll",L"xinput1_1.dll"};
    std::set<std::uintptr_t> seen;std::vector<void*> pending;
    unsigned count{};
    for(unsigned i=0;i<5;++i) {
        const auto module=GetModuleHandleW(names[i]);if(!module) continue;
        for(unsigned extended=0;extended<2;++extended) {
            const auto entry=reinterpret_cast<std::uintptr_t>(GetProcAddress(module,extended ? MAKEINTRESOURCEA(100):"XInputGetState"));
            if(!entry) continue;
            const auto handler=resolve_stock_handler(entry);const auto address=handler ? handler:entry;
            if(!seen.insert(address).second) continue;
            const unsigned index=i*2+extended;void* forwarding{};
            auto* target=reinterpret_cast<void*>(address);
            if(owned_hook_prepare(target,const_cast<void*>(detours[index]),&forwarding)!=HookOk) return false;
            originals[index]=reinterpret_cast<GetState>(forwarding);pending.push_back(target);++count;
        }
    }
    if(!count) return false;
    const auto user32=GetModuleHandleW(L"user32.dll");
    const auto add=[&](const char* name,void* detour,void** original) {
        const auto entry=reinterpret_cast<std::uintptr_t>(GetProcAddress(user32,name));
        const auto handler=resolve_stock_handler(entry);auto* target=reinterpret_cast<void*>(handler ? handler:entry);
        if(owned_hook_prepare(target,detour,original)!=HookOk) return false;
        pending.push_back(target);return true;
    };
    void* data{};void* buffer{};
    if(!add("GetRawInputData",reinterpret_cast<void*>(&filtered_data),&data) ||
       !add("GetRawInputBuffer",reinterpret_cast<void*>(&filtered_buffer),&buffer)) return false;
    original_data=reinterpret_cast<RawData>(data);original_buffer=reinterpret_cast<RawBuffer>(buffer);
    const auto stock=GetModuleHandleW(L"reskate.dll");const auto base=reinterpret_cast<std::uintptr_t>(stock);
    const bool hid_verified=stock && stock_contract::selected && std::string_view(stock_contract::selected->version)=="2.1.0" &&
        verified_stock_hid([](std::uintptr_t address,void* data,std::size_t size) { return memory::read_bytes(address,data,size); },base);
    if(hid_verified) {
        void* original{};auto* target=reinterpret_cast<void*>(base+stock_hid_rva);
        if(owned_hook_prepare(target,reinterpret_cast<void*>(&filtered_stock_hid),&original)!=HookOk) return false;
        original_hid=reinterpret_cast<StockHid>(original);pending.push_back(target);
    } else log("Glance emote blocking unavailable: shared ReSkate 2.0.7 HID handler did not verify. Existing stick filters remain available.");
    if(!prepare_direct_input(pending)) log("DirectInput gesture coverage incomplete: device adapters could not be prepared.");
    if(!prepare_native_hid(pending)) return false;
    for(auto* target:pending) if(hook_queue_enable(target)!=HookOk) return false;
    const bool installed=apply_owned_hooks()==HookOk;emote_ready=installed && hid_verified;
    if(emote_ready) log("Glance emote blocking ready: XInput/Raw Input and verified ReSkate HID handler. DirectInput coverage is reported separately per Sony device.");
    return installed;
}
}
