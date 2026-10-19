#pragma once
#include <cstdint>
#include <cstring>
#include <atomic>
#include <string_view>
namespace jishsfirstpersonmod::native211 {
struct Entry { const char* role; std::uintptr_t rva; std::string_view prefix; };
inline constexpr Entry status{"status",0xde5250,"4885c90f849c000000574883ec30488bf948895c2440e895f8ffff488bc8488b"};
inline constexpr Entry getter{"getter",0xde4b00,"4883ec288b0d2ed0b10065488b042558000000ba80ff0000488b04c88b040239"};
inline constexpr Entry ctor{"ctor",0xde19a0,"48895c24104889742418574883ec30488bf948894c242033f60f57c00f114118"};
inline constexpr Entry lock{"lock",0x1157774,"33d2e949000000cc4883ec38488d54242048c744242000000000c74424280000"};
inline constexpr Entry unlock{"unlock",0x11577a0,"4883ec2883694c017511c74148ffffffff4883c110ff15cdd30b0033c04883c4"};
inline constexpr Entry fov{"fov",0xbe1900,"40534883ec300f297424200f28f1488bd90f28c1e8176a58006685c07f3cf30f"};
inline constexpr Entry sampler{"sampler",0xbe3090,"488bc4488958085556574154415541564157488da868fdffff4881ec60030000"};
inline constexpr Entry source_init{"source_init",0xbd8b10,"48895c2408574883ec20488bf9ff15ddbb63008bd8894424388b150990d20065"};
inline constexpr Entry read{"read",0x97a220,"488bc4488958084889701848897820554154415541564157488da8d8f6ffff48"};
inline constexpr Entry queue{"queue",0x979ca0,"48895c240848896c2410488974241848897c24204154415641574881ec800000"};
inline constexpr Entry console{"console",0x979980,"48895c24085556574154415541564157488d6c24d94881ecd0000000488b051d"};
inline constexpr Entry party_getter{"party_getter",0xa734e0,"4883ec288b0d4ee6e80065488b042558000000ba80ff0000488b04c88b040239"};
inline constexpr Entry remote_getter{"remote_getter",0xa4acf0,"48895c2410574883ec208b0d386eeb0065488b042558000000488b3cc8b880ff"};
inline constexpr Entry render{"render",0xa4aee0,"48895c240848896c2410565741564883ec204c8bf2488bf1e8330400004c8b40"};
inline constexpr Entry controller{"controller",0xdea9e0,"4885c90f84520200004c8bdc5541564883ec78488b05c6bd98004833c4488944"};
inline constexpr Entry gamepad{"gamepad",0xde93d0,"48895c242055488bec4883ec40488bd9488d4de0e867fbffff66c70300000f10"};
inline constexpr Entry hid{"hid",0xdf5b00,"4889742418574883ec508bfa488bf1e8fcc8ffff84c00f843601000048895c24"};
inline constexpr Entry entries[]{status,getter,ctor,lock,unlock,fov,sampler,source_init,read,queue,console,party_getter,remote_getter,render,controller,gamepad,hid};
inline constexpr std::size_t debug_revision_offset=26128;
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
