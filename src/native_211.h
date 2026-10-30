#pragma once
#include <cstdint>
#include <cstring>
#include <atomic>
#include <string_view>
namespace jishsfirstpersonmod::native211 {
struct Entry { const char* role; std::uintptr_t rva; std::string_view prefix; };
inline constexpr Entry status{"status",0x108def0,"4885c90f849d000000574883ec30488bf948895c2440e8a5f8ffff488bc8488b"};
inline constexpr Entry getter{"getter",0x108d7b0,"4883ec288b0df6b0f50065488b042558000000ba88150100488b04c88b040239"};
inline constexpr Entry ctor{"ctor",0x1089d50,"48895c24104889742418574883ec40488bf948894c243033f60f57c00f114118"};
inline constexpr Entry lock{"lock",0x1684524,"33d2e941000000cc4883ec38488364242000488d5424208364242800e8270000"};
inline constexpr Entry unlock{"unlock",0x168454c,"4883ec2883694c01750e834948ff4883c110ff1524670f0033c04883c428c3cc"};
inline constexpr Entry fov{"fov",0xda5de0,"660f7ec8250000807f3d0000807f0f95c03c01753cf30f1005bfa99d000f2fc1"};
inline constexpr Entry sampler{"sampler",0xda75e0,"40555356574156488dac24b0fdffff4881ec500300000f29bc2430030000488b"};
inline constexpr Entry source_init{"source_init",0xd976d0,"40534883ec20488bd9ff1541359e00894424388b15c711250165488b04255800"};
inline constexpr Entry read{"read",0x981fe0,"488bc4488958084889701848897820554154415541564157488da868f6ffff48"};
inline constexpr Entry queue{"queue",0x981a50,"48895c240848896c2410488974241848897c24204154415641574881ec800000"};
inline constexpr Entry console{"console",0x981730,"48895c24085556574154415541564157488d6c24d94881ecd0000000488b05ad"};
inline constexpr Entry party_getter{"party_getter",0xb0f070,"4883ec288b0d36984d0165488b042558000000ba88150100488b04c88b040239"};
inline constexpr Entry remote_getter{"remote_getter",0xae9450,"48895c2410574883ec208b0d50f44f0165488b042558000000488b3cc8b88815"};
inline constexpr Entry render{"render",0xae9640,"48895c240848896c2410565741564883ec204c8bf2488bf1e8630600004c8b40"};
inline constexpr Entry controller{"controller",0x10aca50,"4885c90f84550200004c8bdc5541564883ec78488b05965fd0004833c4488944"};
inline constexpr Entry gamepad{"gamepad",0x10aae10,"48895c2408488974241048897c2418554154415541564157488d6c24c94881ec"};
inline constexpr Entry hid{"hid",0x10a0ac0,"48895c2408555657415641574883ec508bfa488bd9e856daffff8bd7488bcb84"};
inline constexpr Entry entries[]{status,getter,ctor,lock,unlock,fov,sampler,source_init,read,queue,console,party_getter,remote_getter,render,controller,gamepad,hid};
inline constexpr std::size_t debug_revision_offset=26104;
inline void refresh_debug(void* model) {
    const std::uint64_t fresh=0;
    std::memcpy(static_cast<std::byte*>(model)+debug_revision_offset,&fresh,sizeof(fresh));
}
inline constexpr bool panel_controller_navigation=false;
inline bool capture_controller(bool raw,bool recording) { return !raw && recording; }
struct RecordingCapture {
    std::atomic<bool> releasing{};
    bool consume(bool raw,bool recording,bool held) {
        if(raw) return false;
        if(recording) {releasing=true;return true;}
        if(releasing.load() && held) return true;
        releasing=false;return false;
    }
};
}
