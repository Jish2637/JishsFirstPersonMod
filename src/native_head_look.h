#pragma once
namespace jishsfirstpersonmod {
// Called only after existing live owner/focus/menu/device/camera checks pass.
inline bool native_head_look_passthrough(bool enabled,bool active_aim,bool stick_enabled,bool compatibility,
                                         bool turn_selected,bool turning,bool recovery) {
    return enabled && active_aim && stick_enabled && (compatibility || !turn_selected) && !turning && !recovery;
}
}
