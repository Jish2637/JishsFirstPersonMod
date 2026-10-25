#pragma once
#include "emote_aim.h"
namespace jishsfirstpersonmod {
void install_emote_aim(std::uintptr_t base) noexcept;
void publish_emote_camera(const emote_aim::Matrix&,emote_aim::Owner,bool eligible) noexcept;
}
