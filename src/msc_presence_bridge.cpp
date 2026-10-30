#include "msc_presence_bridge.h"
#include "mod.h"
namespace jishsfirstpersonmod {
void start_msc_presence(HMODULE) noexcept {
    // Studio's dispatcher is not part of this reviewed camera profile.
    log("MSC presence disabled for Animator Studio test profile: command dispatch has not been audited.");
}
}
