#include "settings.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
GameplayInput menu_controller(){return {};}
bool overlay_available(bool& v) noexcept {v=false;return false;}
std::filesystem::path module_directory(){return std::filesystem::current_path();}
std::filesystem::path data_directory(){return module_directory()/"native-head-settings-test";}
}
void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
int main(){try {
 using namespace jishsfirstpersonmod;
 const auto root=data_directory();std::filesystem::create_directories(root);
 const auto file=root/"settings.ini";
 Config old;check(!old.stick.limit_angles,"angle limits must default off");check(!old.stick.native_emote_aim,"emote aiming must default off");
 check(!old.stick.native_head_look,"new option must default off");
 {std::ofstream f(root/"old.ini");f<<"[JishsFirstPersonMod]\nstick_look=1\n";}
 check(!read_config(root/"old.ini").stick.limit_angles,"missing limit key changed camera");
 check(!read_config(root/"old.ini").stick.native_emote_aim,"missing emote key must default off");
 check(!read_config(root/"old.ini").stick.native_head_look,"old settings changed feel");
 SettingsStore store(file,old);Command cmd;cmd.kind=CommandKind::stick_settings;cmd.stick=old.stick;cmd.stick.limit_angles=true;cmd.stick.limit_up=23;cmd.stick.limit_down=34;cmd.stick.limit_left=45;cmd.stick.limit_right=67;cmd.stick.native_head_look=true;cmd.stick.native_emote_aim=true;
 check(store.submit(cmd,10),"menu settings command rejected");store.flush_due(10000);
 check(store.snapshot().config.stick.native_head_look && read_config(file).stick.native_head_look,"option did not persist");
 check(store.snapshot().config.stick.native_emote_aim && read_config(file).stick.native_emote_aim,"emote option command/disk round trip");
 auto preset=store.snapshot().config;write_config(root/"preset.ini",preset);
 check(read_config(root/"preset.ini").stick.limit_right==67 && read_config(root/"preset.ini").stick.limit_angles,"preset lost limits");
 check(read_config(root/"preset.ini").stick.native_head_look,"preset lost option");
 check(read_config(root/"preset.ini").stick.native_emote_aim,"emote option preset round trip");
 cmd.stick.native_emote_aim=false;cmd.stick.native_head_look=false;check(store.submit(cmd,20000),"disable rejected");store.flush_due(30000);
 check(!read_config(file).stick.native_head_look,"disable did not persist");
 check(!read_config(file).stick.native_emote_aim,"emote option disable round trip");
 for(float bad:{-1.f,181.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
     auto invalid=preset;invalid.stick.limit_right=bad;bool rejected=false;
     try {write_config(root/"bad.ini",invalid);}catch(...) {rejected=true;}
     check(rejected,"invalid angle limit accepted");
 }
 std::cout<<"Native head option default, legacy settings, command route, saved setting/preset and disable persistence PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
