#include <Windows.h>
#include <atomic>
#include <iostream>
#include <stdexcept>

namespace {
std::atomic<unsigned> deny_count{};
std::atomic<bool> deny_forever{};
DWORD worker_id{};
HANDLE stop_event{};
HANDLE WINAPI open_test_thread(DWORD access,BOOL inherit,DWORD id) {
    if(id==worker_id && (access&THREAD_SET_CONTEXT) &&
       (deny_forever || deny_count.exchange(0))) {
        SetLastError(ERROR_ACCESS_DENIED);return nullptr;
    }
    return OpenThread(access,inherit,id);
}
DWORD WINAPI waiting_thread(void*) { WaitForSingleObject(stop_event,INFINITE);return 0; }
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
}

// Exercise the actual transaction backend with one injected OpenThread denial.
// Every other operation, including Detours attach/detach, is real.
#define OpenThread open_test_thread
#include "../vendor/Engine/Core/Hooks/hooks.cpp"
#undef OpenThread
#include "owned_hooks.h"

using Function=int(*)(int);
Function original{};
extern "C" __declspec(noinline) int target(int value) { return value*3+7; }
extern "C" __declspec(noinline) int replacement(int value) { return original(value)+100; }
Function volatile invoke=&target;

int main() {
    using namespace dingosdk;
    stop_event=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    const auto worker=CreateThread(nullptr,0,waiting_thread,nullptr,0,&worker_id);
    if(!stop_event || !worker) return 1;
    int result{};
    try {
        check(hook_initialize()==HookOk,"service init failed");
        void* relay{};
        check(owned_hook_prepare(reinterpret_cast<void*>(&target),reinterpret_cast<void*>(&replacement),&relay)==HookOk,"prepare failed");
        original=reinterpret_cast<Function>(relay);
        deny_forever=true;
        check(enable_owned_hook(reinterpret_cast<void*>(&target))==HookProtectionFailed,"optional hook did not fail safely");
        deny_forever=false;
        check(apply_owned_hooks()==HookOk && invoke(5)==22 && hook_statistics().attached==0,
            "failed optional hook leaked into an unrelated queued transaction");
        check(hook_queue_enable(reinterpret_cast<void*>(&target))==HookOk,"queue failed");
        deny_count=1;
        check(apply_owned_hooks()==HookOk && owned_hook_retries==1,"transient thread denial was not retried");
        check(invoke(5)==122,"retried hook did not execute");
        check(hook_disable(reinterpret_cast<void*>(&target))==HookOk && invoke(5)==22,"detach failed");
        check(hook_queue_enable(reinterpret_cast<void*>(&target))==HookOk,"requeue failed");
        deny_forever=true;
        check(apply_owned_hooks()==HookProtectionFailed && owned_hook_retries==39,"persistent denial was not bounded");
        const auto detail=hook_last_failure();
        check(std::string_view(detail.operation)=="open thread" && detail.error==ERROR_ACCESS_DENIED && detail.thread_id==worker_id,"failure operation/thread was lost");
        check(hook_failure_description(HookProtectionFailed).find("operation=open thread")!=std::string::npos,"diagnostic lacks failure stage");
        check(invoke(5)==22 && hook_statistics().attached==0,"inaccessible live thread was skipped or target patched");
        deny_forever=false;
        check(apply_owned_hooks()==HookOk && invoke(5)==122,"queued hook could not recover after denied thread became accessible");
        check(rollback_owned_hooks() && invoke(5)==22,"rollback failed");
        std::cout<<"Transient access retry, bounded live-thread refusal, recovery and rollback passed\n";
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';deny_forever=false;rollback_owned_hooks();result=1;
    }
    SetEvent(stop_event);WaitForSingleObject(worker,5000);CloseHandle(worker);CloseHandle(stop_event);
    return result;
}
