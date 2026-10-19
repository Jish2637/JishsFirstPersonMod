#pragma once
#include "mod.h"
#include <cstddef>
#include <algorithm>
#include <stdexcept>
#include <string_view>
namespace jishsfirstpersonmod::stock_contract {
// Private profiles are selected only by exact DLL SHA-256 AND status export RVA.
struct NativeContract {
    const char* version;const char* sha256;
    std::uintptr_t status_rva,state_pointer_rva,mutex_lock_rva,mutex_unlock_rva,write_fov_rva;
    std::uintptr_t source_initialized_rva,source_base_rva,source_transform_rva;
};
inline constexpr NativeContract contracts[]{
    {"1.1.5","822f05dc4ce4aacf2123ff6a29bd595f2feaa099498936c0b35857fffb7d91d9",
     0x3a60a0,0x88f8f0,0x67fa94,0x67fac0,0x251150,0x854d80,0x854fb0,0x854fc0},
    {"1.1.6","5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039",
     0x3a77c0,0x8918f0,0x6811a4,0x6811d0,0x252030,0x856d80,0x856fb0,0x856fc0},
    {"1.1.7","c501c57d12219bd124d7e6b889231d7c2342546e9f38b9d947fcf841668558db",
     0x3ba7c0,0x8a6950,0x693f14,0x693f40,0x265040,0x86bdd0,0x86c000,0x86c010},
    {"1.1.8","6336a00b0a06178bec069f0af0f3922cc3a89c0e29b2b2ee363701da4c63757d",
     0x3ba7e0,0x8a6950,0x693f34,0x693f60,0x265060,0x86bdd0,0x86c000,0x86c010},
    {"2.0.0","ff9b6f63e44fe95c4b902c8b4bb02f2cc8f891d60ccdc15ae486226c7cb1f3a7",
     0x3e1340,0x8edff8,0x6c98c4,0x6c98f0,0x26e080,0x8afdd0,0x8b0068,0x8b0078},
    {"2.0.1","abf77e2b21d133a8a7f635d601a0eff60feb2327f2c0e97bd7f38267552f3512",
     0x3e1ce0,0x8edff8,0x6ca264,0x6ca290,0x26e080,0x8afdd0,0x8b0068,0x8b0078},
    {"2.0.2","082de47e7ce36f34ab395feee5a52f77531bdb5f73f0605c5a4f0ad17f94214e",
     0x3e7220,0x8fe0f8,0x6d70b4,0x6d70e0,0x271890,0x8bfe20,0x8c00b8,0x8c00c8},
    {"2.0.3","083bd7c9636d7ff1432cb643842ba58159f26cf8ab3bbb1d2c22df322f865912",
     0x3c2630,0x8f8e18,0x6c31f4,0x6c3220,0x20e8f0,0x8b6190,0x8b6428,0x8b6438},
    {"2.0.4","9259c3de99626d6a9d6083b337927bc948ee144dbf4e698f7733148979aeb528",
     0x3cbaa0,0x905458,0x6cd424,0x6cd450,0x217f30,0x8c22a0,0x8c2538,0x8c2548},
    {"2.0.5","ca9c103f85167ac6957e15ed45b999f59afc387d2722277114047f8e04fe6aaa",
     0x3cbaa0,0x905458,0x6cd1c4,0x6cd1f0,0x217f30,0x8c22a0,0x8c2538,0x8c2548},
    {"2.0.6","dff7b0211996b596611019f44e2c6de7ca8c18a6fc65106c0e32100e06520d0f",
     0x3e42f0,0x933fc0,0x6ea434,0x6ea460,0x21d4c0,0x8ee2a0,0x8ee540,0x8ee550},
    {"2.0.7","8ae11491fca59317e561211e6abffb0e0dc26890a8845c8e1b0217c2c1a0a427",
     0x3f4190,0x94cf80,0x6fbc74,0x6fbca0,0x22a390,0x9063f0,0x906690,0x9066a0},
    {"2.0.8","ef237e13b2c5ade4e24a7bbdd7824680cb79fd6bdf8d1c13340a0d375ddd2fda",
 0xc18c70,0x1505860,0xf2f894,0xf2f8c0,0xa45010,0x1430e70,0x1431110,0x1431120},
    {"2.1.1","d79ff144768ce8b0a8e992c9f134c670a3ff85bb3b437a0eb915876973014301",
     0xde5250,0x1864198,0x1157774,0x11577a0,0xbe1900,0x17672c0,0x17681f8,0x1768208}
};
inline const NativeContract* selected=nullptr;
inline const NativeContract& contract() { if(!selected) throw std::logic_error("ReSkate contract not validated");return *selected; }
inline const NativeContract* find_contract(const std::array<unsigned char,32>& hash,std::uintptr_t status) {
    const auto nibble=[](char c) { return static_cast<unsigned char>(c<='9' ? c-'0':c-'a'+10); };
    for(const auto& candidate:contracts) {
        if(status!=candidate.status_rva) continue;
        bool match=true;
        for(unsigned i=0;i<32;++i) match=match && hash[i]==
            static_cast<unsigned char>((nibble(candidate.sha256[i*2])<<4)|nibble(candidate.sha256[i*2+1]));
        if(match) return &candidate;
    }
    return nullptr;
}
inline constexpr std::size_t model_debug_offset=128;
// Layout is a property of a reviewed profile, not of its movable export RVA.
inline bool uses_v211_layout(const NativeContract* profile) { return profile && std::string_view(profile->version)=="2.1.1"; }
inline bool uses_v206_layout(const NativeContract* profile) { return profile && (std::string_view(profile->version)=="2.0.6" || std::string_view(profile->version)=="2.0.7" || std::string_view(profile->version)=="2.0.8" || uses_v211_layout(profile)); }
inline bool uses_v2_layout(const NativeContract* profile) {
    if(!profile) return false;
    const std::string_view version(profile->version);
    return version=="2.0.0" || version=="2.0.1" || version=="2.0.2" || version=="2.0.3" || version=="2.0.4" || version=="2.0.5" || version=="2.0.6" || version=="2.0.7" || version=="2.0.8" || uses_v211_layout(profile);
}
inline bool uses_v203_layout(const NativeContract* profile) { return profile && (std::string_view(profile->version)=="2.0.3" || std::string_view(profile->version)=="2.0.4" || std::string_view(profile->version)=="2.0.5"); }
inline std::size_t profile_callbacks_offset(const NativeContract* profile) { return uses_v211_layout(profile) ? 0x5b0:uses_v206_layout(profile) ? 0x590:uses_v203_layout(profile) ? 0x4d0:uses_v2_layout(profile) ? 0x4b8:0x4b0; }
inline std::size_t profile_model_offset(const NativeContract* profile) { return uses_v211_layout(profile) ? 0x5e0:uses_v206_layout(profile) ? 0x5c0:uses_v203_layout(profile) ? 0x500:uses_v2_layout(profile) ? 0x4e8:0x4e0; }
inline std::uintptr_t native_sampler_rva() { return uses_v211_layout(selected) ? 0xbe3090:std::string_view(contract().version)=="2.0.8" ? 0xa467a0:std::string_view(contract().version)=="2.0.7" ? 0x22bb20:uses_v206_layout(selected) ? 0x21ec50:(std::string_view(contract().version)=="2.0.4" || std::string_view(contract().version)=="2.0.5") ? 0x2196c0:uses_v203_layout(selected) ? 0x210080:std::string_view(contract().version)=="2.0.2" ? 0x273020:0x26f810; }
inline std::uintptr_t console_queue_rva() { return uses_v211_layout(selected) ? 0x979980:std::string_view(contract().version)=="2.0.8" ? 0x811900:std::string_view(contract().version)=="2.0.7" ? 0x42210:uses_v206_layout(selected) ? 0x41d90:(std::string_view(contract().version)=="2.0.4" || std::string_view(contract().version)=="2.0.5") ? 0x411b0:uses_v203_layout(selected) ? 0x40e30:std::string_view(contract().version)=="2.0.2" ? 0x3ec00:0x3ea30; }
inline std::size_t bindings_model_offset() { return uses_v211_layout(selected) ? 0x5fe0:std::string_view(contract().version)=="2.0.8" ? 0x5f88:uses_v206_layout(selected) ? 0x5f78:0x5f50; }
inline bool is_v2() { return uses_v2_layout(selected); }
inline std::size_t callbacks_offset() { return profile_callbacks_offset(selected); }
inline std::size_t model_offset() { return profile_model_offset(selected); }
inline std::size_t debug_status_offset() { return is_v2() ? 0x1a8:0x168; }
inline std::size_t camera_issue_offset() { return is_v2() ? 0xa0:0x88; }
enum Action:int { first_person=11, fov, spring, offset_x, offset_y, offset_z,
    pitch,yaw,roll,up,down,left,right,reset };
struct Request { int action{}; bool enabled{}; float value{}; };
// Keep the mod's stable command IDs internal. Unknown/new stock commands pass
// through unchanged instead of being interpreted as legacy camera commands.
inline Request to_native(Request request) {
    if(is_v2()) {
        if(request.action>=first_person && request.action<reset) request.action+=2;
        else if(request.action==reset) request.action=uses_v206_layout(selected) ? 34:33;
    }
    return request;
}
inline bool from_native(const Request& wire,Request& request) {
    request=wire;
    if(!is_v2()) return true;
    if(wire.action>=13 && wire.action<=25) request.action-=2;
    else if(wire.action==(uses_v206_layout(selected) ? 34:33)) request.action=reset;
    else if(wire.action>=11) return false;
    return true;
}
struct DebugPrefix {
    bool available{},camera_available{},ui_available{},free_camera{},first_person{},drone_follow{};
    float fov{},free_fov{};
    dingosdk::first_person::Settings arm;
};
static_assert(sizeof(Request)==12 && offsetof(Request,value)==8);
static_assert(offsetof(DebugPrefix,arm)==16 && sizeof(DebugPrefix)==60);
inline bool apply(Config& config,const Request& request) {
    switch(request.action) {
    case fov: config.fov=request.value; break;
    case spring: config.spring.enabled=request.enabled; break;
    case offset_x:case offset_y:case offset_z:config.spring.offset[request.action-offset_x]=request.value;break;
    case pitch:case yaw:case roll:config.spring.rotation[request.action-pitch]=request.value;break;
    case up:config.spring.up=request.value;break;case down:config.spring.down=request.value;break;
    case left:config.spring.left=request.value;break;case right:config.spring.right=request.value;break;
    case reset:config.spring={};break;
    default:return false;
    }
    return true;
}
inline std::array<float,12> camera_values(const Config& config) {
    return {config.fov,config.spring.enabled ? 1.0f:0.0f,
        config.spring.offset[0],config.spring.offset[1],config.spring.offset[2],
        config.spring.rotation[0],config.spring.rotation[1],config.spring.rotation[2],
        config.spring.up,config.spring.down,config.spring.left,config.spring.right};
}
inline void set_camera_values(Config& config,const std::array<float,12>& values) {
    config.fov=values[0];config.spring.enabled=values[1]!=0;
    for(unsigned i=0;i<3;++i) { config.spring.offset[i]=values[2+i];config.spring.rotation[i]=values[5+i]; }
    config.spring.up=values[8];config.spring.down=values[9];config.spring.left=values[10];config.spring.right=values[11];
}
struct CameraMirror {
    bool initialized{};
    std::array<float,12> last_output{};
    std::array<std::uint64_t,12> pending_until{};
    void requested(const Config& before,const Config& after,std::uint64_t now) {
        const auto a=camera_values(before),b=camera_values(after);
        for(unsigned i=0;i<12;++i) if(a[i]!=b[i]) pending_until[i]=now+2000;
    }
    Config reconcile(const Config& native,const Config& current,std::uint64_t now) {
        auto next=current;
        const auto source=camera_values(native);auto values=camera_values(current);
        if(initialized) for(unsigned i=0;i<12;++i) {
            // The stock zero-FOV setter is unsupported. Only an explicit stock
            // FOV request (already routed by queue_hook) exits game-FOV mode.
            if(i==0 && values[i]==0 && source[i]>0) continue;
            if(source[i]==values[i]) pending_until[i]=0;
            else if(now>=pending_until[i] && source[i]!=last_output[i] && values[i]==last_output[i]) values[i]=source[i];
        }
        set_camera_values(next,values);last_output=values;initialized=true;return next;
    }
};
}
