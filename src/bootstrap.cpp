#include "mod.h"
#include "Engine/Game/Build/image_identity.h"
#include "Extension/UI/Overlay/overlay.h"
#include "settings.h"
#include "stock_bridge.h"
#include "bridge_resolver.h"
#include "owned_hooks.h"
#include "launcher_mod.h"
#include "diagnostic_log.h"
#include "user_data.h"
#include <shellapi.h>
#include <atomic>
#include <fstream>
#include <mutex>
#include <sstream>
#include <TlHelp32.h>

namespace {
HMODULE self_module{};
volatile LONG started{};
std::atomic<int> status{};
std::atomic<bool> toggle_request{}, reload_request{};
std::filesystem::path directory;
using GetOverlayStatus = void (*)(dingosdk::overlay::Status*);
// ABI copied from stock 1.1.4; this is the only ReSkate function called.
GetOverlayStatus overlay_status{};
}

namespace jishsfirstpersonmod {
std::filesystem::path module_directory() { return directory; }
std::filesystem::path data_directory() {
    return scripts_data_directory(directory);
}
void log(const std::string& text) noexcept {
    try {write_diagnostic_log(data_directory()/L"logs",text);} catch(...) {}
}
bool take_toggle_request() noexcept { return toggle_request.exchange(false); }
bool take_reload_request() noexcept { return reload_request.exchange(false); }
bool overlay_available(bool& visible) noexcept {
    if (!overlay_status) return false;
    dingosdk::overlay::Status s{};
    overlay_status(&s);
    visible = s.visible;
    return s.started && s.ready && !s.failed && !s.stopping;
}
}

namespace {
bool host_matches() {
    wchar_t path[32768]{};
    const auto size = GetModuleFileNameW(nullptr, path, 32768);
    if (!size || size >= 32768) return false;
    return _wcsicmp(std::filesystem::path(path).filename().c_str(), L"Skate.exe") == 0;
}
bool protected_module_present() {
    const auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE) return true;
    MODULEENTRY32W e{}; e.dwSize = sizeof(e);
    bool reject = !Module32FirstW(snap, &e);
    if (!reject) do {
        if (_wcsnicmp(e.szModule, L"eaanticheat", 11) == 0) { reject = true; break; }
    } while (Module32NextW(snap, &e));
    CloseHandle(snap);
    return reject;
}
DWORD WINAPI worker(void*) noexcept {
    try {
        wchar_t path[32768]{};
        if (!GetModuleFileNameW(self_module, path, 32768)) { status = 3; return 0; }
        directory = std::filesystem::path(path).parent_path();
        jishsfirstpersonmod::log("version " JISH_MOD_VERSION " initialisation worker started; native setup runs outside DllMain");
        if (!host_matches()) { jishsfirstpersonmod::log("ERROR: host is not Skate.exe; no hooks installed"); status = 3; return 0; }
        wchar_t game_path[32768]{};
        if(!GetModuleFileNameW(nullptr,game_path,32768)) { status=3;return 0; }
        auto data_root=std::filesystem::path(game_path).parent_path();
        int argument_count{};
        if(auto arguments=CommandLineToArgvW(GetCommandLineW(),&argument_count)) {
            for(int i=1;i<argument_count;++i) {
                const std::wstring argument(arguments[i]);
                if(_wcsicmp(argument.c_str(),L"-dataPath")==0 && i+1<argument_count) { data_root/=arguments[i+1];break; }
                if(argument.size()>10 && _wcsnicmp(argument.c_str(),L"-dataPath=",10)==0) { data_root/=argument.substr(10);break; }
            }
            LocalFree(arguments);
        }
        if(!jishsfirstpersonmod::launcher_mod_enabled(data_root/L"Mods")) {
            jishsfirstpersonmod::log("Disabled in the ReSkate launcher; no hooks installed.");status=3;return 0;
        }
        const auto reskate = GetModuleHandleW(L"reskate.dll");
        if (!reskate) { jishsfirstpersonmod::log("ERROR: reskate.dll is not loaded; no hooks installed"); status = 3; return 0; }
        if(GetModuleHandleW(L"MyMod.dll")) { jishsfirstpersonmod::log("ERROR: previous MyMod.dll is loaded; remove its Lua bootstrap and restart."); status=3; return 0; }
        if (protected_module_present()) { jishsfirstpersonmod::log("ERROR: protected module detected; no hooks installed"); status = 3; return 0; }
        const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        if (!dingosdk::supported_build::running_image_matches(base)) {
            jishsfirstpersonmod::log("ERROR: Skate.exe differs from supported build 25414733 (20260929); no hooks installed"); status = 3; return 0;
        }
        if(!jishsfirstpersonmod::compatibility::initialize(reskate)) { status=3;return 0; }
        overlay_status = reinterpret_cast<GetOverlayStatus>(GetProcAddress(reskate, "DingoSDKOverlayGetStatus"));
        if (!overlay_status) { jishsfirstpersonmod::log("ERROR: stock overlay status export unavailable; no hooks installed"); status = 3; return 0; }
        // Custom Lua executes during startup. Do not initialize graphics/input
        // or attach game hooks until stock has a stable, ready swapchain.
        jishsfirstpersonmod::log("waiting for ReSkate graphics readiness; no hooks installed yet");
        ULONGLONG ready_since{};
        bool ready{};
        for (unsigned tries=0; tries<6000; ++tries) {
            bool visible{};
            if (jishsfirstpersonmod::overlay_available(visible)) {
                if (!ready_since) ready_since=GetTickCount64();
                if (GetTickCount64()-ready_since>=3000) { ready=true; break; }
            } else ready_since=0;
            Sleep(100);
        }
        if (!ready) {
            jishsfirstpersonmod::log("ERROR: graphics readiness timed out after ten minutes; no hooks installed");
            status=3; return 0;
        }
        jishsfirstpersonmod::log("graphics ready; waiting for loaded-world hooks");
        HMODULE pinned{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&worker), &pinned)) {
            jishsfirstpersonmod::log("ERROR: module pin failed; no hooks installed"); status = 3; return 0;
        }
        if(!jishsfirstpersonmod::initialize_settings()) { status=3; return 0; }
        jishsfirstpersonmod::prepare_input();
        struct Rollback {
            bool committed{};
            ~Rollback() {
                if(committed) return;
                jishsfirstpersonmod::compatibility::disable();
                jishsfirstpersonmod::stop_stock_bridge();
                if(!jishsfirstpersonmod::restore_stock_camera_feed()) jishsfirstpersonmod::log("Rollback: camera callback changed owner; left replacement untouched.");
                if(!dingosdk::rollback_owned_hooks()) jishsfirstpersonmod::log("ERROR: some owned hooks could not detach; feature gates remain inactive.");
            }
        } rollback;
        if(!jishsfirstpersonmod::start_stock_bridge(reskate)) { status=3;return 0; }
        if (!jishsfirstpersonmod::install_hooks(base)) {
            jishsfirstpersonmod::publish_runtime(false,false,"Camera hooks failed. See JishsFirstPersonMod.log."); status = 3; return 0;
        }
        if(!jishsfirstpersonmod::start_menu()) { status=3;return 0; }
        jishsfirstpersonmod::compatibility::enable();rollback.committed=true;
        status = 2;
        jishsfirstpersonmod::log("initialised once; controller requests feed the existing native ReSkate camera");
    } catch (const std::exception& e) { jishsfirstpersonmod::log(std::string("ERROR: initialisation failed: ") + e.what()); status = 3; }
      catch (...) { jishsfirstpersonmod::log("ERROR: initialisation failed with unknown exception"); status = 3; }
    return 0;
}
void ensure_started() noexcept {
    if (InterlockedCompareExchange(&started, 1, 0) != 0) return;
    status = 1;
    const auto thread = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
    else { status = 3; InterlockedExchange(&started, 0); }
}
}
extern "C" __declspec(dllexport) int luaopen_jishsfirstpersonmod(void*) noexcept { ensure_started(); return 0; }
extern "C" __declspec(dllexport) int luaopen_jishsfirstpersonmod_toggle(void*) noexcept { toggle_request = true; return 0; }
extern "C" __declspec(dllexport) int luaopen_jishsfirstpersonmod_reload(void*) noexcept { reload_request = true; return 0; }
extern "C" __declspec(dllexport) int JishsFirstPersonModInitializationStatus() noexcept { return status.load(); }
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        self_module = module;
        DisableThreadLibraryCalls(module);
        ensure_started();
    }
    return TRUE;
}
