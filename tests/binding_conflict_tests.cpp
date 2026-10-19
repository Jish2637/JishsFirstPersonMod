#include "settings.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory(){return std::filesystem::current_path();}
std::filesystem::path data_directory(){return module_directory();}
}
int main() {
    using namespace jishsfirstpersonmod;
    const auto check=[](bool ok,const char* text) { if(!ok) throw std::runtime_error(text); };
    const auto root=std::filesystem::current_path()/"binding-conflict-test-data";
    std::filesystem::create_directories(root);
    // L1+D-pad down is distinct from native L1+R1+D-pad down.
    Config recommended;recommended.toggle=0x302;
    recommended.look[1].combo=0x102;
    recommended.native.active=true;recommended.native.bindings[0]=0x302;
    check(!native_binding_overlap(0x102,recommended.native),"subset chord incorrectly reserved");
    check(native_binding_overlap(0x302,recommended.native),"identical native chord not reserved");
    NativeControls shorter;shorter.active=true;shorter.bindings[0]=0x102;
    check(!native_binding_overlap(0x302,shorter),"superset chord incorrectly reserved");
    SettingsStore chords(root/"distinct-chords.ini",recommended);
    const auto chord_route=route_controller_input(chords.snapshot().config,false);
    check(chord_route.toggle_blocked && chord_route.glance[1].combo==0x102 && !chords.snapshot().conflicts.glance[1],
        "recommended glance down paused by native three-button toggle");
    check(!toggle_emote_bindings(recommended,false).enabled(),"unrelated stock action was treated as first-person toggle");
    recommended.native.bindings[10]=0x302;
    const auto stock_toggle=toggle_emote_bindings(recommended,false);
    check(stock_toggle.companion==0 && stock_toggle.native==0x302 && stock_toggle.enabled(),"stock-owned toggle lost emote protection while companion activation was paused");
    check(!toggle_emote_bindings(recommended,true).enabled(),"blocked/menu input kept emote protection active");
    recommended.toggle=0x304;
    check(toggle_emote_bindings(recommended,false).companion==0x304 && toggle_emote_bindings(recommended,false).native==0x302,"distinct companion and native toggles did not retain both protections");
    recommended.native.bindings[10]=0x80000070;
    check(toggle_emote_bindings(recommended,false).native==0,"keyboard stock binding was treated as a controller chord");
    dingosdk::ControllerLookState glance;
    glance.update(chord_route.glance,{true,0,1},false);
    check(glance.update(chord_route.glance,{true,0x102,1},false)[0]==-30,"distinct glance down did not fire");
    Command rebind;rebind.kind=CommandKind::binding;rebind.action=2;rebind.mask=0x102;
    check(chords.submit(rebind,0),"recording distinct chord was rejected");
    rebind.mask=0x302;
    check(!chords.submit(rebind,1),"recording identical native chord was accepted");
    Config config;config.toggle=16;config.stick.enabled=true;
    config.look={{{1,30},{2,30},{4,90},{8,90}}};
    NativeControls native;native.active=true;native.bindings[9]=1;config.native=native;
    SettingsStore store(root/"native.ini",config);
    auto snapshot=store.snapshot();
    check(snapshot.binding_conflict && snapshot.conflicts.glance[0] && !snapshot.conflicts.glance[1] && !snapshot.conflicts.toggle,"conflict reporting is not per action");
    auto route=route_controller_input(snapshot.config,false);
    check(!route.stick_blocked && !route.toggle_blocked && !route.glance[0].combo && route.glance[1].combo==2,"glance conflict became a blanket gate");
    check(snapshot.config.look[0].combo==1,"saved conflicting binding was cleared");

    StickLookState stick;dingosdk::first_person::Settings spring;
    stick.update(config.stick,{},!route.stick_blocked,1,1,0,0,spring);
    const auto aim=stick.update(config.stick,{1,0},!route.stick_blocked,1,1.05,0,0,spring);
    check(aim.active && aim.capture && std::abs(aim.heading)>1,"right-stick look stopped during a glance conflict");
    dingosdk::ControllerLookState look;
    dingosdk::ControllerInput input{true,0,1};look.update(route.glance,input,false);
    input.buttons=1;check(look.update(route.glance,input,false)[0]==0,"conflicting glance fired");
    input.buttons=2;check(look.update(route.glance,input,false)[0]==-30,"unaffected glance stopped");

    native.bindings[9]=16;store.refresh_native_controls(native);
    route=route_controller_input(store.snapshot().config,false);
    check(route.toggle_blocked && !route.stick_blocked,"native toggle overlap can double-toggle or block analog look");
    input.buttons=1;check(look.update(route.glance,input,false)[0]==0,"restored glance fired before neutral entry");
    input.buttons=0;look.update(route.glance,input,false);
    input.buttons=1;check(look.update(route.glance,input,false)[0]==30,"restored glance failed after neutral entry");
    native.bindings[9]=2;store.refresh_native_controls(native);
    route=route_controller_input(store.snapshot().config,false);
    check(!route.toggle_blocked && store.snapshot().config.toggle==16,"unrelated native hotkey removed the saved mod toggle");
    check(look.update(route.glance,input,false)[0]==30,"native edit reset unaffected held glance");
    native.bindings[9]=1;store.refresh_native_controls(native);

    Command enabled;enabled.kind=CommandKind::set_enabled;enabled.enabled=true;
    check(store.submit(enabled,0) && store.take_enabled()==true,"conflict blocked explicit activation");
    Command camera;camera.kind=CommandKind::camera_settings;camera.value=95;camera.spring=config.spring;
    check(store.submit(camera,10),"conflict blocked unrelated settings edit");store.flush_due(760);
    auto saved=read_config(root/"native.ini");
    check(!store.snapshot().unsaved && saved.fov==95 && saved.look[0].combo==1,"conflicted binding or unrelated setting failed to save");
    saved.native=native;
    SettingsStore reloaded(root/"reload.ini",saved);
    check(reloaded.snapshot().conflicts.glance[0],"saved conflict failed to reload");
    Command binding;binding.kind=CommandKind::binding;binding.action=3;binding.mask=1;
    check(!store.submit(binding,800),"new conflicting chord accepted");
    binding.action=1;binding.mask=0;
    check(store.submit(binding,801) && !store.snapshot().binding_conflict,"cannot clear only the conflicting glance");

    Config legacy=config;legacy.native={};legacy.reserved={1,0,0};
    SettingsStore old_store(root/"legacy.ini",legacy);
    route=route_controller_input(old_store.snapshot().config,false);
    check(!route.stick_blocked && !route.toggle_blocked && !route.glance[0].combo,"legacy glance conflict blocks unrelated input");
    old_store.refresh_reserved({16,0,0});
    route=route_controller_input(old_store.snapshot().config,false);
    check(route.toggle_blocked && !route.stick_blocked && route.glance[0].combo==1,"legacy toggle conflict blocks glance/stick");
    check(old_store.submit(camera,10),"legacy runtime conflict blocks settings");old_store.flush_due(760);
    check(read_config(root/"legacy.ini").toggle==16 && !old_store.snapshot().unsaved,"legacy runtime conflict fails persistence");
    Config same=legacy;same.reserved={};same.toggle=1;
    write_config(root/"shared.ini",same);
    const auto shared=read_config(root/"shared.ini");
    check(find_binding_conflicts(shared).toggle && find_binding_conflicts(shared).glance[0],"legacy toggle/glance overlap lost on reload");
    const auto blocked=route_controller_input(config,true);
    check(blocked.stick_blocked && blocked.toggle_blocked,"normal gameplay safety gate removed");
    input.buttons=2;check(look.update(blocked.glance,input,true)[0]==0,"menu/focus inhibition no longer blocks glances");
    const auto stopped=stick.update(config.stick,{1,0},!blocked.stick_blocked,1,1.1,0,0,spring);
    check(!stopped.active && !stopped.capture,"menu/focus inhibition no longer releases stick capture");
    std::cout<<"Per-action conflicts, analog/glance routing, neutral restoration, activation and persistence passed\n";
}
