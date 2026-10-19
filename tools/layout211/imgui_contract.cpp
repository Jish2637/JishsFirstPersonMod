#include <imgui.h>
extern "C" __declspec(dllexport) void* context(){ return ImGui::GetCurrentContext(); }
extern "C" __declspec(dllexport) void allocators(ImGuiMemAllocFunc* a,ImGuiMemFreeFunc* f,void** u){ ImGui::GetAllocatorFunctions(a,f,u); }
extern "C" __declspec(dllexport) void* allocate(size_t n){return ImGui::MemAlloc(n);}
extern "C" __declspec(dllexport) void release(void* p){ImGui::MemFree(p);}
extern "C" __declspec(dllexport) ImGuiIO* io(){return &ImGui::GetIO();}
