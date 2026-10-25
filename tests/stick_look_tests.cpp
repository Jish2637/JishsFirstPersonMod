#include "settings.h"
#include "stick_input_filter.h"
#include "camera_transition.h"
#include "camera_movement.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "look_smoothing.h"
#include "spring_policy.h"
#include <stdexcept>
#include <cstdlib>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory(){return std::filesystem::current_path()/"stick-test-data";}
std::filesystem::path data_directory(){return module_directory();}
}
void check(bool ok,const char* message) { if(!ok) { std::cerr<<message<<std::endl; std::exit(1); } }
int main() {
    using namespace jishsfirstpersonmod;using namespace dingosdk::first_person;
    constexpr std::uintptr_t game=0x140000000,parent=0x100000;
    for(const auto& child:player_state_contract::normal_foot_states) {
        check(camera_movement_mode(true,true,parent,parent+child.offset,game+child.vtable,game,504)==1,"normal movement lost custom camera");
        check(camera_movement_mode(true,true,parent,parent+child.offset,game+child.vtable+8,game,504)==0,"changed child vtable allowed custom camera");
    }
    check(camera_movement_mode(true,true,parent,parent+0x900,game+0x1234,game,100)==0,"special offboard animation mistaken for board camera");
    check(camera_movement_mode(false,false,0,0,0,game,100)==0,"invalid state allowed custom camera");
    check(camera_movement_mode(true,false,0,0,0,game,504)==0,"offboard physics alone proved normal walking");
    check(camera_movement_mode(true,false,0,0,0,game,100)==2,"board camera denied");
    check(camera_movement_mode(true,false,0,0,0,game,999)==0,"unknown state allowed custom camera");
    check(normalize_axis(-32768)==-1 && normalize_axis(32767)==1,"axis extremes incorrect");
    auto dead=radial_stick({.1f,.1f},.15f);check(dead.x==0 && dead.y==0,"radial deadzone drift");
    auto diagonal=radial_stick({1,1},.15f);check(std::abs(std::hypot(diagonal.x,diagonal.y)-1)<.00001f,"diagonal faster than straight");
    StickSettings settings;settings.enabled=true;Settings spring;
    {
        StickSettings bounded=settings;bounded.limit_angles=true;
        bounded.limit_left=30;bounded.limit_right=50;bounded.limit_up=20;bounded.limit_down=35;
        StickLookState limited;limited.seed(bounded,1,0,0,0,180);
        StickAim result;
        for(int i=1;i<=30;++i) result=limited.update(bounded,{-1,1},true,1,i*.1,0,180,spring);
        check(std::abs(result.heading-30)<.001 && std::abs(result.pitch-20)<.001,"left/up limits overrun");
        for(int i=31;i<=60;++i) result=limited.update(bounded,{1,-1},true,1,i*.1,0,180,spring);
        check(std::abs(result.heading+50)<.001 && std::abs(result.pitch+35)<.001,"right/down limits overrun");
        result=limited.update(bounded,{-1,1},true,1,6.1,0,180,spring);
        check(result.heading>-50 && result.pitch>-35,"limit accumulated hidden input at the stop");
        // Turning the skater through the +/-180 seam moves the permitted view.
        limited.seed(bounded,1,7,170,0,-10);
        result=limited.update(bounded,{},true,1,7.1,170,20,spring);
        check(std::abs(wrap_heading(result.heading-(-160)))<=50,"moving body seam broke bounds");
        bounded.limit_left=bounded.limit_right=bounded.limit_up=bounded.limit_down=0;
        auto damped=spring;damped.enabled=true;
        result=limited.update(bounded,{1,-1},true,1,7.2,170,20,damped);
        check(std::abs(wrap_heading(result.heading+160))<.001 && result.pitch==0,"smoothing escaped zero limits");
        bounded.limit_angles=false;
        result=limited.update(bounded,{-1,1},true,1,7.3,170,20,spring);
        check(result.pitch>0,"disabled limits remained active");
    }
    StickLookState state;
    auto coupled=settings;coupled.turn_character=true;
    StickLookState recentered;recentered.recenter_facing();
    check(!recentered.update(coupled,{1,0},true,1,0,45,90,spring,20).active,"dismount recenter skipped neutral stick");
    const auto centered=recentered.update(coupled,{},true,1,.016,45,90,spring,20);
    check(centered.active && centered.heading==-90 && centered.body_heading==90 && centered.pitch==0,
        "dismount/recovery recenter faces into the body");
    const auto facing=recentered.update(coupled,{1,0},true,1,.116,45,90,spring);
    check(std::abs(std::abs(wrap_heading(facing.body_heading-facing.heading))-180.f)<.001f,"recentered camera and character diverged");
    recentered.recenter_facing();
    const auto camera_only=recentered.update(settings,{},true,1,.132,45,90,spring,20);
    check(camera_only.heading==45 && camera_only.pitch==20,"camera-only mode was recentered");
    for(const float heading:{-179.f,-90.f,0.f,90.f,179.f}) {
        StickLookState dismounted;dismounted.recenter_facing();
        const auto outward=dismounted.update(coupled,{},true,1,0,heading,heading,spring);
        const auto camera=aim_camera(Matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},outward);
        const auto body=face_heading(Matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},heading);
        // Viewing direction is camera -Z; character facing is body +Z.
        const float dot=-(camera[8]*body[8]+camera[9]*body[9]+camera[10]*body[10]);
        check(dot>.999f && std::abs(wrap_heading(outward.body_heading-heading))<.001f,
            "dismount view did not face outward without turning the character");
    }
    StickLookState bail_center;bail_center.recenter_facing(180.f);
    check(!bail_center.update(coupled,{1,0},true,1,0,45,90,spring,20).active,"bail recenter skipped neutral stick");
    const auto bail_centered=bail_center.update(coupled,{},true,1,.016,45,90,spring,20);
    check(bail_centered.active && bail_centered.heading==-90 && bail_centered.body_heading==90 && bail_centered.pitch==0,
        "bail recenter did not reverse camera while preserving character facing");
    const auto bail_turned=bail_center.update(coupled,{1,0},true,1,.116,45,90,spring);
    check(std::abs(std::abs(wrap_heading(bail_turned.body_heading-bail_turned.heading))-180.f)<.001f,
        "character lock lost outward camera offset after bail");
    check(!state.update(settings,{1,0},true,1,0,0,0,spring).active,"held stick armed without neutral");
    check(state.update(settings,{},true,1,.016,0,0,spring).active,"neutral did not arm");
    auto aim=state.update(settings,{1,0},true,1,.116,0,0,spring);
    check(std::abs(aim.heading+12)<.0001f,"yaw rate incorrect");
    check(!state.update(settings,{1,0},false,1,.132,0,0,spring).capture,"mounting retained stick ownership");
    check(!state.update(settings,{1,0},true,1,.148,0,0,spring).active,"return from board skipped neutral");
    for(int fps:{30,60,120}) {
        StickLookState run;run.update(settings,{},true,1,0,0,0,spring);
        for(int i=1;i<=fps;++i) aim=run.update(settings,{1,0},true,1,double(i)/fps,0,0,spring);
        check(std::abs(aim.heading+120)<.001f,"frame-rate dependent sensitivity");
    }
    state.reset();state.update(settings,{},true,1,0,0,0,spring);
    for(int i=1;i<=120;++i) aim=state.update(settings,{0,1},true,1,i/60.,0,0,spring);
    check(aim.pitch==85,"pitch not clamped");
    auto held=state.update(settings,{},true,1,2.016,0,0,spring);check(held.pitch==85,"release recentered analog view");
    settings.turn_character=true;
    check(!state.update(settings,{1,0},true,1,2.032,0,0,spring).active,"mode switch skipped neutral");
    state.update(settings,{},true,1,2.048,30,10,spring);
    aim=state.update(settings,{1,0},true,1,2.148,30,10,spring);
    check(std::abs(aim.body_heading+2)<.001f && std::abs(aim.heading-18)<.001f,"camera/body heading bias doubled rotation");
    check(!state.update(settings,{},true,2,2.164,0,0,spring).capture ||
        state.update(settings,{},true,2,2.164,0,0,spring).heading==0,"device change retained old view");
    settings.invert_y=true;state.reset();state.update(settings,{},true,1,0,0,0,spring);
    aim=state.update(settings,{0,1},true,1,.1,0,0,spring);check(aim.pitch<0,"invert Y ignored");
    settings.invert_y=false;spring.enabled=true;
    state.reset();state.update(settings,{},true,1,0,0,0,spring);
    aim=state.update(settings,{1,0},true,1,.1,0,0,spring);check(aim.heading<0 && aim.heading>-12,"spring did not smooth analog turn");
    Matrix pose{1,0,0,7,0,1,0,8,0,0,1,9,3,4,5,1};
    StickAim view{true,true,70,15,70};auto camera=aim_camera(pose,view);
    check(std::abs(matrix_heading(camera)-70)<.001f,"absolute camera yaw incorrect");
    auto turned=target(pose,{}, {0,-40});auto same=aim_camera(turned,view);
    check(std::abs(matrix_heading(same)-70)<.001f,"native facing applied twice to view");
    Matrix tilted=pose;
    write(tilted,multiply(orientation(turned),{0,0,std::sin(.4f),std::cos(.4f)}),{3,4,5});
    tilted=target(tilted,{}, {35,0});
    check(aim_camera(tilted,{})==tilted,"inactive stick changed native camera");
    for(float pitch:{-85.f,0.f,35.f,85.f}) for(float yaw:{-170.f,-90.f,0.f,70.f,170.f}) {
        const auto level=aim_camera(tilted,{true,true,yaw,pitch,20});
        check(std::abs(wrap_heading(matrix_heading(level)-yaw))<.001f,"tilted head changed world yaw");
        check(std::abs(level[1])<.00001f,"horizontal look introduced roll");
        check(std::abs(level[9]+std::sin(pitch*std::numbers::pi_v<float>/180.f))<.00001f,"yaw changed viewing pitch");
        check(level[12]==3 && level[13]==4 && level[14]==5 && level[3]==7 && level[7]==8 && level[11]==9,"stick look changed native position/metadata");
        for(unsigned axis:{0u,4u,8u}) check(std::abs(length({level[axis],level[axis+1],level[axis+2]})-1)<.00001f,"camera basis lost normalization");
    }
    auto body=face_heading(pose,90);check(body[12]==3 && body[13]==4 && body[14]==5 && body[3]==7 && body[7]==8 && body[11]==9,"native motion position/metadata changed");
    CameraTransition handoff;
    Settings transition_settings;transition_settings.enabled=true;
    transition_settings.up=transition_settings.down=transition_settings.left=transition_settings.right=30;
    const auto foot=aim_camera(pose,{true,true,90,25,0});
    handoff.apply(foot,transition_settings,1,1,true,0);
    auto board=pose;board[12]+=1;
    auto start=handoff.apply(board,transition_settings,1,2,false,.016);
    check(std::abs(matrix_heading(start)-90)<.001f && std::abs(start[12]-foot[12])<.00001f,"mount handoff snapped");
    auto repeated=handoff.apply(board,transition_settings,1,2,false,.016);
    check(repeated==start,"same-frame callback integrated twice");
    auto progress=handoff.apply(board,transition_settings,1,2,false,.032);
    check(matrix_heading(progress)<90 && matrix_heading(progress)>0,"handoff failed to ease");
    auto reversed=handoff.apply(foot,transition_settings,1,1,true,.048);
    check(std::abs(matrix_heading(reversed)-matrix_heading(progress))<.001f,"reversed transition snapped");
    for(int i=4;i<240;++i) reversed=handoff.apply(foot,transition_settings,1,1,true,i*.016);
    check(reversed==foot,"handoff failed to finish exactly");
    check(handoff.apply(board,transition_settings,1,0,false,4)==board,"bail retained blending");
    auto native_animation=target(pose,{}, {40,120});
    check(handoff.apply(native_animation,transition_settings,1,0,false,4.001)==native_animation,"special animation retained custom transition");
    state.reset();state.update(settings,{},true,1,0,20,20,spring);
    state.update(settings,{1,0},true,1,.1,20,20,spring);
    check(!state.update(settings,{1,0},false,1,.116,20,20,spring).capture,"animation retained stick ownership");
    check(!state.update(settings,{1,0},true,1,.132,80,80,spring).active,"animation exit skipped neutral");
    const auto resumed=state.update(settings,{},true,1,.148,80,80,spring);
    check(resumed.active && resumed.heading==80,"animation exit retained old heading");
    handoff.apply(foot,transition_settings,1,1,true,4.016);
    check(handoff.apply(board,transition_settings,2,2,false,4.032)==board,"new player retained handoff");
    transition_settings.enabled=false;
    check(handoff.apply(foot,transition_settings,2,1,true,4.048)==foot,"disabled spring blended");
    transition_settings.enabled=true;
    handoff.reset();handoff.apply(foot,transition_settings,1,1,true,0);
    handoff.apply(board,transition_settings,1,2,false,.016);
    auto moving=board;moving[12]+=.2f;
    auto followed=handoff.apply(moving,transition_settings,1,2,false,.016);
    check(std::abs(followed[12]-start[12]-.2f)<.00001f,"handoff added lag to native motion");
    const auto transition_at=[&](float strength) {
        CameraTransition blend;auto cfg=transition_settings;cfg.up=cfg.down=cfg.left=cfg.right=strength;
        blend.apply(foot,cfg,1,1,true,0);blend.apply(board,cfg,1,2,false,.016);
        return blend.apply(board,cfg,1,2,false,.116);
    };
    check(matrix_heading(transition_at(70))>matrix_heading(transition_at(35)),"higher handoff strength did not soften mount");
    check(transition_at(0)==board,"zero handoff strength did not switch immediately");
    check(std::abs(matrix_heading(body)-90)<.001f,"native facing target incorrect");
    for(auto kind:{dingosdk::PlayStationPad::dualshock4,dingosdk::PlayStationPad::dualsense}) {
        std::array<std::uint8_t,64> report{};report[0]=1;report[1]=170;report[2]=180;report[3]=220;report[4]=30;
        report[5]=0x20;report[8]=40;report[9]=60;
        const auto before=dingosdk::parse_playstation_report(kind,false,report.data(),report.size());
        check(neutral_sony_stick(kind,report),"Sony report rejected");
        const auto after=dingosdk::parse_playstation_report(kind,false,report.data(),report.size());
        check(before && after && after->right_x==0 && std::abs(after->right_y)<=258 && before->buttons==after->buttons &&
            before->left_x==after->left_x && before->left_y==after->left_y && before->left_trigger==after->left_trigger,
            "Sony stick capture altered unrelated input");
    }
    Config config;config.stick=settings;config.stick.deadzone=.22f;
    config.stick.mount_smoothing=82;config.stick.dismount_smoothing=35;config.stick.smooth_transitions=false;
    auto path=data_directory()/"settings.ini";write_config(path,config);const auto reloaded=read_config(path);
    check(reloaded.stick.enabled && reloaded.stick.turn_character && reloaded.stick.deadzone==.22f,"stick preferences did not persist");
    check(!reloaded.stick.smooth_transitions && reloaded.stick.mount_smoothing==82 && reloaded.stick.dismount_smoothing==35,"independent handoff controls did not persist");
    config.stick.mount_smoothing=101;bool rejected=false;try {validate_config(config);} catch(...) {rejected=true;}
    check(rejected,"invalid handoff strength accepted");

    // Legacy migration preserves the effective on-foot behavior, then becomes independent.
    const auto migration_path=module_directory()/"migration.ini";
    for(bool linked:{false,true}) for(bool foot_off:{false,true}) {
        { std::ofstream old(migration_path); old<<"[JishsFirstPersonMod]\nspring=1\nstrength_up=12\nstrength_down=23\nstrength_left=34\nstrength_right=45\nlink_smoothing="<<linked
            <<"\nlook_smoothing_initialized=1\nlook_spring=1\nlook_strength_up=21\nlook_strength_down=32\nlook_strength_left=43\nlook_strength_right=54\ndisable_spring_on_foot="<<foot_off<<"\nbald_first_person=obsolete\n"; }
        auto migrated=read_config(migration_path);
        check(migrated.settings_migrated && migrated.stick_smoothing_initialized,"legacy migration not detected");
        check(migrated.stick_spring.enabled==!(linked && foot_off),"legacy on-foot switch changed");
        check(migrated.stick_spring.up==(linked ? 12:21) && migrated.stick_spring.right==(linked ? 45:54),"legacy strengths changed");
        write_config(migration_path,migrated);
        auto roundtrip=read_config(migration_path);
        check(!roundtrip.settings_migrated && roundtrip.stick_spring.up==migrated.stick_spring.up,"migration repeated or lost");
        std::ifstream saved(migration_path);std::stringstream text;text<<saved.rdbuf();
        check(text.str().find("bald_first_person")==std::string::npos,"retired option saved");
        roundtrip.look_spring.up=99;roundtrip.spring.up=98;
        check(roundtrip.stick_spring.up==migrated.stick_spring.up,"stick strengths linked after migration");
    }
    for(bool glance_enabled:{false,true}) for(bool stick_enabled:{false,true}) {
        Config independent;independent.link_smoothing=false;independent.look_smoothing_initialized=true;
        independent.look_spring.enabled=glance_enabled;independent.stick_spring.enabled=stick_enabled;independent.stick_smoothing_initialized=true;
        independent.look_spring.up=independent.look_spring.down=independent.look_spring.left=independent.look_spring.right=70;
        independent.stick_spring=independent.look_spring;independent.stick_spring.enabled=stick_enabled;
        LookSmoothing glance;
        glance.apply(pose,glance_settings(independent),{},0,false);
        const auto glancing=glance.apply(pose,glance_settings(independent),{30,0},.1,false);
        check((glancing==target(pose,{}, {30,0}))==!glance_enabled,"glance toggle affected incorrectly");
        for(StickAxes axis: {StickAxes{1,0},StickAxes{-1,0},StickAxes{0,1},StickAxes{0,-1}}) {
            StickLookState a,b;auto direct=independent.stick_spring;direct.enabled=false;
            a.update(settings,{},true,1,0,0,0,independent.stick_spring);b.update(settings,{},true,1,0,0,0,direct);
            auto smoothed=a.update(settings,axis,true,1,.1,0,0,independent.stick_spring);
            auto immediate=b.update(settings,axis,true,1,.1,0,0,direct);
            check((std::abs(smoothed.heading-immediate.heading)<.0001f && std::abs(smoothed.pitch-immediate.pitch)<.0001f)==!stick_enabled,"stick directional smoothing toggle failed");
        }
        SettingsStore independent_store(migration_path,independent);Command smoothing;smoothing.kind=CommandKind::stick_smoothing;
        smoothing.spring=independent.stick_spring;smoothing.spring.up=36;check(independent_store.submit(smoothing,0),"stick command rejected");
        check(independent_store.snapshot().config.look_spring.up==70,"stick command changed glance settings");
        independent_store.flush_due(1000);auto saved=read_config(migration_path);
        check(saved.stick_spring.up==36 && saved.look_spring.enabled==glance_enabled && saved.stick_spring.enabled==stick_enabled,"independent settings roundtrip failed");
        auto foot_config=camera_config_for_state(saved,true,1);
        check(foot_config.stick_spring.up==36,"on-foot spring policy changed stick smoothing");
        smoothing.kind=CommandKind::apply_preset;smoothing.preset=saved;
        check(independent_store.submit(smoothing,1000) && independent_store.snapshot().config.stick_spring.up==36,"preset lost independent smoothing");
    }
    {auto invalid=Config{};invalid.stick_spring.up=101;bool failed=false;try{validate_config(invalid);}catch(...){failed=true;}check(failed,"invalid stick smoothing accepted");}

    std::cout<<"Stick axes, timing, neutral handoff, smoothing, native facing and persistence passed\n";
}


