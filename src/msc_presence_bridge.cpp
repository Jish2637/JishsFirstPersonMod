#include "msc_presence_bridge.h"
#include "msc_presence_policy.h"
#include "msc_server_discovery.h"
#include "stock_bridge.h"
#include "stock_contract.h"
#include "comfort_policy.h"
#include "spectator_runtime.h"
#include "owned_hooks.h"
#include "bridge_resolver.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/image_identity.h"
#include <array>
#include <cstring>
#include <mutex>
#include <stdexcept>
namespace jishsfirstpersonmod {
namespace {
// Exact official v2.1.3 only. Offsets reproduced from the pinned upstream
// Session layout; native chat() independently confirms mutex and chat_view.
std::uintptr_t base{};
using NativeMutex=int(*)(void*);
using Queue=bool(*)(const char*,const char*,const char*,char*,std::size_t);
struct View {std::uintptr_t data{};std::size_t size{};};
static_assert(sizeof(View)==16);
using Command=void*(*)(void*,const View*,const View*,const View*);
Command original_command{};
using SendCommand=void*(*)(void*,std::uintptr_t,const View*);
SendCommand native_send{};
NativeMutex native_lock{},native_unlock{};
std::mutex guard;
msc_presence::Gate gate;
struct Pending {msc_presence::Session session;std::string command;bool enabled{};};
Pending pending;
std::uint64_t next_attempt{};
std::uint32_t serial{};
template<class T>T read(std::uintptr_t address) {
    T value{};if(!dingosdk::memory::read(address,value))throw std::runtime_error("MSC presence snapshot unreadable");return value;
}
std::string text(std::uintptr_t address) {
    const auto size=read<std::size_t>(address+16),capacity=read<std::size_t>(address+24);
    if(size>4096 || capacity<size || (capacity<16 && size>15))throw std::runtime_error("MSC presence string rejected");
    std::string result(size,'\0');
    if(size && !dingosdk::memory::read_bytes(capacity<16 ? address:read<std::uintptr_t>(address),result.data(),size))
        throw std::runtime_error("MSC presence string unreadable");
    return result;
}
std::string view_text(const View* address) {
    const auto view=read<View>(reinterpret_cast<std::uintptr_t>(address));
    if(view.size>256)throw std::runtime_error("MSC command view rejected");
    std::string result(view.size,'\0');
    if(view.size && !dingosdk::memory::read_bytes(view.data,result.data(),view.size))throw std::runtime_error("MSC command unreadable");
    return result;
}
// No native STL object is copied, constructed or freed by the companion.
// Only bounded POD/string-byte reads under the stock session's own mutex.
msc_presence::Session sample() {
    const auto session=read<std::uintptr_t>(base+0x1ba0010);
    if(!session) {std::lock_guard lock(guard);gate.reset();return {};}
    auto* mutex=reinterpret_cast<void*>(session+783584);
    if(native_lock(mutex)!=0)throw std::runtime_error("MSC session mutex unavailable");
    struct Unlock {void* mutex;~Unlock(){native_unlock(mutex);}} unlock{mutex};
    const auto mode=read<unsigned>(session+780680);
    msc_presence::Session key{read<std::uint64_t>(session+780736),read<std::uint64_t>(session+780720),read<std::uint64_t>(session+780712)};
    bool authenticated=false;
    const auto name=text(session+781352+592);
    const auto slots=read<std::size_t>(session+779680);
    if(slots>249)throw std::runtime_error("MSC peer count rejected");
    if(mode==2 && key.host && read<std::uint8_t>(session+781352+324)==1 && read<std::uint8_t>(session+781352+802)==1) {
        for(std::size_t i=0;i<slots;++i) {
            const auto peer=session+2800+i*3120;
            if(read<std::uint64_t>(peer+552)==key.host) {
                key.host_epoch=read<std::uint64_t>(peer+560);
                authenticated=read<std::uint8_t>(peer+728)==1;
                break;
            }
        }
    } else key={};
    if(read<unsigned>(session+780680)!=mode || read<std::uint64_t>(session+780736)!=key.host ||
       read<std::uint64_t>(session+780720)!=key.epoch)key={};
    std::lock_guard lock(guard);
    gate.observe(key,authenticated && key.host_epoch!=0,name);
    return key;
}
void* empty_native_string(void* result) {
    // MSVC release std::string's empty SSO representation; no allocation crosses DLLs.
    std::array<std::byte,32> empty{};const std::uint64_t capacity=15;
    std::memcpy(empty.data()+24,&capacity,8);std::memcpy(result,empty.data(),empty.size());return result;
}
void* command_hook(void* result,const View* action,const View* argument,const View* password) {
    std::string command;
    try {
        if(view_text(action)=="chat")command=view_text(argument);
    } catch(...) {return original_command(result,action,argument,password);}
    if(!command.starts_with("/jish-fp"))return original_command(result,action,argument,password);
    msc_presence::Session key;bool enabled{};
    try {
        // Re-check at actual dispatch, not only enqueue: queued reports must not
        // leak into a different server after a stop/join command runs first.
        key=sample();bool allowed{};
        {
            std::lock_guard lock(guard);
            allowed=msc_presence::dispatch_allowed(gate,key,pending.session,command,pending.command);
            enabled=pending.enabled;pending={};next_attempt=GetTickCount64()+5000;
        }
        if(!allowed)return empty_native_string(result);
    } catch(...) {return empty_native_string(result);}
    // Use ReSkate's existing send_chat_command implementation, including its
    // normal packet builder, routing and rate limits. Skip the higher-level
    // command() wrapper which would display failures in chat.
    const auto session=read<std::uintptr_t>(base+0x1ba0010);
    const auto returned=native_send(result,session,argument);
    try {
        if(text(reinterpret_cast<std::uintptr_t>(returned)).empty()) {
            std::lock_guard lock(guard);
            if(gate.session()==key && gate.active())gate.sent(enabled,GetTickCount64());
        }
    } catch(...) {}
    return returned;
}
DWORD WINAPI worker(void*) noexcept {
    msc_presence::Discovery discovery;
    bool attempted{},attempt_value{};
    msc_presence::Session attempt_session;
    for(;;) {
        try {
            if(!compatibility::enabled()) {Sleep(250);continue;}
            const auto key=sample();const auto now=GetTickCount64();
            bool eligible{};{std::lock_guard lock(guard);eligible=gate.eligible();}
            const bool supported=eligible && discovery.update(key,now);
            if(!eligible)discovery.reset();
            {std::lock_guard lock(guard);gate.capability(key,supported);}
            const auto comfort=comfort_snapshot();
            const bool enabled=msc_presence::enabled_setting(comfort.initialized,comfort.master,stock_first_person_requested(),spectator_first_person_enabled());
            std::string command;
            {
                std::lock_guard lock(guard);
                if(attempt_session!=key) {pending={};next_attempt=0;attempted=false;attempt_session=key;}
                if(!pending.command.empty() && now>=next_attempt)pending={};
                const auto due=gate.due(enabled,now);
                if(due && pending.command.empty() && (now>=next_attempt || (attempted && enabled!=attempt_value))) {
                    command=std::string("/jish-fp ")+(*due ? "1 ":"0 ")+std::to_string(++serial);
                    pending={key,command,*due};next_attempt=now+10000;
                    attempted=true;attempt_value=*due;
                }
            }
            if(!command.empty()) {
                char response[256]{};
                if(!reinterpret_cast<Queue>(base+0x9895b0)("chat",command.c_str(),"",response,sizeof(response))) {
                    std::lock_guard lock(guard);if(pending.command==command)pending={};next_attempt=now+5000;
                }
            }
        } catch(...) {std::lock_guard lock(guard);gate.reset();pending={};next_attempt=GetTickCount64()+5000;}
        Sleep(100);
    }
}
template<std::size_t N>bool bytes(std::uintptr_t address,const std::array<unsigned char,N>& expected) {
    std::array<unsigned char,N> actual{};return dingosdk::memory::read(address,actual) && actual==expected;
}
}
void start_msc_presence(HMODULE stock) noexcept {
    try {
        wchar_t path[32768]{};if(!GetModuleFileNameW(stock,path,32768))return;
        const auto file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
        std::array<unsigned char,32> hash{};const auto hashed=dingosdk::supported_build::file_sha256(file,hash);
        if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);
        base=reinterpret_cast<std::uintptr_t>(stock);
        const auto* profile=stock_contract::find_contract(hash,reinterpret_cast<std::uintptr_t>(GetProcAddress(stock,"DingoSDKOverlayGetStatus"))-base);
        if(!hashed || !profile || std::string_view(profile->version)!="2.1.3")throw std::runtime_error("exact official 2.1.3 required");
        // Official binary + pinned Session layout. Verify entry prefixes and
        // the session getter's RIP-relative pointer before using private ABI.
        if(!bytes(base+0xa205f0,std::array<unsigned char,16>{0x40,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0xb8,0xf0,0x12,0x0}) ||
           !bytes(base+0x9895b0,std::array<unsigned char,16>{0x48,0x8b,0xc4,0x53,0x41,0x57,0x48,0x83,0xec,0x58,0x48,0x89,0x68,0x8,0x4d,0x8b}) ||
           !bytes(base+0x9fce20,std::array<unsigned char,16>{0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57}) ||
           !bytes(base+0x9d6e57,std::array<unsigned char,7>{0x48,0x8b,0x5,0xb2,0x91,0x1c,0x1}))
            throw std::runtime_error("native presence contracts differ");
        native_send=reinterpret_cast<SendCommand>(base+0x9fce20);
        native_lock=reinterpret_cast<NativeMutex>(base+profile->mutex_lock_rva);
        native_unlock=reinterpret_cast<NativeMutex>(base+profile->mutex_unlock_rva);
        void* original{};
        if(dingosdk::owned_hook_prepare(reinterpret_cast<void*>(base+0xa205f0),reinterpret_cast<void*>(&command_hook),&original)!=dingosdk::HookOk)
            throw std::runtime_error("presence dispatch guard unavailable");
        original_command=reinterpret_cast<Command>(original);
        if(dingosdk::enable_owned_hook(reinterpret_cast<void*>(base+0xa205f0))!=dingosdk::HookOk)
            throw std::runtime_error("presence dispatch hook failed");
        const auto thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);
        if(!thread)throw std::runtime_error("presence worker unavailable");CloseHandle(thread);
        log("MSC presence ready: authenticated MSC prefix + matching Steam capability metadata required; silent dispatch guarded.");
    } catch(const std::exception& error) {
        log(std::string("MSC presence unavailable: ")+error.what());
    } catch(...) {log("MSC presence unavailable: optional bridge initialization failed.");}
}
}
