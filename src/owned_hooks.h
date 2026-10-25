#pragma once
#include "Engine/Core/Hooks/hooks.h"
#include <vector>
#include <mutex>
#include <string>
#include <string_view>
namespace dingosdk {
class OwnedHookLedger {
    std::vector<void*> targets_;
public:
    void remember(void* target) { targets_.push_back(target); }
    template<class Remove> bool rollback(Remove remove) {
        bool ok=true;std::vector<void*> retained;
        for(auto i=targets_.rbegin();i!=targets_.rend();++i) {
            if(remove(*i)!=HookOk) { retained.push_back(*i);ok=false; }
        }
        targets_=std::move(retained);return ok;
    }
    std::size_t size() const { return targets_.size(); }
};
inline std::mutex owned_hook_mutex;
inline OwnedHookLedger owned_hook_ledger;
inline HookStatus owned_hook_prepare(void* target,void* replacement,void** original=nullptr) {
    std::lock_guard lock(owned_hook_mutex);
    const auto status=hook_prepare(target,replacement,original);
    if(status==HookOk) owned_hook_ledger.remember(target);
    return status;
}
// The backend aborts failed transactions before returning. Only retry thread
// collection/enlistment races; never omit an inaccessible live thread.
inline thread_local unsigned owned_hook_retries{};
template<class Apply> inline HookStatus retry_owned_hooks(Apply apply) {
    owned_hook_retries=0;
    for(unsigned attempt=0;;++attempt) {
        const auto result=apply();
        const auto detail=hook_last_failure();
        const std::string_view operation(detail.operation);
        const bool thread_failure=operation=="open thread" || operation=="query thread owner" ||
            operation=="enumerate threads" || operation=="enlist thread";
        if(result==HookOk || attempt==39 || !thread_failure ||
            (detail.error!=ERROR_ACCESS_DENIED && detail.error!=ERROR_INVALID_PARAMETER)) return result;
        ++owned_hook_retries;
        Sleep(50); // No transaction is active and no thread remains suspended.
    }
}
inline HookStatus apply_owned_hooks() { return retry_owned_hooks([] {return hook_apply_queued();}); }
// Optional hooks must not leave a failed target queued for unrelated adapters.
inline HookStatus enable_owned_hook(void* target) { return retry_owned_hooks([&] {return hook_enable(target);}); }
inline std::string hook_failure_description(HookStatus result) {
    const auto detail=hook_last_failure();
    return std::to_string(result)+" ("+hook_status_string(result)+
        "; operation="+detail.operation+"; Win32="+std::to_string(detail.error)+
        "; thread="+std::to_string(detail.thread_id)+"; retries="+std::to_string(owned_hook_retries)+")";
}
inline bool rollback_owned_hooks() {
    std::lock_guard lock(owned_hook_mutex);
    return owned_hook_ledger.rollback([](void* target) { return hook_remove(target); });
}
inline bool restore_owned_slot(void* volatile* slot,void* replacement,void* original) {
    const auto current=InterlockedCompareExchangePointer(slot,original,replacement);
    return current==replacement || current==original;
}
}
