#include "game_camera_ownership.h"
#include "native.h"
#include "stock_bridge.h"
#include "stock_contract.h"
#include "stick_runtime.h"
#include "Engine/Game/Build/image_identity.h"
#include <atomic>
#include <mutex>
#include <string_view>

namespace jishsfirstpersonmod {
namespace {
// Source: stock 1.1.8 native_party.h and native_party_hooks.cpp. Its bool()
// hook includes ReSkate's remote-party spectator state, not just EA players.
constexpr std::uintptr_t spectator_active=0x7572a0;
constexpr std::array<unsigned char,32> spectator_prefix{
    0x48,0x83,0xec,0x28,0x48,0x8b,0x5,0xd5,0x74,0xa9,0x6,0x48,0x85,0xc0,0x74,0x4b,
    0x48,0x8b,0x48,0x8,0x48,0x85,0xc9,0x74,0x6,0x48,0x83,0x39,0x0,0x75,0xe,0x48};
using SpectatorActive=bool(*)();
std::atomic<SpectatorActive> spectator_query{};
std::mutex ownership_mutex;
GameCameraOwnershipPolicy policy;
std::atomic<bool> suspended{},managing{};
std::atomic<bool> stock_foreign{};
bool query_spectator(bool& result) noexcept {
    __try { result=spectator_query.load()();return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}
bool game_camera_suspended() { return suspended.load(); }
void observe_stock_camera_owner(bool foreign) {
    stock_foreign=foreign;
    if(!foreign) return; // Only a verified local gameplay snapshot may restore.
    std::lock_guard lock(ownership_mutex);
    policy.reason=GameCameraReason::other_camera;policy.managing=true;
    suspended=true;managing=true;
}
bool game_camera_managing() { return managing.load(); }
const char* game_camera_status() {
    return suspended.load() ? "First person is paused while another camera owns the view.":
        !spectator_query.load() ? "Spectator camera detection is unavailable. See the mod log.":"";
}
void acknowledge_game_camera(bool master,bool active) {
    std::lock_guard lock(ownership_mutex);policy.acknowledge(master,active);managing=policy.managing;
}
void install_game_camera_observer(std::uintptr_t base) {
    // Bootstrap checks the exact game hash. Only the reviewed stock version
    // is accepted here; automatic camera-bridge detection proves no party ABI.
    if((std::string_view(stock_contract::contract().version)!="1.1.8" && !stock_contract::is_v2()) ||
       !dingosdk::supported_build::running_image_matches(base)) {
        log("Game camera observer unavailable: spectator contract requires verified ReSkate 1.1.8, 2.0.0, 2.0.1, 2.0.2, 2.0.3, 2.0.4, 2.0.5, 2.0.6 or 2.0.7.");return;
    }
    std::array<unsigned char,32> live{};
    const bool native=dingosdk::memory::read(base+spectator_active,live) && live==spectator_prefix;
    if(!native && !resolve_stock_handler(base+spectator_active)) {
        log("Game camera observer unavailable: spectator query has an unverified owner.");return;
    }
    // Call the entry, not a trampoline: the stock hook supplies party state.
    spectator_query=reinterpret_cast<SpectatorActive>(base+spectator_active);
    log("Game camera observer installed: verified spectator query, stock freecam/drone flags, gameplay world and local view ownership. Replay transitions require live confirmation.");
}
void update_game_camera_ownership(std::uintptr_t client,std::uintptr_t base) {
    if(!spectator_query.load()) return;
    GameCameraEvidence evidence;
    try {
        using namespace dingosdk::client_source::detail;
        SourceReader reader;
        source_require(reader.pointer(client)==base+dingosdk::addr::engine::client_vtable,"Camera ownership client changed.");
        evidence.context=reader.pointer(client,8);
        const auto world=reader.value<std::uint32_t>(client,0xc4);
        reader.verify();
        evidence.valid=query_spectator(evidence.spectator);
        evidence.other_camera=stock_foreign.load() || (world!=13 && world!=21);
        if(evidence.valid && !evidence.spectator && !evidence.other_camera) {
            SourceTrial trial;trial.base=base;
            const auto camera=source_camera_snapshot(trial,client);
            std::uintptr_t type{};
            evidence.local_camera=camera.identity.context==evidence.context && camera.mode==0 && camera.view_enabled &&
                !camera.input_active && dingosdk::memory::read(camera.active,type) && type==base+dingosdk::addr::engine::camera_vtable;
            // Mode 1 is shared by native First Person/freecam/drone. Do not
            // classify that shared lease using a delayed activation snapshot:
            // the verified native debug flags above distinguish its owner.
            // This also preserves startup, activation and bail handovers.
            const bool shared_lease=camera.mode==1 && camera.active==camera.identity.camera;
            evidence.other_camera=!evidence.local_camera && !shared_lease;
        }
    } catch(...) { /* Missing evidence neither suspends nor restores. */ }
    bool changed{},active{};
    {
        std::lock_guard lock(ownership_mutex);
        const auto before=policy.reason;policy.observe(evidence);
        active=policy.suspended();changed=before!=policy.reason;
        suspended=active;managing=policy.managing;
    }
    if(changed) {
        suspend_custom_camera();
        clear_stock_camera_blends();
        log(active ? "Game camera ownership: another camera took over; releasing First Person.":
            "Game camera ownership: local gameplay camera verified; existing restrictions still apply.");
    }
}
}
