#include "settings.h"
#include "menu.h"
#include "native_recording.h"
#include "native_211.h"
#include "spectator_contract.h"
#include "owned_hooks.h"
#include "stock_contract.h"
#include "Engine/Core/Platform/memory.h"
#include "Extension/UI/Overlay/playstation_input.h"
#include <algorithm>
#include <vector>

namespace jishsfirstpersonmod {
namespace {
NativeRecordingReader original_reader{};
struct StockGamepad { bool valid{},playstation{};float axes[6]{};unsigned buttons{}; };
static_assert(sizeof(StockGamepad)==32);
using ReadGamepad=StockGamepad*(*)(StockGamepad*);
ReadGamepad original_gamepad{};
native211::RecordingCapture reader_capture,gamepad_capture;
void captured_reader(NativeRecordingInput* output,bool allow_menu) {
    original_reader(output,allow_menu);
    if(output) {
        const bool held=output->buttons || std::any_of(output->keys.begin(),output->keys.end(),[](auto key){return key!=0;});
        if(reader_capture.consume(raw_controller_read,panel_recording_active(),held)) *output={};
    }
}
StockGamepad* captured_gamepad(StockGamepad* output) {
    auto* result=original_gamepad(output);
    if(output) {
        const bool held=output->buttons || std::any_of(std::begin(output->axes),std::end(output->axes),[](float value){return std::abs(value)>.25f;});
        if(gamepad_capture.consume(raw_controller_read,panel_recording_active(),held)) *output={};
    }
    return result;
}
NativeRecordingReader native_reader() {
    using namespace stock_contract;
    if(!uses_v211_layout(selected)) return nullptr;
    const auto stock=GetModuleHandleW(L"reskate.dll");
    const auto base=reinterpret_cast<std::uintptr_t>(stock);
    if(!stock || !spectator::signature(dingosdk::memory::read_bytes,base+native211::controller.rva,native211::controller.prefix) ||
        !spectator::signature(dingosdk::memory::read_bytes,base+native211::gamepad.rva,native211::gamepad.prefix)) return nullptr;
    void* reader{},*gamepad{};
    using namespace dingosdk;
    if(owned_hook_prepare(reinterpret_cast<void*>(base+native211::controller.rva),reinterpret_cast<void*>(&captured_reader),&reader)!=HookOk ||
       owned_hook_prepare(reinterpret_cast<void*>(base+native211::gamepad.rva),reinterpret_cast<void*>(&captured_gamepad),&gamepad)!=HookOk) return nullptr;
    original_reader=reinterpret_cast<NativeRecordingReader>(reader);original_gamepad=reinterpret_cast<ReadGamepad>(gamepad);
    if(hook_queue_enable(reinterpret_cast<void*>(base+native211::controller.rva))!=HookOk ||
       hook_queue_enable(reinterpret_cast<void*>(base+native211::gamepad.rva))!=HookOk) return nullptr;
    log("Verified 2.1.2 controller readers: stock navigation; explicit recording captures only while focused.");
    return original_reader;

}
}
// Both verified readers are queued during bootstrap, before the common hook
// transaction commits. Failure aborts startup instead of using an unsafe fallback.
bool initialize_panel_recording() { return native_reader()!=nullptr; }
dingosdk::ControllerInput recording_controller() {
    return original_reader ? read_native_recording(original_reader):dingosdk::ControllerInput{};
}
GameplayInput menu_controller() {
    RecordingInputAccess access;
    auto input=read_controller(true);
    static_cast<dingosdk::ControllerInput&>(input)=recording_controller();
    return input;
}
}
