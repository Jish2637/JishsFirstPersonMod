#include "stock_contract.h"
#include "settings.h"
#include <iostream>
#include <stdexcept>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory(){return {};}
std::filesystem::path data_directory(){return {};}
}
int main() {
    using namespace jishsfirstpersonmod;
    using namespace stock_contract;
    auto require=[](bool value,const char* text) { if(!value) throw std::runtime_error(text); };
    for(const auto& profile:contracts) {
        require(std::string(profile.version).size()>0 && std::string(profile.sha256).size()==64,
            "invalid profile identity" );
        const std::uintptr_t addresses[]{profile.status_rva,profile.state_pointer_rva,profile.mutex_lock_rva,
            profile.mutex_unlock_rva,profile.write_fov_rva,profile.source_initialized_rva,
            profile.source_base_rva,profile.source_transform_rva};
        for(auto address:addresses) require(address>0 && address<0x10000000,"invalid bridge RVA" );
        require(profile.source_base_rva%8==0 && profile.source_transform_rva%8==0 &&
            profile.state_pointer_rva%8==0,"unaligned bridge pointer slot" );
        for(const auto& other:contracts) if(&other!=&profile)
            require(std::string(profile.sha256)!=other.sha256,"duplicate profile hash" );
        std::array<unsigned char,32> digest{};
        for(unsigned i=0;i<32;++i) digest[i]=static_cast<unsigned char>(std::stoul(std::string(profile.sha256+i*2,2),nullptr,16));
        require(find_contract(digest,profile.status_rva)==&profile,"supported stock profile rejected");
        require(!find_contract(digest,profile.status_rva+1),"wrong status export accepted");
        digest[0]^=1;require(!find_contract(digest,profile.status_rva),"unverified stock DLL hash accepted");
    }
    for(const auto& profile:contracts) {
        selected=&profile;
        if(uses_v213_layout(&profile)) {
            require(native_sampler_rva()==0xda75e0 && console_queue_rva()==0x981730,"2.1.3 function mapping");
            require(callbacks_offset()==0xb68 && model_offset()==0xb98 && bindings_model_offset()==0x5fc8,"2.1.3 model mapping");
            require(debug_status_offset()==496 && camera_issue_offset()==160,"2.1.3 debug strings mapping");
            require(to_native({first_person,true,0}).action==13 && to_native({reset,false,0}).action==34,"2.1.3 action mapping");
            selected=nullptr;continue;
        }
        if(uses_v211_layout(&profile)) {
            require(native_sampler_rva()==0xbe3090 && console_queue_rva()==0x979980,"2.1.1 function mapping");
            require(callbacks_offset()==0x5b0 && model_offset()==0x5e0 && bindings_model_offset()==0x5fe0,"2.1.1 model mapping");
            require(to_native({first_person,true,0}).action==13 && to_native({reset,false,0}).action==34,"2.1.1 action mapping");
            continue;
        }
        const bool v2=std::string_view(profile.version)=="2.0.0" || std::string_view(profile.version)=="2.0.1" || std::string_view(profile.version)=="2.0.2" || std::string_view(profile.version)=="2.0.3" || std::string_view(profile.version)=="2.0.4" || std::string_view(profile.version)=="2.0.5" || std::string_view(profile.version)=="2.0.6" || std::string_view(profile.version)=="2.0.7" || std::string_view(profile.version)=="2.0.8";
        if(v2) {
            const bool latest=std::string_view(profile.version)=="2.0.2" || std::string_view(profile.version)=="2.0.3" || std::string_view(profile.version)=="2.0.4" || std::string_view(profile.version)=="2.0.5" || std::string_view(profile.version)=="2.0.6" || std::string_view(profile.version)=="2.0.7" || std::string_view(profile.version)=="2.0.8";
            require(native_sampler_rva()==(std::string_view(profile.version)=="2.0.8" ? 0xa467a0:std::string_view(profile.version)=="2.0.7" ? 0x22bb20:uses_v206_layout(&profile) ? 0x21ec50:(std::string_view(profile.version)=="2.0.4" || std::string_view(profile.version)=="2.0.5") ? 0x2196c0:uses_v203_layout(&profile) ? 0x210080:latest ? 0x273020:0x26f810),"wrong native sampler profile");
            require(console_queue_rva()==(std::string_view(profile.version)=="2.0.8" ? 0x811900:std::string_view(profile.version)=="2.0.7" ? 0x42210:uses_v206_layout(&profile) ? 0x41d90:(std::string_view(profile.version)=="2.0.4" || std::string_view(profile.version)=="2.0.5") ? 0x411b0:uses_v203_layout(&profile) ? 0x40e30:latest ? 0x3ec00:0x3ea30),"wrong console callback profile");
        }
        require(is_v2()==v2,"reviewed version selected wrong bridge family");
        require(callbacks_offset()==(uses_v206_layout(&profile) ? 0x590:uses_v203_layout(&profile) ? 0x4d0:v2 ? 0x4b8:0x4b0) && model_offset()==(uses_v206_layout(&profile) ? 0x5c0:uses_v203_layout(&profile) ? 0x500:v2 ? 0x4e8:0x4e0),"overlay layout mismatch");
        require(bindings_model_offset()==(std::string_view(profile.version)=="2.0.8" ? 0x5f88:uses_v206_layout(&profile) ? 0x5f78:0x5f50),"bindings model layout mismatch");
        for(int action=first_person;action<=reset;++action) {
            const Request original{action,true,12.5f};
            const auto wire=to_native(original);Request decoded;
            require(wire.action==(v2 ? (action==reset ? (uses_v206_layout(&profile) ? 34:33):action+2):action),"incorrect native command");
            require(from_native(wire,decoded) && decoded.action==action && decoded.enabled && decoded.value==12.5f,"camera command roundtrip failed");
        }
        if(v2) for(int action: {11,12,26,27,28,29,30,31,32,uses_v206_layout(&profile) ? 33:34}) {
            Request decoded; require(!from_native({action,true,1},decoded),"new native setting captured as legacy action");
        }
    }
    selected=nullptr;
    Config config; config.toggle=0x100;config.look[0].combo=0x200;
    SettingsStore store("unused-settings-test.ini",config);
    const Request requests[]{{spring,true,0},{offset_x,false,.25f},{offset_y,false,.5f},{offset_z,false,.75f},
        {pitch,false,10},{yaw,false,20},{roll,false,30},{up,false,12},{down,false,23},{left,false,34},{right,false,45},{fov,false,100}};
    for(const auto& request:requests) {
        auto next=store.snapshot().config;
        require(apply(next,request),"stock menu camera action not mapped");
        Command command;command.kind=CommandKind::camera_settings;command.spring=next.spring;command.value=next.fov;
        require(store.submit(command,0),"stock camera action rejected by shared settings");
    }
    const auto shared=store.snapshot().config;
    require(shared.spring.enabled && shared.spring.offset==std::array<float,3>{.25f,.5f,.75f} &&
        shared.spring.rotation==std::array<float,3>{10,20,30} && shared.spring.up==12 && shared.spring.down==23 &&
        shared.spring.left==34 && shared.spring.right==45 && shared.fov==100,"stock camera values differ in companion");
    require(shared.toggle==config.toggle && shared.look[0].combo==config.look[0].combo,"camera sync changed controller bindings");
    config=shared;require(apply(config,{fov,false,0}) && config.fov==0,"game FOV request lost");
    require(apply(config,{reset,false,0}) && !config.spring.enabled && config.spring.up==50 && config.spring.offset==std::array<float,3>{},"stock arm reset mismatch");
    require(!apply(config,{first_person,true,0}) && !apply(config,{0,false,0}),"unrelated native request mapped to camera preferences");
    CameraMirror mirror;
    Config native,local;native.fov=local.fov=90;
    mirror.reconcile(native,local,0);
    native.spring.offset[0]=.4f;
    local=mirror.reconcile(native,local,100);
    require(local.spring.offset[0]==.4f,"stock-side camera edit did not reach companion");
    const auto before=local;local.spring.offset[1]=.6f;
    mirror.requested(before,local,200);
    local=mirror.reconcile(native,local,201);
    require(local.spring.offset[1]==.6f,"stale stock snapshot reverted companion edit");
    native.spring.offset[1]=.6f;
    local=mirror.reconcile(native,local,300);
    native.spring.rotation[0]=17;
    local=mirror.reconcile(native,local,400);
    require(local.spring.rotation[0]==17 && local.spring.offset[1]==.6f,"two-way edits did not coexist");
    const auto previous=local;local.fov=0;mirror.requested(previous,local,500);
    local=mirror.reconcile(native,local,3000);
    require(local.fov==0,"stock's unsupported zero-FOV value cancelled game FOV mode");
    std::cout<<"Stock camera request mapping, shared preferences, game FOV and reset passed\n";
}
