#pragma once
#include <cstdint>
#include <cstring>
#include <atomic>
#include <string_view>
namespace jishsfirstpersonmod::native211 {
struct Entry { const char* role; std::uintptr_t rva; std::string_view prefix; };
inline constexpr Entry status{"status",0xfb65f0,"4885c90f849c000000574883ec30488bf948895c2440e895f8ffff488bc8488b"};
inline constexpr Entry getter{"getter",0xfb5ea0,"4883ec288b0dc6e4e70065488b042558000000bab0010100488b04c88b040239"};
inline constexpr Entry ctor{"ctor",0xfb2bc0,"48895c24104889742418574883ec40488bf948894c243033f60f57c00f114118"};
inline constexpr Entry lock{"lock",0x14c0354,"33d2e949000000cc4883ec38488d54242048c744242000000000c74424280000"};
inline constexpr Entry unlock{"unlock",0x14c0380,"4883ec2883694c017511c74148ffffffff4883c110ff1535050e0033c04883c4"};
inline constexpr Entry fov{"fov",0xd9c050,"40534883ec300f297424200f28f1488bd90f28c1e8d75273006685c07f3cf30f"};
inline constexpr Entry sampler{"sampler",0xd9d800,"488bc4488958085556574154415541564157488da868fdffff4881ec60030000"};
inline constexpr Entry source_init{"source_init",0xd8dfe0,"48895c2408574883ec20488bf9ff15a52a81008bd8894424388b1571630a0165"};
inline constexpr Entry read{"read",0x99a360,"488bc4488958084889701848897820554154415541564157488da898f6ffff48"};
inline constexpr Entry queue{"queue",0x999de0,"48895c240848896c2410488974241848897c24204154415641574881ec800000"};
inline constexpr Entry console{"console",0x999ac0,"48895c24085556574154415541564157488d6c24d94881ecd0000000488b051d"};
inline constexpr Entry party_getter{"party_getter",0xb16c90,"4883ec288b0dd6d6310165488b042558000000bab0010100488b04c88b040239"};
inline constexpr Entry remote_getter{"remote_getter",0xaed490,"48895c2410574883ec208b0dd06e340165488b042558000000488b3cc8b8b001"};
inline constexpr Entry render{"render",0xaed680,"48895c240848896c2410565741564883ec204c8bf2488bf1e8630500004c8b40"};
inline constexpr Entry controller{"controller",0xfbd2e0,"4885c90f84520200004c8bdc5541564883ec78488b0506dbbe004833c4488944"};
inline constexpr Entry gamepad{"gamepad",0xfbb3e0,"48895c2408488974241048897c2418554154415541564157488d6c24c94881ec"};
inline constexpr Entry hid{"hid",0xfc7300,"48895c24085556574154415541564157488bec4883ec608bf2488bf9e80fdbff"};
inline constexpr Entry lock_impl{"lock_impl",0x14c03a4,"48895c241048896c24184889742420574883ec40488b0541aa6e004833c44889"};
inline constexpr Entry entries[]{lock_impl,status,getter,ctor,lock,unlock,fov,sampler,source_init,read,queue,console,party_getter,remote_getter,render,controller,gamepad,hid};
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
