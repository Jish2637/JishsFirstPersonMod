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
 Config old;check(!old.stick.native_head_look,"new option must default off");
 {std::ofstream f(root/"old.ini");f<<"[JishsFirstPersonMod]\nstick_look=1\n";}
 check(!read_config(root/"old.ini").stick.native_head_look,"old settings changed feel");
 SettingsStore store(file,old);Command cmd;cmd.kind=CommandKind::stick_settings;cmd.stick=old.stick;cmd.stick.native_head_look=true;
 check(store.submit(cmd,10),"menu settings command rejected");store.flush_due(10000);
 check(store.snapshot().config.stick.native_head_look && read_config(file).stick.native_head_look,"option did not persist");
 auto preset=store.snapshot().config;write_config(root/"preset.ini",preset);
 check(read_config(root/"preset.ini").stick.native_head_look,"preset lost option");
 cmd.stick.native_head_look=false;check(store.submit(cmd,20000),"disable rejected");store.flush_due(30000);
 check(!read_config(file).stick.native_head_look,"disable did not persist");
 std::cout<<"Native head option default, legacy settings, command route, saved setting/preset and disable persistence PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
