#include "marker_reset.h"
#include "stick_look.h"
#include "board_look.h"
#include "marker_glance.h"
#include <iostream>
#include <stdexcept>
using namespace jishsfirstpersonmod;
void check(bool value,const char* why) {if(!value) throw std::runtime_error(why);}
int main() {try {
    const MarkerReset::Owner player{1,2,3,4},other{5,6,7,8};
    for(bool camera_first:{false,true}) for(bool turn:{false,true}) {
        MarkerReset reset;
        check(!reset.observe(player,false,true,{0,0,0}),"initial observation falsely reset");
        const auto old=reset.generation;
        check(reset.observe(player,true,true,{0,0,0}),"same-position teleport missed");
        check(!reset.permits(old),"old facing request survived teleport");
        const auto episode=reset.generation;
        check(!reset.observe(player,true,true,{0,0,0}) && reset.generation==episode,"second callback duplicated reset");
        check(!reset.accept_native(player,true),"native accepted during teleport");
        reset.observe(player,false,true,{20,0,0});
        check(reset.generation==episode,"teleport exit duplicated reset");
        if(camera_first) check(!reset.take_neutral(true,true),"neutral preceded native seed");
        check(!reset.accept_native(player,false),"invalid native accepted");
        check(reset.accept_native(player,true),"fresh native rejected");
        check(!reset.accept_native(player,true),"native accepted twice");
        check(!reset.take_neutral(true,false),"held stick resumed look");
        check(!reset.take_neutral(false,true),"inhibited controls resumed look");
        check(reset.take_neutral(true,true) && reset.permits(episode),"neutral did not resume fresh aim");
        StickSettings config;config.enabled=true;config.turn_character=turn;
        StickLookState look;dingosdk::first_person::Settings spring;
        for(bool smooth:{false,true}) {
            spring.enabled=smooth;spring.up=spring.down=spring.left=spring.right=80;
            look.seed(config,1,8,95,-30,10);
            look.update(config,{1,.5f},true,1,8.2,95,10,spring,-30);
            look.reset();
            look.seed(config,1,10,-120,7,60);
            auto aim=look.update(config,{},true,1,10,-120,60,spring,7);
            check(aim.active && std::abs(wrap_heading(aim.heading+120))<.001f,"new heading changed after native seed");
            check(std::abs(aim.pitch-7)<.001f && std::abs(wrap_heading(aim.body_heading-60))<.001f,"pitch/body heading corrupted");
        }
        check(reset.observe(player,true,true,{20,0,0}),"repeated teleport missed");
        reset.observe(other,false,true,{20,0,0});
        check(!reset.pending && !reset.neutral && !reset.accept_native(player,true),"reset crossed owner replacement");
    }
    MarkerReset fallback;fallback.observe(player,false,true,{0,0,0});
    check(!fallback.observe(player,false,true,{1,0,0}),"normal motion reset");
    check(fallback.observe(player,false,true,{7,0,0}),"position discontinuity missed");
    const auto token=fallback.generation;fallback.clear();check(!fallback.pending && !fallback.neutral && !fallback.permits(token),"world/camera reset retained request");
    MarkerReset replacement;replacement.observe(player,false,true,{0,0,0});
    replacement.observe(player,true,true,{0,0,0});const auto old_generation=replacement.generation;
    replacement.observe(other,false,true,{20,0,0},true);
    check(replacement.pending && replacement.generation!=old_generation,"marker owner replacement lost pending native reset");
    check(!replacement.accept_native(player,true) && replacement.accept_native(other,true),"native reset not rebound to verified marker owner");
    MarkerGlanceMemory memory;memory.observe(true,true,2,1,10,1);
    check(memory.take(true)==2,"optional glance memory lost");
    memory.observe(false,true,2,1,10,2);check(memory.take(true)==-1,"disabled glance memory restored");
    BoardLookSettings board;board.enabled=true;BoardLookState board_look;
    dingosdk::ControllerInput input;input.available=true;input.device=1;input.buttons=board.modifier;
    auto aim=board_look.update(board,{1,0},true,input,1);
    check(!aim.capture && std::abs(aim.heading)<.001f,"held board modifier resumed stale look");
    std::cout<<"Teleport generation/callback order, native seed, both modes/smoothing, neutral, owner replacement, positional fallback, board modifier and optional glance memory PASS\n";
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}
