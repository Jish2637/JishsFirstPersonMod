// Compile against the official 2.1.0 source with MSVC /MT /std:c++20 and
// _ITERATOR_DEBUG_LEVEL=0. This verifies the POD fields read by the bridge.
#include "Extension/Multiplayer/Hud/native_party_internal.h"
#include "Extension/Multiplayer/Remote/native_skater_internal.h"
#include <cstddef>
using namespace dingosdk::multiplayer;
using namespace native_party_detail;
using native_skater_detail::Remote;
static_assert(offsetof(State,published)==0x10 && offsetof(State,spectating)==0x3850 && offsetof(State,spectating_epoch)==0x3858);
static_assert(offsetof(State,positions)==192 && sizeof(Position)==56 && offsetof(Position,sampled)==8);
static_assert(offsetof(Published,records)==8 && sizeof(Record)==152 && offsetof(Record,handle)==88);
static_assert(offsetof(PartyPlayer,id)==0 && offsetof(PartyPlayer,epoch)==8 && offsetof(PartyPlayer,local)==48 &&
    offsetof(PartyPlayer,present)==50 && offsetof(PartyPlayer,slot)==56);
static_assert(sizeof(Remote)==888 && offsetof(Remote,entity)==0 && offsetof(Remote,component)==8 &&
    offsetof(Remote,context)==24 && offsetof(Remote,written_buffer)==256 && offsetof(Remote,generation)==264);
static_assert(max_players==250);
int main() {}
