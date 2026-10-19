#pragma once
#include "mod.h"
#include <algorithm>
namespace jishsfirstpersonmod {
struct BindingConflicts {
    bool toggle{};
    std::array<bool,4> glance{};
    bool any() const {
        return toggle || std::any_of(glance.begin(),glance.end(),[](bool conflict) { return conflict; });
    }
};
inline BindingConflicts find_binding_conflicts(const Config& config) {
    BindingConflicts result;
    const auto external=[&](std::uint32_t combo) {
        if(!combo) return false;
        if(config.native.active) return native_binding_overlap(combo,config.native);
        return std::find(config.reserved.begin(),config.reserved.end(),combo)!=config.reserved.end();
    };
    result.toggle=external(config.toggle);
    for(unsigned i=0;i<4;++i) {
        const auto combo=config.look[i].combo;
        result.glance[i]=external(combo);
        if(combo && combo==config.toggle) result.toggle=result.glance[i]=true;
    }
    return result;
}
struct ControllerInputRoute {
    bool stick_blocked{},toggle_blocked{};
    dingosdk::FirstPersonLookControls glance;
};
inline ControllerInputRoute route_controller_input(const Config& config,bool blocked) {
    const auto conflicts=find_binding_conflicts(config);
    ControllerInputRoute route{blocked,blocked || conflicts.toggle,config.look};
    for(unsigned i=0;i<4;++i) if(conflicts.glance[i]) route.glance[i].combo=0;
    return route;
}
struct ToggleEmoteBindings { std::uint32_t companion{},native{};bool enabled() const {return companion || native;} };
inline ToggleEmoteBindings toggle_emote_bindings(const Config& config,bool blocked) {
    if(blocked) return {};
    // Verified 2.1.x ControllerBindingsModel: nine tool/vote chords, then
    // action_combos[0]=menu and action_combos[1]=first_person (POD byte 44).
    const auto native=config.native.active ? config.native.bindings[10]:0;
    return {route_controller_input(config,false).toggle_blocked ? 0:config.toggle,
        dingosdk::valid_controller_combo(native) ? native:0};
}
}
