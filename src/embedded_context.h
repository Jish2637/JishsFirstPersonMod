#pragma once
#include <imgui.h>
namespace jishsfirstpersonmod {
// ImGui globals belong to each DLL. Host allocations must keep the host owner.
class ScopedHostImGui {
    ImGuiContext* previous_;ImGuiMemAllocFunc alloc_;ImGuiMemFreeFunc free_;void* data_;
public:
    ScopedHostImGui(ImGuiContext* context,ImGuiMemAllocFunc alloc,ImGuiMemFreeFunc free,void* data):previous_(ImGui::GetCurrentContext()) {
        ImGui::GetAllocatorFunctions(&alloc_,&free_,&data_);
        ImGui::SetAllocatorFunctions(alloc,free,data);ImGui::SetCurrentContext(context);
    }
    ~ScopedHostImGui() { ImGui::SetCurrentContext(previous_);ImGui::SetAllocatorFunctions(alloc_,free_,data_); }
    ScopedHostImGui(const ScopedHostImGui&)=delete;
    ScopedHostImGui& operator=(const ScopedHostImGui&)=delete;
};
}
