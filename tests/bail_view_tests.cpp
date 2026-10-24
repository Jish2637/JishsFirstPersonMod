#include "bail_view.h"
#include "camera_policy.h"
#include "bail_recovery_contract.h"
#include "getup_contract.h"
#include "camera_transition.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstring>
using namespace jishsfirstpersonmod;using namespace dingosdk::first_person;
void check(bool value,const char* message) { if(!value) { std::cerr<<message<<std::endl;std::exit(1); } }
float difference(const Matrix& a,const Matrix& b) { return length(rotation_vector(multiply(conjugate(orientation(a)),orientation(b)))); }
Matrix pose(float yaw=0,float x=0) { Matrix result{1,0,0,0,0,1,0,0,0,0,1,0,x,0,0,1};return target(result,{}, {0,yaw}); }
HeadPose head(float x=0) { HeadPose result;result.valid=true;result.head=pose(0,x);result.root=result.head;return result; }
std::vector<std::string> fields(const std::string& line) { std::vector<std::string> values;std::istringstream in(line);std::string word;while(std::getline(in,word,',')) values.push_back(word);return values; }
void replay(const char* path) {
    std::ifstream file(path);check(bool(file),"trace cannot be read");std::string line;std::getline(file,line);
    auto names=fields(line);std::map<std::string,unsigned> columns;for(unsigned i=0;i<names.size();++i) columns[names[i]]=i;
    BailView view;std::string event;unsigned rows=0,bail_rows=0,events=0,attached_rows=0;
    while(std::getline(file,line)) {
        auto values=fields(line);auto value=[&](const std::string& key) { return std::stod(values.at(columns.at(key))); };
        if(values[columns.at("event")]!=event) { event=values[columns.at("event")];view.reset();++events; }
        auto matrix=[&](const std::string& prefix) { auto result=pose();write(result,{static_cast<float>(value(prefix+"qx")),static_cast<float>(value(prefix+"qy")),static_cast<float>(value(prefix+"qz")),static_cast<float>(value(prefix+"qw"))}, {static_cast<float>(value(prefix+"x")),static_cast<float>(value(prefix+"y")),static_cast<float>(value(prefix+"z"))});return result; };
        const auto native=matrix("native_");HeadPose current;current.valid=value("head_valid")!=0;current.head=matrix("head_");current.root=matrix("root_");
        const bool bail=value("bail_latched") || value("wipeout_request") || value("animation_wipeout") || value("recovery");
        const bool normal=value("state_valid") && value("animation_valid") && value("custom_mode") && !bail;
        const auto output=view.apply(native,current,bail,normal,value("time_ms")/1000.);
        if(bail) {
            auto expected=native;
            if(current.valid)
                write(expected,head_camera_rotation(current),{native[12],native[13],native[14]});
            check(view.info().blocked && difference(output,expected)<.00002f,"trace bail orientation did not follow physical head axes");++bail_rows;
        }
        if(view.info().attached) { ++attached_rows;check(length(subtract({output[12],output[13],output[14]},{current.head[12],current.head[13],current.head[14]}))<=.40001f,"trace head attachment drifted"); }
        if(!view.info().blocked) {
            view.remember(value("phase")==0 ? matrix("final_"):output,native,current,normal);
        }
        ++rows;
    }
    check(events>=6 && bail_rows>250,"six-bail replay incomplete");
    std::cout<<"Replayed "<<events<<" bail events, "<<rows<<" samples, "<<bail_rows<<" head-facing bail samples, "<<attached_rows<<" head-attached samples\n";
}
int main(int argc,char** argv) {
    if(argc==2) { replay(argv[1]);return 0; }
    BailView view;auto base=pose(20,.1f);view.apply(base,head(),false,true,0);view.remember(base,base,head(),true);
    auto moving_head=head(2);const auto held=view.apply(pose(170,4),moving_head,true,false,.016);
    check(view.info().blocked && view.info().attached && std::abs(held[12]-2.1f)<.00001f,"head attachment did not move directly");
    auto head_facing=held;write(head_facing,head_camera_rotation(moving_head),{held[12],held[13],held[14]});
    check(difference(head_facing,held)<.00001f,"bail retained calibrated native look direction");
    auto revised=pose(-170,4);revised[3]=.123f;
    auto same=view.apply(revised,moving_head,true,false,.016);
    check(difference(held,same)<.00001f && same[3]==.123f,"same-frame hold or metadata failed");
    HeadPose missing;auto fallback=view.apply(pose(90,3),missing,true,false,.032);
    check(fallback[12]==3 && !view.info().attached,"missing head data did not use native position");
    auto restored=view.apply(pose(90,3),head(2),false,true,.048);
    check(!view.info().blocked && view.info().phase==0 && difference(restored,pose(90))<.00001f,
        "bail clear did not restore camera immediately");
    auto unstable=view.apply(pose(-90,3),head(2),false,false,.064);
    check(!view.info().blocked && difference(unstable,pose(-90))<.00001f,"angular instability delayed restored control");
    auto reheld=view.apply(pose(170,3),head(2),true,false,.080);
    check(view.info().blocked && view.info().phase==1,"renewed bail did not hold");
    view.reset();auto distant=pose(0,1);view.apply(distant,head(),false,true,0);view.remember(distant,distant,head(),true);
    auto rejected=view.apply(pose(90,2),head(1),true,false,.016);
    check(!view.info().calibrated && rejected[12]==2 && difference(rejected,head_facing)<.00002f,"invalid offset calibration was accepted");
    view.reset();view.apply(base,head(),false,true,0);view.remember(base,base,head(),true);
    auto attached=view.attach(pose(45,10),head(2));
    check(std::abs(attached[12]-2.1f)<.00001f && difference(attached,pose(45))<.00001f,"hippy attachment changed orientation or lagged position");
    view.apply(pose(179,2),head(2),true,false,.016);
    view.apply(pose(-179,2),head(2),false,true,.032);
    check(!view.info().blocked,"yaw wrap delayed immediate release");
    view.apply(pose(0,20),head(20),false,true,.048);
    check(!view.info().blocked,"teleport reused bail view");
    view.reset();view.apply(base,head(),false,true,0);view.remember(base,base,head(),true);
    view.apply(pose(60,2),head(2),true,false,.016);
    for(int i=2;i<=40;++i) view.apply(pose(60,2),head(2),false,true,i*.016);
    check(!view.info().blocked,"settled recovery never released camera");
    // All three head axes and the calibrated offset must move together.
    view.reset();auto calibrated_head=head();
    write(calibrated_head.head,from_rotation_vector({.2f,-.3f,.1f}),{0,0,0});
    auto native=calibrated_head.head;
    const Vec3 local_position{.1f,.02f,-.03f};const auto local_rotation=from_rotation_vector({-.1f,.05f,.02f});
    write(native,multiply(orientation(calibrated_head.head),local_rotation),rotate(orientation(calibrated_head.head),local_position));
    view.apply(native,calibrated_head,false,true,0);view.remember(native,native,calibrated_head,true);
    auto tumble=head(1);
    for(int i=1;i<=20;++i) {
        write(tumble.head,from_rotation_vector({i*.12f,i*-.09f,i*.16f}),{1,0,0});
        const auto output=view.apply(pose(80,1),tumble,true,false,i*.016);
        auto expected=tumble.head;write(expected,head_camera_rotation(tumble),
            add({1,0,0},rotate(orientation(tumble.head),local_position)));
        check(difference(output,expected)<.00002f && length(subtract({output[12],output[13],output[14]},
            {expected[12],expected[13],expected[14]}))<.00001f,"full head rotation/position target failed");
    }
    const auto recovery=view.apply(pose(80,1),tumble,true,false,.336,true);
    check(view.info().phase==2 && !view.info().blocked && view.info().attached,"controllable recovery did not release input while attached");
    StickAim relative{true,true,-25,12,0,true};
    const auto looked=aim_head_relative(recovery,relative);
    const auto expected_look=target(recovery,{}, {12,25});
    check(difference(looked,expected_look)<.00002f && looked[12]==recovery[12],"recovery look did not remain head-relative");
    auto moved=tumble;write(moved.head,from_rotation_vector({.7f,.8f,-.9f}),{1,0,0});
    const auto following=view.apply(pose(80,1),moved,true,false,.352,true);
    auto expected_follow=following;write(expected_follow,multiply(orientation(moved.head),local_rotation),{following[12],following[13],following[14]});
    check(difference(following,expected_follow)<.00002f,"recovery head stopped moving");
    view.apply(pose(80,1),moved,true,false,.368,false);
    check(view.info().phase==1 && view.info().blocked && view.info().recovery_yaw==0,"renewed tumble retained recovery look");
    const auto missing_rotation=view.apply(pose(90,1),{},true,false,.384);
    check(difference(missing_rotation,pose(90,1))<.00002f && !view.info().attached,"missing head reused a stale rotation");
    view.apply(pose(80,1),moved,true,false,1);
    check(!view.info().calibrated,"observation gap retained calibration");
    // Native control evidence, rather than the recovery boolean alone.
    Config config;config.third_person_bailing=false;config.stick.enabled=true;
    ComfortSnapshot state;state.state_valid=state.animation_valid=true;state.bailing=state.recovery=true;
    CameraMovementSample movement;movement.mode=1;movement.bail_recovery=true;
    check(!camera_look_allowed(config,state,movement),"recovery flag alone enabled look");
    movement.recovery_verified=movement.recovery_controllable=true;
    check(camera_look_allowed(config,state,movement) && !character_facing_allowed(state,movement),"recovery look turned character or stayed blocked");
    config.third_person_bailing=true;check(!camera_look_allowed(config,state,movement),"third person bail acquired recovery look");
    config.third_person_bailing=false;config.look_only=true;check(!camera_look_allowed(config,state,movement),"compatibility behavior changed");
    config.look_only=false;state.suppressed=true;check(!camera_look_allowed(config,state,movement),"suppressed camera acquired look");
    using namespace bail_recovery_contract;
    check(controllable(true,0x6000,false,false,false),"verified ground input rejected");
    check(!controllable(false,0x6000,false,false,false) && !controllable(true,0x2000,false,false,false) &&
        !controllable(true,0x4000,false,false,false) && !controllable(true,0x6001,false,false,false) &&
        !controllable(true,0x26000,false,false,false) &&
        !controllable(true,0x6000,true,false,false) && !controllable(true,0x6000,false,true,false) &&
        !controllable(true,0x6000,false,false,true),"animation/ragdoll/override/bail state acquired look");
    // A mismatched executable disables control recognition rather than guessing.
    bool corrupt=false;
    const auto read_contract=[&](std::uintptr_t address,void* output,std::size_t size) {
        const auto copy=[&](std::uintptr_t site,const auto& prefix) {
            if(address!=site || size!=prefix.size()) return false;
            std::memcpy(output,prefix.data(),size);if(corrupt) static_cast<unsigned char*>(output)[0]^=1;return true;
        };
        if(copy(selection_site,selection_prefix) || copy(ground_input_site,ground_input_prefix) ||
            copy(ground_source_site,ground_source_prefix)) return true;
        const auto ground=player_state_contract::normal_foot_states[0];
        if(address==ground.vtable+0x18 && size==sizeof(std::uintptr_t)) {
            std::memcpy(output,&ground.update,size);return true;
        }
        return false;
    };
    check(verified(read_contract,0),"reviewed recovery contract rejected");
    corrupt=true;check(!verified(read_contract,0),"mismatched recovery executable accepted");
    state.suppressed=false;
    for(bool foot_spring:{false,true}) {
        config.disable_spring_on_foot=foot_spring;
        check(camera_look_allowed(config,state,movement),"foot Spring suppression blocked verified recovery");
        config.stick.turn_character=true;
        check(!character_facing_allowed(state,movement),"recovery permitted character turning");
    }
    StickLookState stick;StickSettings options;options.enabled=true;options.turn_character=false;
    Settings no_spring;
    check(!stick.update(options,{1,0},true,1,0,0,0,no_spring).active,"held recovery input bypassed neutral safeguard");
    auto neutral=stick.update(options,{},true,1,.016,0,0,no_spring);
    check(neutral.active && neutral.heading==0 && neutral.pitch==0,"recovery did not enter at zero offset");
    const auto right=stick.update(options,{1,0},true,1,.032,0,0,no_spring);
    check(right.active && right.heading<0,"recovery right stick direction reversed");
    check(!stick.update(options,{},false,1,.048,0,0,no_spring).active,"lost control retained recovery look");
    check(!stick.update(options,{1,0},true,2,.064,0,0,no_spring).active,"controller change bypassed neutral safeguard");
    CameraTransition transition;Settings handoff;handoff.enabled=true;
    const auto bail_pose=aim_head_relative(recovery,relative);
    transition.apply(bail_pose,handoff,1,1,true,0,{},nullptr,nullptr,true,true,true);
    const auto released=transition.apply(pose(),handoff,1,1,false,.016,{},nullptr,nullptr,false,true,false);
    check(difference(released,bail_pose)<.00002f,"bail release did not seed existing transition");
    const auto settled=transition.apply(pose(),handoff,1,1,false,.032,{},nullptr,nullptr,false,true,false);
    check(difference(settled,pose())<difference(released,pose()),"bail release blend did not advance");
    // Smooth live head position independently of its spinning orientation.
    view.reset();auto upright=head();auto front=pose(35,.1f);Vec3 player{0,0,0};
    view.apply(front,upright,false,true,0);view.remember(front,front,upright,true);
    // A neutral camera needs the inverse joint-to-camera axis conversion;
    // an identity Head joint physically looks upward, not horizontally.
    const auto neutral_head=conjugate(head_camera_rotation(head()));
    auto before=head(1);write(before.head,multiply(from_rotation_vector({.25f,.1f,.3f}),neutral_head),{1,0,0});
    auto before_spin=view.apply(pose(0,1),before,true,false,.016);
    before_spin=view.finish(before_spin,before,&player,.016,false);
    auto spinning=head(1);write(spinning.head,from_rotation_vector({1.f,-2.f,2.f}),{1,0,0});
    auto locked=view.apply(pose(150,1),spinning,true,false,.032,false,true,&player);
    locked=view.finish(locked,spinning,&player,.032,false);
    check(view.info().phase==3 && view.info().blocked && view.info().getup_camera_phase==1 && !view.info().attached &&
        difference(locked,before_spin)<.00002f && std::abs(locked[12]-before_spin[12])<.00001f,
        "detached entry did not preserve displayed transform");
    const auto held_heading=matrix_heading(locked);
    for(int i=3;i<=30;++i) {
        player={i*.01f,0,0};
        write(spinning.head,from_rotation_vector({i*.3f,i*.8f,i*-.5f}),{1+i*.04f,1,0});
        auto output=view.apply(pose(-150,1),spinning,true,false,i*.016,false,false,&player);
        output=view.finish(output,spinning,&player,i*.016,false);
        check(view.info().getup_camera_phase==1 && !view.info().attached,"detached camera released early");
        const Vec3 previous{locked[12],locked[13],locked[14]},actual{output[12],output[13],output[14]};
        const auto wanted=view.info().head_target;
        const float fraction=1-std::exp(-.016f/.05f);auto expected_position=previous;
        for(unsigned axis=0;axis<3;++axis) expected_position[axis]+=(wanted[axis]-previous[axis])*fraction;
        if(length(subtract(actual,expected_position))>=.00002f)
            std::cerr<<"Get-up frame "<<i<<": actual "<<actual[0]<<","<<actual[1]<<","<<actual[2]
                <<" expected "<<expected_position[0]<<","<<expected_position[1]<<","<<expected_position[2]<<'\n';
        check(length(subtract(actual,expected_position))<.00002f,"get-up did not smoothly follow current head position");
        check(length(subtract(actual,wanted))<length(subtract(previous,wanted)),"head follower did not converge");
        if(i==30) check(std::abs(wrap_heading(matrix_heading(output)-held_heading))<.15f &&
            std::abs(output[9])<.01f && std::abs(output[4])<.01f,"detached view did not level");
        locked=output;
    }
    auto target_pose=pose(150,1);
    view.apply(target_pose,spinning,true,false,.496,true,false,&player);
    auto blend=view.finish(target_pose,spinning,&player,.496,true);
    check(difference(blend,locked)<.00002f && std::abs(blend[12]-locked[12])<.00001f &&
        view.info().getup_camera_phase==2 && !view.info().blocked,"control return snapped or stayed blocked");
    // Bail completion does not discard the in-flight positional return.
    write(spinning.head,orientation(pose(80)),{2,1,0});
    for(int i=1;i<=19;++i) {
        const double time=.496+i*.016;
        view.apply(target_pose,spinning,false,true,time,true,false,&player);
        blend=view.finish(target_pose,spinning,&player,time,true);
    }
    check(view.info().getup_camera_phase==3 && view.info().getup_blend==1 &&
        length(subtract({blend[12],blend[13],blend[14]},{target_pose[12],target_pose[13],target_pose[14]}))<.00001f &&
        difference(blend,target_pose)<.00002f,"300 ms return missed ordinary position/facing");
    const auto completed=blend;
    view.apply(target_pose,spinning,false,true,.816,true,false,&player);
    blend=view.finish(target_pose,spinning,&player,.816,true);
    check(length(subtract({blend[12],blend[13],blend[14]},{completed[12],completed[13],completed[14]}))<.00001f,
        "return completion snapped from calibrated head to ordinary camera position");
    // Gameplay regression: AnimatedRagdoll flags 0x10b0c004 -> 0x1180e004
    // clear the stand-up bits 16 ms before Ground gains control. The existing
    // wipeout request remains set throughout; this must not skip the return.
    view.reset();player={};view.apply(front,upright,false,true,0);view.remember(front,front,upright,true);
    check(!view.observe_wipeout_request(true,true),"initial bail counted as renewed get-up");
    auto output=view.apply(front,upright,true,false,.016,false,true,&player);
    output=view.finish(output,upright,&player,.016,false);
    for(int i=2;i<=30;++i) {
        output=view.apply(front,upright,true,false,i*.016,false,true,&player);
        output=view.finish(output,upright,&player,i*.016,false);
    }
    const auto stable=output;
    check(!getup_contract::requested(0x1000,0x18e0,0x65e8e60,0,0x1180e004),"captured handoff still requested stand-up");
    const bool renewal=view.observe_wipeout_request(true,true);
    auto rolled=head();write(rolled.head,from_rotation_vector({.7f,-1.5f,.9f}),{.2f,1,0});
    output=view.apply(pose(140),rolled,true,false,.496,false,false,&player,renewal);
    output=view.finish(output,rolled,&player,.496,false);
    check(!renewal && view.detached() && difference(output,stable)<.001f,
        "cleared stand-up flags snapped camera back to head rotation");
    const auto before_control=output;
    view.observe_wipeout_request(true,false);
    output=view.apply(pose(140),rolled,true,false,.512,true,false,&player);
    output=view.finish(output,rolled,&player,.512,true);
    check(view.info().getup_camera_phase==2 && difference(output,before_control)<.00002f,
        "verified Ground recovery did not seed the return from displayed rotation");
    auto prior=output;
    for(int i=1;i<=19;++i) {
        const double time=.512+i*.016;
        output=view.apply(pose(140),rolled,i<2,true,time,true,false,&player);
        output=view.finish(pose(140),rolled,&player,time,true);
        check(difference(output,prior)<.24f,"roll/yaw recovery returned in a single angular snap");prior=output;
    }
    check(difference(output,pose(140))<.00002f && !view.detached(),"handoff did not finish at ordinary orientation");
    // A new positive request interrupts a held episode; repeated requests,
    // unavailable evidence and flag-clearing transitions do not.
    view.reset();view.apply(front,upright,false,true,0);view.remember(front,front,upright,true);
    view.observe_wipeout_request(true,true);
    output=view.apply(front,upright,true,false,.016,false,true,&player);view.finish(output,upright,&player,.016,false);
    check(!view.observe_wipeout_request(true,true) && !view.observe_wipeout_request(false,false),
        "unchanged/unverified request interrupted stand-up");
    check(!view.observe_wipeout_request(true,false) && view.observe_wipeout_request(true,true),
        "new validated wipeout request was not detected");
    check(view.observe_wipeout_request(true,3),"new animation wipeout was masked by an existing request");
    // Renewed ragdoll, missing head, teleports and observation gaps release detached state.
    const auto start=[&]() {
        view.reset();player={};view.apply(front,upright,false,true,0);view.remember(front,front,upright,true);
        auto output=view.apply(front,upright,true,false,.016,false,true,&player);
        view.finish(output,upright,&player,.016,false);
    };
    start();auto resumed=view.apply(target_pose,spinning,true,false,.032,false,false,&player,true);
    resumed=view.finish(resumed,spinning,&player,.032,false);
    check(!view.detached() && view.info().attached && view.info().phase==1,"renewed ragdoll retained detached hold");
    start();view.apply(target_pose,{},true,false,.032,false,false,&player);
    check(!view.detached() && !view.info().attached,"invalid head reused detached transform");
    start();player={6,0,0};view.apply(target_pose,spinning,true,false,.032,false,false,&player);
    check(!view.detached() && !view.info().calibrated,"teleport retained detached state");
    start();view.apply(target_pose,spinning,true,false,.4,false,false,&player);
    check(!view.detached() && !view.info().calibrated,"observation gap retained detached state");
    start();view.reset();check(!view.detached(),"owner/menu/focus reset retained detached state");
    check(getup_contract::requested(0x1000,0x1720,0x65e8df8,0,0xb0c01c) &&
        !getup_contract::requested(0x1000,0x1720,0x65e8df8,0,0x88c000) &&
        !getup_contract::requested(0x1000,0x1060,0x65e8c20,0,0xb0c01c),"get-up request recognized unrelated movement");
    std::cout<<"Head-facing bails, smoothed get-up head-follow, seamless camera return and lifecycle tests passed\n";
}
