#pragma once
#include "spectator_view.h"
#include "mod.h"
namespace jishsfirstpersonmod {
using SpectatorRenderWriter=bool(*)(std::uintptr_t client,std::uintptr_t camera,const spectator::Matrix& pose,float fov);
void initialize_spectator(std::uintptr_t game,std::uintptr_t stock,SpectatorRenderWriter writer=nullptr);
bool spectator_available() noexcept;
bool spectator_first_person_enabled();
std::string spectator_status();
void update_spectator_session(bool watching,const Config& config,std::uintptr_t context,bool interrupted,std::uintptr_t client=0);
bool toggle_spectator_view();
void update_spectator_input(const Config&,const GameplayInput&,bool blocked);
StickInputPolicy spectator_input_policy(unsigned slot=~0u);
bool spectator_transform_call(std::uintptr_t caller) noexcept;
bool spectator_fov_call(std::uintptr_t caller) noexcept;
bool spectator_transform(std::uintptr_t camera,const spectator::Matrix& normal,spectator::Matrix& output);
float spectator_fov(std::uintptr_t camera,float normal);
void reset_spectator_view();
}
