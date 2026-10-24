#pragma once
#include <cstdint>
#include <cstring>
#include <atomic>
#include <string_view>
namespace jishsfirstpersonmod::native211 {
struct Entry { const char* role; std::uintptr_t rva; std::string_view prefix; };
inline constexpr Entry status{"status",0xf1d8e0,"4885c90f849c000000574883ec30488bf948895c2440e895f8ffff488bc8488b"};
inline constexpr Entry getter{"getter",0xf1d190,"4883ec288b0d9e9ad70065488b042558000000ba60010100488b04c88b040239"};
inline constexpr Entry ctor{"ctor",0xf19f60,"48895c24104889742418574883ec40488bf948894c243033f60f57c00f114118"};
inline constexpr Entry lock{"lock",0x13d6914,"33d2e949000000cc4883ec38488d54242048c744242000000000c74424280000"};
inline constexpr Entry unlock{"unlock",0x13d6940,"4883ec2883694c017511c74148ffffffff4883c110ff15a5710d0033c04883c4"};
inline constexpr Entry fov{"fov",0xd2c230,"40534883ec300f297424200f28f1488bd90f28c1e8c7b66b006685c07f3cf30f"};
inline constexpr Entry sampler{"sampler",0xd2d9e0,"488bc4488958085556574154415541564157488da868fdffff4881ec60030000"};
inline constexpr Entry source_init{"source_init",0xd1e1c0,"48895c2408574883ec20488bf9ff15f5f678008bd8894424388b15598af70065"};
inline constexpr Entry read{"read",0x988b80,"488bc4488958084889701848897820554154415541564157488da898f6ffff48"};
inline constexpr Entry queue{"queue",0x988600,"48895c240848896c2410488974241848897c24204154415641574881ec800000"};
inline constexpr Entry console{"console",0x9882e0,"48895c24085556574154415541564157488d6c24d94881ecd0000000488b05fd"};
inline constexpr Entry party_getter{"party_getter",0xb02000,"4883ec288b0d2e4c190165488b042558000000ba60010100488b04c88b040239"};
inline constexpr Entry remote_getter{"remote_getter",0xadcd40,"48895c2410574883ec208b0de89e1b0165488b042558000000488b3cc8b86001"};
inline constexpr Entry render{"render",0xadcf30,"48895c240848896c2410565741564883ec204c8bf2488bf1e8230500004c8b40"};
inline constexpr Entry controller{"controller",0xf34550,"4885c90f84520200004c8bdc5541564883ec78488b05962cb5004833c4488944"};
inline constexpr Entry gamepad{"gamepad",0xf32950,"48895c2408488974241048897c2418554154415541564157488d6c24c94881ec"};
inline constexpr Entry hid{"hid",0xf2c110,"48895c2410488974241848897c242041564883ec508bfa488bf1e831bdffff44"};
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
