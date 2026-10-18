#pragma once
#include <cstdint>
#include <optional>

namespace jishsfirstpersonmod {
enum class GameCameraReason { none, spectator, other_camera };
// Unknown evidence never creates or clears a restriction. A return must be
// positively verified, independently of movement and comfort restrictions.
struct GameCameraEvidence {
    std::uintptr_t context{};
    bool valid{}, spectator{}, local_camera{};
    bool other_camera{};
};
struct GameCameraOwnershipPolicy {
    GameCameraReason reason{GameCameraReason::none};
    bool managing{};
    std::uintptr_t context{};
    void observe(const GameCameraEvidence& evidence) {
        if(!evidence.valid || !evidence.context) return;
        if(evidence.spectator) {
            reason=GameCameraReason::spectator;managing=true;context=evidence.context;
        } else if(evidence.other_camera) {
            reason=GameCameraReason::other_camera;managing=true;context=evidence.context;
        } else if(evidence.local_camera) {
            reason=GameCameraReason::none;context=evidence.context;
        }
    }
    bool suspended() const { return reason!=GameCameraReason::none; }
    std::optional<bool> effective(std::optional<bool> existing) const {
        return suspended() ? std::optional<bool>{false}:existing;
    }
    void acknowledge(bool master,bool active) {
        if(!suspended() && (!master || active)) managing=false;
    }
};
bool game_camera_suspended();
void observe_stock_camera_owner(bool foreign);
bool game_camera_managing();
void acknowledge_game_camera(bool master,bool active);
void update_game_camera_ownership(std::uintptr_t client,std::uintptr_t base);
void install_game_camera_observer(std::uintptr_t base);
const char* game_camera_status();
}
