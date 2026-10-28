#pragma once
#include "Engine/Game/Input/playstation_report.h"
#include <span>
#include "board_look.h"
#include "Engine/Game/Input/controller_bindings.h"
#include <array>
namespace jishsfirstpersonmod {
inline void sony_report_crc(std::span<std::uint8_t> report) {
    // 2.1.3 also accepts Windows-padded 547-byte DS4 Bluetooth reads. Their
    // CRC stays at byte 74; bytes 78 onward are padding, not checksum input.
    dingosdk::refresh_playstation_bluetooth_crc(report.data(),report.size());
}
inline bool filter_sony_dpad(dingosdk::PlayStationPad kind,std::span<std::uint8_t> report,std::uint32_t consume) {
    const auto parsed=dingosdk::parse_playstation_report(kind,report.size()>64,report.data(),report.size());
    if(!parsed) return false;
    const auto id=report[0];const unsigned start=id==0x31 ? 2:id==0x11 ? 3:1;
    const bool full=kind==dingosdk::PlayStationPad::dualsense && (id==0x31 || (id==1 && report.size()<=64));
    const unsigned offset=start+(full ? 7:4);
    const auto remaining=(parsed->buttons&15)&~consume;
    constexpr unsigned hats[16]{8,0,4,8,6,7,5,8,2,1,3,8,8,8,8,8};
    report[offset]=static_cast<std::uint8_t>((report[offset]&0xf0)|hats[remaining]);
    sony_report_crc(report);return true;
}
void publish_glance_filter(const dingosdk::FirstPersonLookControls&,bool eligible,const std::array<std::uintptr_t,4>& owner,unsigned device,
                          std::uint32_t toggle=0,bool toggle_eligible=false,std::uintptr_t toggle_context=0,std::uint32_t native_toggle=0);
void clear_glance_filter();
void publish_toggle_filter(std::uint32_t toggle,bool allowed,unsigned device,std::uintptr_t context,std::uint32_t native_toggle=0);
bool glance_emote_active();
bool glance_emote_filter_ready();
void private_controller_handle(void* handle,bool add);
inline bool set_sony_stick(dingosdk::PlayStationPad kind,std::span<std::uint8_t> report,StickAxes axes) {
    if(!dingosdk::parse_playstation_report(kind,report.size()>64,report.data(),report.size())) return false;
    const auto id=report[0];
    const unsigned start=id==0x31 ? 2:id==0x11 ? 3:1;
    report[start+2]=static_cast<std::uint8_t>(std::clamp(std::lround(128.f+stick_short(axes.x)/258.f),0l,255l));
    report[start+3]=axes.y==0 ? 128:static_cast<std::uint8_t>(std::clamp(std::lround(127.f-stick_short(axes.y)/258.f),0l,255l));
    sony_report_crc(report);
    return true;
}
inline bool neutral_sony_pad(dingosdk::PlayStationPad kind,std::span<std::uint8_t> report) {
    if(!dingosdk::parse_playstation_report(kind,report.size()>64,report.data(),report.size())) return false;
    const auto id=report[0];const unsigned start=id==0x31 ? 2:id==0x11 ? 3:1;
    const bool full=kind==dingosdk::PlayStationPad::dualsense && (id==0x31 || (id==1 && report.size()<=64));
    const unsigned buttons=start+(full ? 7:4);
    for(unsigned i=0;i<4;++i) report[start+i]=128;
    report[buttons]=8;report[buttons+1]=0;report[buttons+2]&=0xfc;
    report[start+(full ? 4:7)]=report[start+(full ? 5:8)]=0;
    sony_report_crc(report);return true;
}
inline bool neutral_sony_stick(dingosdk::PlayStationPad kind,std::span<std::uint8_t> report) { return set_sony_stick(kind,report,{}); }
}
