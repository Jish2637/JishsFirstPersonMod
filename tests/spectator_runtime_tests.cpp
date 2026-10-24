#include "spectator_runtime.h"
#include "settings.h"
#include "spectator_pose.h"
#include "spectator_look.h"
#include "bridge_resolver.h"
#include "Engine/Core/Hooks/hooks.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Engine/Game/Build/addresses.h"
#include <vector>
#include <iostream>
using namespace jishsfirstpersonmod;
using Matrix=dingosdk::first_person::Matrix;
using Render=bool(*)(std::uintptr_t,std::uintptr_t);
namespace {
std::uintptr_t stock{},game=0x140000000ULL;
Render replacement{};
Config config;
bool compatible=true,image_verified=true;
bool render_chain_known=false;
void check(bool value,const char* reason);
Matrix rendered_camera{};
unsigned render_writes{};
bool accept_camera=true;
bool write_camera(std::uintptr_t client,std::uintptr_t camera,const Matrix& matrix,float) {
    check(client==0x1234 && camera==0x100000,"render camera identity changed");
    check(!spectator_input_policy().mask,"reentrant render input unexpectedly acquired spectator ownership");
    ++render_writes;rendered_camera=matrix;return accept_camera;
}
bool native_render(std::uintptr_t,std::uintptr_t) { return true; }
void check(bool value,const char* reason) { if(!value)throw std::runtime_error(reason); }
template<class T> void put(std::uintptr_t address,T value) { std::memcpy(reinterpret_cast<void*>(address),&value,sizeof(value)); }
void code(std::uintptr_t address,std::string_view hex) {
    auto nibble=[](char c){return c<='9'?c-'0':c-'a'+10;};
    for(std::size_t i=0;i<hex.size()/2;++i)put(address+i,static_cast<unsigned char>(16*nibble(hex[2*i])+nibble(hex[2*i+1])));
}
Matrix pose(float x,float y,float z) { Matrix result{};dingosdk::first_person::write(result,{0,0,0,1},{x,y,z});return result; }
}
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
MenuSnapshot menu_snapshot() { MenuSnapshot result;result.config=config;return result; }
std::uintptr_t resolve_stock_handler(std::uintptr_t address) { return render_chain_known && address==game+dingosdk::addr::engine::board_render_pose ? stock+spectator::render_hook:0; }
namespace compatibility { bool enabled() { return compatible; } }
}
namespace dingosdk {
namespace supported_build { bool running_image_matches(std::uintptr_t) noexcept { return image_verified; } }
HookStatus WINAPI hook_prepare(void* target,void* hook,void** original) {
    if(reinterpret_cast<std::uintptr_t>(target)!=stock+spectator::render_hook)return HookNotExecutable;
    replacement=reinterpret_cast<Render>(hook);*original=reinterpret_cast<void*>(&native_render);return HookOk;
}
HookStatus WINAPI hook_queue_enable(void*) { return HookOk; }
HookStatus WINAPI hook_apply_queued() { return HookOk; }
HookFailure hook_last_failure() noexcept { return {}; }
const char* WINAPI hook_status_string(HookStatus) { return "mock"; }
}
int main() { try {
    std::vector<unsigned char> image(spectator::party_pointer+0x1000),party(0xc000),published(8+250*152),remotes(249*888),
        entity(0x700),component(0x200),collection(0x100),holder(0x200),rig(0x100),definition(0x200),
        vtable(0x100),resource(0x100),header(0x100+395*0x30),table(0x100),render_data(0x100),world(0x100);
    auto address=[](auto& v){return reinterpret_cast<std::uintptr_t>(v.data());};
    stock=address(image);const auto context=address(world),actor=address(entity),animation=address(component),holder_address=address(holder),buffer=address(header)+0x100;
    const auto* profile=std::find_if(std::begin(stock_contract::contracts),std::end(stock_contract::contracts),
        [](const auto& entry) { return std::string_view(entry.version)=="2.0.7"; });
    check(profile!=std::end(stock_contract::contracts),"2.0.7 runtime fixture profile missing");
    stock_contract::selected=profile;
    code(stock+spectator::party_getter,spectator::party_signature);code(stock+spectator::remote_getter,spectator::remote_signature);
    code(stock+spectator::render_hook,spectator::render_signature);code(stock+spectator::party_transform_return-17,spectator::transform_signature);
    put(stock+spectator::party_getter,static_cast<unsigned char>(0));initialize_spectator(game,stock,write_camera);
    check(!spectator_available() && !replacement && spectator_status().find("signature differs")!=std::string::npos,"signature failure was not explained");
    code(stock+spectator::party_getter,spectator::party_signature);
    // Spectator-local contracts suffice even when broad image/profile checks differ.
    image_verified=false;stock_contract::selected=nullptr;
    Config retry;retry.spectator_first_person=true;update_spectator_session(false,retry,context,false,0x1234);check(spectator_available() && replacement,"verified runtime bridge did not initialize");image_verified=true;stock_contract::selected=profile;
    check(spectator_transform_call(stock+spectator::party_transform_return) && !spectator_transform_call(stock+spectator::party_transform_return+1),"unrelated camera accepted");
    check(spectator_fov_call(stock+spectator::party_fov_return) && !spectator_fov_call(stock+spectator::party_fov_return+1),"unrelated FOV caller accepted");
    put(stock+spectator::party_pointer,address(party));put(stock+spectator::remote_pointer,address(remotes));
    put(address(party)+spectator::published_offset,address(published));put(address(party)+spectator::selected_offset,std::uint64_t{42});put(address(party)+spectator::epoch_offset,std::uint64_t{7});
    const auto record=address(published)+8+152*3;
    put(record,std::uint64_t{42});put(record+8,std::uint64_t{7});put(record+50,static_cast<unsigned char>(1));put(record+56,std::size_t{2});put(record+88,std::uint64_t{88});
    const auto remote=address(remotes)+2*888,position=address(party)+192+2*56;
    put(position,static_cast<unsigned char>(1));put(remote,actor);put(remote+8,animation);put(remote+24,context);put(remote+256,buffer);put(remote+264,std::uint64_t{1});
    put(actor,game+dingosdk::addr::engine::skater_entity_vtable);put(actor+0x20,context);put(actor+0x70,address(collection));
    put(animation,game+dingosdk::addr::engine::skater_component_vtable);put(animation+0x18,address(collection));put(animation+0xa0,holder_address);put(address(collection),actor);
    put(holder_address+0x78,address(rig));put(address(rig)+0x18,address(definition));put(address(rig)+0x20,address(header));
    put(address(definition),address(vtable));put(address(vtable)+0x78,game+dingosdk::addr::engine::animation_count_getter);put(address(definition)+0x1a0,address(resource));put(address(resource)+0xc,std::uint32_t{395});
    put(address(header)+0x10,std::uint64_t(address(table)));put(address(header)+0x1c,std::uint16_t{0x100});put(address(table),std::int32_t{-1});
    put(address(render_data)+0x10,buffer);
    for(unsigned i=0;i<395;++i) {std::array<float,12> bone{1,1,1,0,0,0,0,1,0,0,0,0};if(!i){bone[8]=10;bone[9]=2;bone[10]=4;}if(i==103)bone[9]=1.6f;put(buffer+i*0x30ULL,bone);}
    put(buffer+103*0x30ULL+16,std::array<float,4>{-.5f,-.5f,.5f,.5f});
    put(buffer+107*0x30ULL+32,std::array<float,4>{.06f,.02f,0,0});
    put(buffer+108*0x30ULL+32,std::array<float,4>{0,0,-.032f,0});
    put(buffer+109*0x30ULL+32,std::array<float,4>{0,0,.032f,0});
    config.spectator_first_person=true;config.fov=90;config.spring.offset={.036f,.03f,.018f};
    auto read=[](std::uintptr_t p,void* out,std::size_t count){std::memcpy(out,reinterpret_cast<void*>(p),count);return true;};
    auto eye=spectator::eye_pose(read,buffer,pose(0,0,0));
    check(std::abs(eye.camera[13]-3.66f)<1e-4 && std::abs(eye.camera[14]-3.93f)<1e-4,"eye midpoint/face clearance mismatch");
    put(buffer+16,std::array<float,4>{0,.70710678f,0,.70710678f});
    const auto turned=spectator::eye_pose(read,buffer,pose(0,0,0));
    check(std::abs(turned.camera[12]-9.93f)<1e-4 && std::abs(turned.camera[13]-3.66f)<1e-4 && std::abs(turned.camera[14]-4)<1e-4 && std::abs(turned.camera[2]+1)<1e-4,"rotated skeleton camera basis/eye position mismatch");
    put(buffer+16,std::array<float,4>{0,0,0,1});put(buffer,std::array<float,3>{2,2,2});
    const auto scaled=spectator::eye_pose(read,buffer,pose(0,0,0));
    check(std::abs(scaled.camera[13]-5.32f)<1e-4 && std::abs(scaled.camera[14]-3.86f)<1e-4,"eye clearance did not follow actor scale");
    put(buffer,std::array<float,3>{1,1,1});
    put(buffer+109*0x30ULL+32,std::array<float,4>{0,0,-.032f,0});
    bool invalid_eye=false;try{spectator::eye_pose(read,buffer,pose(0,0,0));}catch(...){invalid_eye=true;}
    check(invalid_eye,"collapsed eye spacing accepted");
    put(buffer+109*0x30ULL+32,std::array<float,4>{0,0,.032f,0});
    Config native=config;native.native.active=true;native.native.stabilize=false;native.native.follow_flips=false;
    native.native.smoothing=31;native.native.head_pitch=42;native.native.head_roll=53;native.native.bob=64;
    const auto look=spectator::look_settings(native);
    check(!look.stabilize && !look.follow_flips && look.smoothing==31 && look.head_pitch==42 && look.head_roll==53 && look.bob==64 && look.offset==config.spring.offset,"native look settings not mirrored");
    reskate_spectator::FrameSpring fs;
    auto spring_settings=spectator::look_settings(config);spring_settings.enabled=true;
    fs.begin(1);fs.sample(eye.camera,spring_settings,eye.origin);
    auto fresh=eye.camera;fresh[12]+=.5f;fs.begin(1.016);
    const auto frame_start=fs.frame_start;fs.sample(eye.camera,spring_settings,eye.origin);
    auto expected_state=frame_start;
    const auto expected=reskate_spectator::update(expected_state,fresh,spring_settings,1.016,eye.origin);
    check(fs.sample(fresh,spring_settings,eye.origin)==expected && fs.sample(fresh,spring_settings,eye.origin)==expected,"render refinement integrated Spring twice");
    auto alternate_header=header;
    const auto alternate_buffer=address(alternate_header)+0x100;
    auto normal=pose(10,4,7);Matrix output{};constexpr std::uintptr_t camera=0x100000;
    auto frame=[&](bool watching=true,bool blocked=false) {
        put(position+8,std::uint64_t{GetTickCount64()});update_spectator_session(watching,config,context,blocked,0x1234);
        const bool custom=spectator_transform(camera,normal,output);replacement(holder_address+0xc0,address(render_data));return custom;
    };
    check(!frame(),"session did not start with native follow camera");check(toggle_spectator_view(),"hotkey did not select spectator first person");
    check(frame(),"valid rendered head was unavailable");
    for(unsigned i=0;i<4;++i){Sleep(70);check(frame(),"head tracking interrupted");}
    check(std::abs(output[12]-10.036f)<1e-4 && std::abs(output[13]-3.69f)<1e-4 && std::abs(output[14]-3.912f)<1e-4,"runtime did not follow ReSkate eye camera plus configured offsets");
    check(std::abs(output[0]-1)<1e-4 && std::abs(output[5]-1)<1e-4 && std::abs(output[10]-1)<1e-4,"runtime used raw joint axes instead of camera axes");
    check(render_writes>0 && rendered_camera==output,"render camera did not match eye camera");
    // Animation may publish a fresh buffer before the camera callback; its
    // corresponding render callback supplies the first evidence for that buffer.
    // Keep the selected view and handoff while staging the new render pose.
    for(unsigned i=0;i<8;++i) {
        Sleep(35);
        const auto next_header=i%2 ? address(header):address(alternate_header);
        const auto next_buffer=i%2 ? buffer:alternate_buffer;
        put(address(rig)+0x20,next_header);put(remote+256,next_buffer);
        put(address(render_data)+0x10,next_buffer);
        const auto previous_writes=render_writes;
        frame();
        if(render_writes==previous_writes)std::cerr<<spectator_status()<<'\n';
        check(render_writes>previous_writes,"New animation buffer stalled spectator camera until a later matching frame");
        check(spectator_status().find("first person")!=std::string::npos,"Buffer refresh discarded first-person preference or handoff");
        check(std::abs(rendered_camera[13]-3.69f)<1e-4,"Buffer refresh restarted completed spectator blend");
    }
    check(toggle_spectator_view(),"rotating-buffer follow switch rejected");
    for(unsigned i=0;i<10;++i) {
        Sleep(35);
        const auto next_header=i%2 ? address(header):address(alternate_header);
        const auto next_buffer=i%2 ? buffer:alternate_buffer;
        put(address(rig)+0x20,next_header);put(remote+256,next_buffer);put(address(render_data)+0x10,next_buffer);frame();
    }
    check(spectator_fov(camera,70)==70,"rotating-buffer exit did not restore follow FOV");
    check(toggle_spectator_view(),"rotating-buffer first-person switch rejected");
    for(unsigned i=0;i<10;++i) {
        Sleep(35);
        const auto next_header=i%2 ? address(header):address(alternate_header);
        const auto next_buffer=i%2 ? buffer:alternate_buffer;
        put(address(rig)+0x20,next_header);put(remote+256,next_buffer);put(address(render_data)+0x10,next_buffer);
        const auto previous_writes=render_writes;frame();
        check(render_writes>previous_writes,"rotating buffer prevented first-person entry");
    }
    check(std::abs(rendered_camera[13]-3.69f)<1e-4 && spectator_fov(camera,70)==90,"rotating-buffer entry never completed its handoff");
    config.native.active=true;config.native.stabilize=false;config.spring.enabled=false;
    put(position+8,std::uint64_t{GetTickCount64()});update_spectator_session(true,config,context,false,0x1234);
    check(spectator_transform(camera,normal,output),"render refinement lost camera");
    put(buffer+103*0x30ULL+32,std::array<float,4>{0,1.8f,0,0});
    replacement(holder_address+0xc0,address(render_data));
    check(std::abs(rendered_camera[13]-3.89f)<1e-4,"rendered head movement left a one-frame camera offset");
    put(buffer+103*0x30ULL+32,std::array<float,4>{0,1.6f,0,0});frame();
    // Local modifier-held board look also applies to both spectator camera
    // callbacks, without changing the watched player's animation pose.
    config.board.enabled=true;config.board.look_smoothing=0;config.board.center_smoothing=0;
    GameplayInput pad;pad.available=true;pad.device=1;
    update_spectator_input(config,pad,false); // arm after modifier release
    pad.buttons=config.board.modifier;update_spectator_input(config,pad,false);
    const auto capture=spectator_input_policy(0);
    check(capture.mask==config.board.modifier && capture.axes.x==0 && capture.axes.y==0 &&
        !spectator_input_policy(1).mask,"spectator board-look input ownership mismatch");
    frame();const auto straight=rendered_camera;
    for(unsigned i=0;i<4;++i) {
        Sleep(20);pad.right_x=.7f;update_spectator_input(config,pad,false);frame();
    }
    using namespace dingosdk::first_person;
    check(length(rotation_vector(multiply(conjugate(orientation(straight)),orientation(rendered_camera))))>.05f,
        "modifier-held spectator right stick did not rotate the rendered camera");
    const auto looked=rendered_camera;
    check(length(subtract({straight[12],straight[13],straight[14]},
        {looked[12],looked[13],looked[14]}))<.00001f,"spectator freelook moved off the watched head");
    pad.buttons=0;pad.right_x=0;
    for(unsigned i=0;i<3;++i) {Sleep(20);update_spectator_input(config,pad,false);frame();}
    check(!spectator_input_policy(0).mask &&
        length(rotation_vector(multiply(conjugate(orientation(straight)),orientation(rendered_camera))))<.00001f,
        "spectator modifier release did not recenter");
    pad.buttons=config.board.modifier;update_spectator_input(config,pad,false);
    update_spectator_input(config,pad,true);
    check(!spectator_input_policy().mask,"blocked spectator input retained capture");
    config.board.enabled=false;pad={};update_spectator_input(config,pad,false);frame();
    check(spectator_fov(camera,70)==90 && spectator_fov(camera+8,70)==70,"spectator FOV escaped owned camera");
    std::array<float,3> scale{};std::memcpy(&scale,reinterpret_cast<void*>(buffer+103*0x30ULL),sizeof(scale));check(scale==std::array<float,3>{.001f,.001f,.001f},"watched rendered head remained visible");
    accept_camera=false;replacement(holder_address+0xc0,address(render_data));accept_camera=true;
    std::memcpy(&scale,reinterpret_cast<void*>(buffer+103*0x30ULL),sizeof(scale));
    check(scale==std::array<float,3>{1,1,1} && spectator_status().find("ownership changed")!=std::string::npos,"render ownership loss retained head view");
    frame();for(unsigned i=0;i<4;++i){Sleep(70);frame();}
    check(toggle_spectator_view(),"follow switch rejected");frame();for(unsigned i=0;i<4;++i){Sleep(70);frame();}
    check(spectator_fov(camera,70)==70,"follow FOV not restored");std::memcpy(&scale,reinterpret_cast<void*>(buffer+103*0x30ULL),sizeof(scale));check(scale==std::array<float,3>{1,1,1},"follow head visibility not restored");
    check(toggle_spectator_view(),"second head-view switch rejected");frame();update_spectator_session(true,config,context,true);check(!toggle_spectator_view(),"menu/focus interruption consumed toggle");
    std::memcpy(&scale,reinterpret_cast<void*>(buffer+103*0x30ULL),sizeof(scale));check(scale==std::array<float,3>{1,1,1},"interruption left head hidden");
    update_spectator_session(false,config,context,false);check(!toggle_spectator_view(),"local turn consumed spectator toggle");
    check(!frame(),"new spectator session retained first person");toggle_spectator_view();frame();put(remote+264,std::uint64_t{2});frame();
    config.spectator_first_person=false;check(!frame() && !toggle_spectator_view(),"disabled option retained spectator camera");
    config.spectator_first_person=true;put(address(party)+spectator::selected_offset,std::uint64_t{0});
    update_spectator_session(true,config,context,false);check(!toggle_spectator_view(),"native non-party spectator consumed view toggle");
    check(spectator_status().find("verified party player")!=std::string::npos,"native spectator fallback reason missing");
    config.spectator_first_person=false;
    reset_spectator_view();
    check(config.fov==90 && config.spectator_first_person==false,"spectator switching changed preferences");
    std::cout<<"Mocked spectator runtime camera, render visibility, FOV, input and restoration passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;} }
