#include "Extension/Multiplayer/Session/session_internal.h"
#include "Extension/UI/Overlay/overlay_internal.h"
#include "Extension/Multiplayer/Hud/native_party_internal.h"
#include "Extension/Multiplayer/Remote/native_skater_internal.h"
#include "Extension/Skater/client_source_spawn_internal.h"
#include <iostream>
#define FIELD(T,F) std::cout<<#T "." #F "="<<offsetof(T,F)<<"\n"
int main(){
using namespace dingosdk;using namespace dingosdk::multiplayer::session_detail;
using O=dingosdk::overlay::detail::State;using M=dingosdk::overlay::Model;using D=dingosdk::overlay::DebugModel;
static_assert(offsetof(O,callbacks)==1456 && offsetof(O,model)==1504);
static_assert(offsetof(M,debug)==128 && offsetof(M,bindings)==24544 && offsetof(M,revisions)==26120 && sizeof(M)==26152);
static_assert(offsetof(D,drone_follow)==5 && offsetof(D,first_person_arm)==16 && offsetof(D,status)==424);
static_assert(offsetof(Session,mutex)==779528 && offsetof(Session,mode)==776624 && offsetof(Session,epoch)==776664);
static_assert(offsetof(Session,host_id)==776680 && offsetof(Session,secret)==776656 && offsetof(Session,used_slots)==775640);
static_assert(offsetof(Session,view)==777296 && offsetof(Session,peers)==2744 && sizeof(Peer)==3104);
static_assert(offsetof(Peer,member)==552 && offsetof(Peer,handshaken)==728);
static_assert(static_cast<int>(overlay::DebugAction::set_first_person)==13 && static_cast<int>(overlay::DebugAction::reset_first_person_arm)==34);
static_assert(sizeof(overlay::Gamepad)==32 && offsetof(overlay::Gamepad,buttons)==28);
static_assert(static_cast<int>(Mode::join)==2);
static_assert(offsetof(ControllerBindingsModel,freecam_controller_combo)==4 && offsetof(ControllerBindingsModel,action_combos)==40);
static_assert(std::tuple_size_v<decltype(ControllerBindingsModel::action_combos)> ==9);
FIELD(O,callbacks);FIELD(O,model);FIELD(O,render_mutex);FIELD(M,debug);FIELD(M,bindings);FIELD(M,multiplayer);FIELD(M,menu_fullscreen);
FIELD(M,revisions);using V=decltype(M::revisions);FIELD(V,debug);std::cout<<"Model.size="<<sizeof(M)<<"\nGamepad.size="<<sizeof(overlay::Gamepad)<<"\n";
FIELD(D,first_person);FIELD(D,drone_follow);FIELD(D,first_person_fov);FIELD(D,first_person_arm);FIELD(D,camera_unavailable);FIELD(D,status);
using P=dingosdk::multiplayer::native_party_detail::State;using R=dingosdk::multiplayer::native_skater_detail::Remote;
FIELD(P,spectating);FIELD(P,spectating_epoch);FIELD(P,published);FIELD(P,positions);std::cout<<"Party.size="<<sizeof(P)<<"\n";
FIELD(R,written_buffer);FIELD(R,generation);std::cout<<"Remote.size="<<sizeof(R)<<"\n";
FIELD(Session,mutex);FIELD(Session,mode);FIELD(Session,epoch);FIELD(Session,host_id);FIELD(Session,secret);
FIELD(Session,used_slots);FIELD(Session,view);FIELD(Session,chat_view);FIELD(Session,peers);
FIELD(Peer,member);FIELD(Peer,handshaken);std::cout<<"Peer.size="<<sizeof(Peer)<<"\n";
FIELD(multiplayer::Member,id);FIELD(multiplayer::Member,epoch);FIELD(multiplayer::Member,name);
FIELD(MultiplayerModel,dedicated);FIELD(MultiplayerModel,connected);FIELD(MultiplayerModel,lobby_name);
FIELD(MultiplayerChat,lines);FIELD(MultiplayerChat,latest);
FIELD(MultiplayerChatLine,sequence);FIELD(MultiplayerChatLine,received);FIELD(MultiplayerChatLine,sender);
FIELD(MultiplayerChatLine,text);FIELD(MultiplayerChatLine,server);FIELD(MultiplayerChatLine,local);
std::cout<<"MultiplayerChatLine.size="<<sizeof(MultiplayerChatLine)<<"\n";
}
