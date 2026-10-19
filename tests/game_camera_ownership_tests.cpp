#include "game_camera_ownership.h"
#include "comfort_policy.h"
#include <cstdlib>
#include <iostream>
using namespace jishsfirstpersonmod;
void check(bool value,const char* message) {
    if(!value) { std::cerr<<message<<'\n';std::exit(1); }
}
int main() {
    GameCameraOwnershipPolicy ownership;
    ComfortPolicy comfort;comfort.request(true);
    PlayerState player;player.valid=player.board_verified=player.bail_verified=true;
    player.riding=true;
    for(bool riding:{false,true}) for(bool bail:{false,true}) {
        player.riding=riding;player.bailing=bail;
        ownership.observe({10,true,false,false});
        check(!ownership.suspended(),"ordinary gameplay created ownership suspension");
        check(ownership.effective(comfort.desired(player,false,false))==true,"movement affected ownership");
        check(ownership.effective(comfort.desired(player,true,true))==(!bail && riding),"existing restrictions changed");
    }
    ownership.observe({10,false,true,true});
    check(!ownership.suspended(),"unverified takeover was accepted");
    ownership.observe({10,true,true,false});
    check(ownership.suspended() && ownership.managing,"spectator takeover absent");
    check(ownership.effective(true)==false && ownership.effective({})==false,"takeover failed to inhibit activation");
    ownership.observe({20,false,false,true});
    ownership.observe({20,true,false,false});
    check(ownership.suspended(),"unknown/context replacement restored camera");
    ownership.acknowledge(true,false);
    check(ownership.managing,"unacknowledged disable lost management");
    ownership.observe({20,true,false,true});
    check(!ownership.suspended() && ownership.managing,"verified return did not wait for activation acknowledgment");
    player.bailing=true;player.riding=false;
    check(ownership.effective(comfort.desired(player,true,false))==false,"return bypassed bail");
    check(ownership.effective(comfort.desired(player,false,true))==false,"return bypassed board-only");
    player.valid=false;
    check(!ownership.effective(comfort.desired(player,true,true)),"return bypassed validity");
    comfort.request(false);
    check(ownership.effective(comfort.desired(player,false,false))==false,"return bypassed manual disable");
    ownership.acknowledge(false,false);
    check(!ownership.managing,"manual disable did not finish restoration");
    ownership.observe({20,true,true,false});
    ownership.observe({20,true,false,true});
    ownership.acknowledge(true,true);
    check(!ownership.managing,"successful restoration not acknowledged");
    ownership.observe({20,true,false,false,true});
    check(ownership.suspended() && ownership.reason==GameCameraReason::other_camera,"freecam/drone/replay takeover ignored");
    ownership.observe({21,false,false,true,false});
    ownership.observe({21,true,false,false,false});
    check(ownership.suspended(),"uncertain evidence restored after a foreign camera");
    ownership.observe({21,true,false,true,true});
    check(ownership.suspended(),"foreign camera flag lost precedence over local snapshot");
    ownership.observe({21,true,false,true,false});
    check(!ownership.suspended() && ownership.managing,"verified foreign-camera return not managed");
    std::cout<<"Ownership and existing restriction checks passed.\n";
}
