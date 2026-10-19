#include "Extension/UI/Overlay/overlay_internal.h"
#include "Extension/Multiplayer/Hud/native_party_internal.h"
#include "Extension/Multiplayer/Remote/native_skater_internal.h"
#include <cstdio>
#include <cstddef>
static_assert(offsetof(dingosdk::overlay::detail::State,callbacks)==0x5b0 && offsetof(dingosdk::overlay::detail::State,model)==0x5e0);
static_assert(offsetof(dingosdk::overlay::Model,bindings)==0x5fe0);
static_assert(int(dingosdk::overlay::DebugAction::reset_first_person_arm)==34);
int main() {
using namespace dingosdk::overlay;
printf("callbacks=%zx model=%zx debug=%zx bindings=%zx state_size=%zx\n",offsetof(detail::State,callbacks),offsetof(detail::State,model),offsetof(Model,debug),offsetof(Model,bindings),sizeof(detail::State));
using P=dingosdk::multiplayer::native_party_detail::State;
using R=dingosdk::multiplayer::native_skater_detail::Remote;
printf("party_size=%zx selected=%zx epoch=%zx published=%zx positions=%zx remote_size=%zx buffer=%zx generation=%zx\n",sizeof(P),offsetof(P,spectating),offsetof(P,spectating_epoch),offsetof(P,published),offsetof(P,positions),sizeof(R),offsetof(R,written_buffer),offsetof(R,generation));
}
