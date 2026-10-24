#include <imgui_internal.h>
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
FIELD(Session,chat_view);FIELD(M,menu_scale);std::cout<<"DebugAction.fp="<<int(dingosdk::overlay::DebugAction::set_first_person)<<"\n";FIELD(D,camera_unavailable);FIELD(D,status);FIELD(O,context);FIELD(O,menu);FIELD(O,window);using S=overlay::SkateMenu;FIELD(S,page);FIELD(S,skater_tab);FIELD(S,last_menu_frame);FIELD(S,bold);FIELD(S,heading);std::cout<<"ImGuiContext.size="<<sizeof(ImGuiContext)<<"\nImGuiIO.size="<<sizeof(ImGuiIO)<<"\nImDrawVert.size="<<sizeof(ImDrawVert)<<"\nImDrawIdx.size="<<sizeof(ImDrawIdx)<<"\n";FIELD(O,callbacks);FIELD(O,model);FIELD(O,render_mutex);FIELD(M,debug);FIELD(M,bindings);FIELD(M,multiplayer);FIELD(M,menu_fullscreen);
FIELD(M,revisions);using V=decltype(M::revisions);FIELD(V,debug);std::cout<<"Model.size="<<sizeof(M)<<"\nGamepad.size="<<sizeof(overlay::Gamepad)<<"\n";
FIELD(D,first_person);FIELD(D,first_person_fov);FIELD(D,first_person_arm);FIELD(D,camera_unavailable);FIELD(D,status);
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
