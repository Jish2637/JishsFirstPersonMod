#include "native_camera_interop.h"
#include "marker_native_reset.h"
#include "stick_runtime.h"
#include "head_attachment.h"
#include "Extension/Skater/free_flight.h"
#include <mutex>
#include "stock_contract.h"
#include "stock_bridge.h"
#include "bridge_resolver.h"
#include "game_camera_ownership.h"
#include "camera_policy.h"
#include "settings.h"
#include "owned_hooks.h"
#include "Engine/Core/Platform/memory.h"
#include <array>
#include <string_view>
namespace jishsfirstpersonmod {
namespace {
using Matrix=dingosdk::first_person::Matrix;
using Vec3=dingosdk::first_person::Vec3;
// MSVC x64 ABI for the free function first_person::update returning a
// 64-byte Matrix: RCX=return storage, RDX=Spring, R8=head, R9=Settings,
// [entry RSP+40]=time, [entry RSP+48]=origin. Verified against the release
// DLL's render, FrameSpring::sample, and activation call sites.
using Sample=Matrix*(*)(Matrix*,void*,const Matrix*,const NativeFirstPersonSettings*,double,const Vec3*);
Sample original_sample{};
std::mutex marker_sample_mutex;MarkerNativeReset marker_samples;
Matrix* sample_hook(Matrix* output,void* spring,const Matrix* head,
                    const NativeFirstPersonSettings* saved,double now,const Vec3* origin) {
    const auto error=GetLastError();
    bool locked{},reset_native{},fresh_head{};
    NativeFirstPersonSettings settings{};
    ComfortSnapshot owner{};MarkerReturnStatus marker{};
    std::unique_lock marker_lock(marker_sample_mutex,std::defer_lock);
    if(compatibility::enabled() && !game_camera_suspended() && stock_first_person_requested() && saved) {
        const auto config=menu_snapshot().config;
        owner=comfort_snapshot();observe_marker_return(owner);marker=marker_return_status();
        const auto movement=live_camera_movement();
        locked=owner.state_valid && movement.mode && head_position_attached(config,owner,movement);
        marker_lock.lock();
        // A pending generation must survive the short invalid-owner interval.
        // The observer's pending bit includes the teleport flag, so only count
        // frames with a verified local owner after the native flag clears.
        marker_samples.observe(marker.generation,marker.pending,!owner.state_valid || marker.teleport,now);
        reset_native=marker_samples.reset_sample(marker.pending,now);
        if(locked || reset_native) {
            if(!dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(saved),settings)) {locked=false;reset_native=false;}
        }
        if(marker.pending && !marker.teleport && marker_samples.ready() && owner.state_valid && movement.mode && head && origin) {
            Matrix raw{};Vec3 root{};Matrix body{};
            const auto animation=read_animation_head(owner);
            if(animation.valid && dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(head),raw) &&
               dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(origin),root) &&
               dingosdk::memory::read(owner.rig+0x4720,body) && dingosdk::valid_flight_transform(raw) && dingosdk::valid_flight_transform(body)) {
                using namespace dingosdk::first_person;
                const auto q=orientation(animation.head);
                auto expected=raw;
                const auto right=rotate(q,{0,0,-1}),up=rotate(q,{1,0,0}),back=rotate(q,{0,-1,0});
                for(unsigned i=0;i<3;++i) {expected[i]=right[i];expected[4+i]=up[i];expected[8+i]=back[i];}
                const auto a=orientation(raw),b=orientation(expected);
                float dot=0;for(unsigned i=0;i<4;++i) dot+=a[i]*b[i];
                fresh_head=length(subtract(root,{animation.root[12],animation.root[13],animation.root[14]}))<.05f &&
                    length(subtract(root,{body[12],body[13],body[14]}))<.5f && std::abs(dot)>.9995f;
            }
        }
    }
    // Turning off these temporary sample flags resets both native Spring and
    // its stabilization history to the supplied head. Saved preferences and
    // activation remain unchanged; no opaque spring memory is written.
    if(locked || reset_native) settings=effective_native_sample(settings,true);
    SetLastError(error);
    auto* result=original_sample(output,spring,head,(locked || reset_native) ? &settings:saved,now,origin);
    if(reset_native && fresh_head && result && marker_return_status().generation==marker.generation) {
        Matrix rebuilt{};
        if(dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(result),rebuilt) && dingosdk::valid_flight_transform(rebuilt) &&
           accept_marker_native_view(owner,rebuilt,true)) {
            marker_samples.completed(now);
            log("Return to marker: native spring and stabilization rebuilt from current head; previous camera transform discarded.");
        }
    }
    return result;
}
}
bool install_native_camera_interop(void* module) {
    using namespace dingosdk;
    if(!stock_contract::is_v2()) return true;
    if(!stock_contract::is_v2()) return false;
    // The exact-hash reviewed 2.0.x DLLs use this sampler. Refuse a modified
    // entry rather than patching an unknown function or another mod's detour.
    const auto sample_rva=stock_contract::native_sampler_rva();
    constexpr std::array<unsigned char,32> prefix{
        0x48,0x8b,0xc4,0x48,0x89,0x58,0x8,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,
        0x41,0x57,0x48,0x8d,0xa8,0x68,0xfd,0xff,0xff,0x48,0x81,0xec,0x60,0x3,0x0,0x0};
    const auto target=reinterpret_cast<std::uintptr_t>(module)+sample_rva;
    std::array<unsigned char,32> live{};
    if(!memory::read(target,live) || live!=prefix) {
        log("ERROR: native first-person sampler differs; temporary camera coordination unavailable.");return false;
    }
    void* forwarding{};
    auto result=owned_hook_prepare(reinterpret_cast<void*>(target),reinterpret_cast<void*>(&sample_hook),&forwarding);
    if(result==HookOk) {
        original_sample=reinterpret_cast<Sample>(forwarding);
        result=hook_queue_enable(reinterpret_cast<void*>(target));
        if(result==HookOk) result=apply_owned_hooks();
    }
    if(result!=HookOk) { log("ERROR: native sampler hook failed: "+std::to_string(result));return false; }
    log("Native camera coordination active: stabilization and Follow flips yield to mod head locks; saved preferences untouched.");
    return true;
}
}
