#include "spectator_runtime.h"
#include "spectator_head.h"
#include "spectator_pose.h"
#include "spectator_look.h"
#include "head_attachment.h"
#include "settings.h"
#include "stock_contract.h"
#include "stock_bridge.h"
#include "bridge_resolver.h"
#include "game_camera_ownership.h"
#include "owned_hooks.h"
#include "Extension/Multiplayer/Remote/native_pose_layout.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Engine/Game/Build/image_identity.h"
#include <chrono>
namespace jishsfirstpersonmod {
namespace {
std::mutex spectator_mutex;
std::atomic<bool> supported{};
std::atomic<bool> preference_seen{};
std::uintptr_t game_base{},stock_base{},context{},view_camera{};
std::uintptr_t camera_client{};
DWORD camera_thread{};
SpectatorRenderWriter render_writer{};
reskate_spectator::FrameSpring spring;
reskate_spectator::Settings spring_settings;
BoardLookState board_look;StickAim board_aim{};
unsigned look_device{};std::uint32_t look_modifier{};double input_time{};
spectator::Identity spring_owner{};
reskate_spectator::Vec3 previous_origin{};
double frame_time{};
spectator::Matrix follow_matrix{};
float follow_fov=90;
bool interrupted{};
bool rendered_view{};
double view_sampled{};
spectator::View view;
std::optional<spectator::Target> target;
struct RenderEvidence { spectator::Identity actor;std::uintptr_t buffer;double sampled; };
std::optional<RenderEvidence> render_evidence;
std::string feedback="First-person spectator view requires verified ReSkate 2.1.3.";
using Render=bool(*)(std::uintptr_t,std::uintptr_t);
Render original_render{};
spectator::HiddenHead hidden;
auto memory_read=[](std::uintptr_t address,void* out,std::size_t size) { return dingosdk::memory::read_bytes(address,out,size); };
double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
void report_failure(const std::string& reason) {
    static double last{};
    const auto time=now();
    if(!last || time-last>=1) { last=time;log("Spectator view reset: "+reason); }
}
bool write(std::uintptr_t address,const std::array<float,3>& scale) noexcept {
    __try { std::memcpy(reinterpret_cast<void*>(address),scale.data(),sizeof(scale));return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void restore_head() {
    hidden.restore(memory_read,write,[](std::uintptr_t holder) { return dingosdk::multiplayer::read_native_pose_layout(memory_read,game_base,holder,512); });
}
void reset_look() { spring.reset();spring_owner={};frame_time=0;board_look.reset();board_aim={};look_device=look_modifier=0;input_time=0; }
void discard() { restore_head();target.reset();render_evidence.reset();view_camera=0;rendered_view=false;view.discard();reset_look(); }
bool render(std::uintptr_t animation_interface,std::uintptr_t render_data) {
    const bool result=original_render(animation_interface,render_data);
    const auto outgoing_error=GetLastError();
    if(result && supported.load() && compatibility::enabled()) {
        std::lock_guard lock(spectator_mutex);
        try {
            if(target && view.session() && !interrupted && now()-view_sampled<=.250 && animation_interface==target->holder+0xc0 &&
                spectator::same_actor(memory_read,*target)) {
                spectator::Memory memory(memory_read);
                const auto party=memory.get<std::uintptr_t>(stock_base,spectator::party_pointer);
                if(memory.get<std::uint64_t>(party,spectator::selected_offset)==target->identity.id &&
                    memory.get<std::uint64_t>(party,spectator::epoch_offset)==target->identity.epoch) {
                    const auto pose=dingosdk::multiplayer::read_native_pose_layout(memory_read,game_base,target->holder,512);
                    if(pose.count==395 && pose.buffer && memory.get<std::uintptr_t>(render_data,0x10)==pose.buffer) {
                        render_evidence=RenderEvidence{target->identity,pose.buffer,now()};
                        restore_head();
                        if(view.owns_camera() && render_writer && GetCurrentThreadId()==camera_thread && frame_time && now()-frame_time<=.250) {
                            const auto eyes=spectator::eye_pose(memory_read,pose.buffer,follow_matrix);
                            const auto camera_pose=aim_head_relative(spring.sample(eyes.camera,spring_settings,eyes.origin),board_aim);
                            const auto refined=view.apply(follow_matrix,camera_pose,target->identity,frame_time);
                            if(!render_writer(camera_client,view_camera,refined,view.fov(follow_fov,menu_snapshot().config.fov,frame_time)))
                                throw std::runtime_error("Spectator camera ownership changed.");
                            rendered_view=true;
                            feedback=view.preferred() ? "Spectator view: first person.":"Spectator view: returning to follow camera.";
                        }
                        if(view.owns_camera() && !hidden.hide(memory_read,write,[](std::uintptr_t holder) { return dingosdk::multiplayer::read_native_pose_layout(memory_read,game_base,holder,512); },*target,pose.buffer))
                            throw std::runtime_error("Watched head visibility is unavailable.");
                    } else throw std::runtime_error("Watched render pose is unavailable.");
                }
            }
        } catch(const std::exception& e) {
            report_failure(e.what());
            if(render_writer && view_camera && GetCurrentThreadId()==camera_thread)render_writer(camera_client,view_camera,follow_matrix,follow_fov);
            restore_head();render_evidence.reset();view.discard();view_camera=0;rendered_view=false;reset_look();feedback=std::string("Spectator view: follow camera. ")+e.what();
        }
    }
    SetLastError(outgoing_error);return result;
}
}
bool spectator_available() noexcept { return supported.load(); }
bool spectator_first_person_enabled() {std::lock_guard lock(spectator_mutex);return view.session() && view.preferred();}
bool spectator_transform_call(std::uintptr_t caller) noexcept { return supported.load() && caller==stock_base+spectator::party_transform_return; }
bool spectator_fov_call(std::uintptr_t caller) noexcept { return supported.load() && caller==stock_base+spectator::party_fov_return; }
std::string spectator_status() { std::lock_guard lock(spectator_mutex);return feedback; }
void initialize_spectator(std::uintptr_t game,std::uintptr_t stock,SpectatorRenderWriter writer) {
    using namespace dingosdk;
    if(supported.load()) return;
    game_base=game;stock_base=stock;render_writer=writer;
    const auto unavailable=[&](const std::string& reason) {
        std::lock_guard lock(spectator_mutex);
        feedback="Spectator preference can be saved; runtime unavailable: "+reason;
        log(feedback);
    };
    for(const auto& check:std::array<std::pair<std::uintptr_t,std::string_view>,4>{{
        {spectator::party_getter,spectator::party_signature}, {spectator::remote_getter,spectator::remote_signature},
        {spectator::render_hook,spectator::render_signature}, {spectator::party_transform_return-17,spectator::transform_signature}}}) {
        if(!spectator::signature(memory_read,stock+check.first,check.second)) {
            unavailable("ReSkate spectator signature differs at RVA "+std::to_string(check.first)+".");return;
        }
    }
    const auto handler=resolve_stock_handler(game+addr::engine::board_render_pose);
    if(handler!=stock+spectator::render_hook) {
        log("Spectator native render chain not recognized; attaching directly to the verified ReSkate spectator render function. Resolved address="+std::to_string(handler));
    }
    void* forwarding{};
    auto* entry=reinterpret_cast<void*>(stock+spectator::render_hook);
    auto result=owned_hook_prepare(entry,reinterpret_cast<void*>(&render),&forwarding);
    if(result==HookOk) {
        original_render=reinterpret_cast<Render>(forwarding);result=hook_queue_enable(entry);
        if(result==HookOk)result=apply_owned_hooks();
    }
    if(result!=HookOk) { unavailable("render hook installation failed: "+hook_failure_description(result));return; }
    { std::lock_guard lock(spectator_mutex);feedback="Spectator view: follow camera. Use the First Person binding while watching.";supported=true; }
    log("Spectator head view: verified ReSkate 2.1.3 ID/slot/generation bridge and party camera/render callbacks.");
}
void update_spectator_session(bool watching,const Config& config,std::uintptr_t next_context,bool blocked,std::uintptr_t client) {
    const bool previously_enabled=preference_seen.exchange(config.spectator_first_person);
    if(config.spectator_first_person && !previously_enabled && !supported.load() && game_base && stock_base)
        initialize_spectator(game_base,stock_base,render_writer);
    std::lock_guard lock(spectator_mutex);
    // Restore before the next native animation update; shrinking only the
    // renderer's remote buffer never changes ReSkate's received pose vectors.
    restore_head();context=next_context;interrupted=blocked;camera_client=client;
    bool party_watching{};
    if(watching && config.spectator_first_person && supported.load()) {
        try {
            spectator::Memory memory(memory_read);
            party_watching=memory.get<std::uint64_t>(memory.get<std::uintptr_t>(stock_base,spectator::party_pointer),spectator::selected_offset)!=0;
        } catch(...) {}
    }
    view.observe(watching && party_watching,config.spectator_first_person && supported.load(),context,blocked);
    if(!view.session() || blocked) { target.reset();render_evidence.reset();view_camera=0;rendered_view=false;reset_look(); }
    if(supported.load() && !watching)feedback="Spectator view: follow camera. Use the First Person binding while watching.";
    else if(supported.load() && watching && config.spectator_first_person && !party_watching)
        feedback="Spectator view: follow camera. ReSkate is not following a verified party player.";
}
bool toggle_spectator_view() {
    std::lock_guard lock(spectator_mutex);
    if(interrupted) { feedback="Spectator view paused. Close the menu and focus the game to switch.";return false; }
    if(!view.toggle())return false;
    board_look.reset();board_aim={};input_time=0;
    feedback=view.preferred() ? "Spectator view: first person selected.":"Spectator view: follow camera selected.";
    log(feedback);return true;
}
void update_spectator_input(const Config& config,const GameplayInput& input,bool blocked) {
    std::lock_guard lock(spectator_mutex);
    const auto time=now();
    const bool eligible=!blocked && !interrupted && supported.load() && compatibility::enabled() &&
        view.session() && view.preferred() && view.owns_camera() && rendered_view && target && time-view_sampled<=.250;
    board_aim=board_look.update(config.board,{input.right_x,input.right_y},eligible,input,time);
    look_device=eligible ? input.device:0;look_modifier=config.board.modifier;input_time=eligible ? time:0;
}
StickInputPolicy spectator_input_policy(unsigned slot) {
    // Native camera writes may poll input while the render callback already
    // owns this mutex. Input adapters must never wait on their own renderer.
    std::unique_lock lock(spectator_mutex,std::try_to_lock);
    if(!lock.owns_lock()) return {};
    if(!input_time || now()-input_time>.1 || interrupted || !view.preferred() || !view.owns_camera() ||
        !board_aim.capture || !compatibility::enabled()) return {};
    if(slot!=~0u && (slot>=4 || look_device>15 || !(look_device&(1u<<slot)))) return {};
    // Spectator look must not steer the native party camera or the local skater.
    return {look_modifier,{}};
}
bool spectator_transform(std::uintptr_t camera,const spectator::Matrix& normal,spectator::Matrix& output) {
    std::lock_guard lock(spectator_mutex);
    const auto config=menu_snapshot().config;
    view.observe(true,config.spectator_first_person && supported.load(),context,interrupted);
    if(!view.session() || interrupted || !compatibility::enabled()) { discard();return false; }
    restore_head();std::string reason;
    const auto captured=spectator::read_target(memory_read,stock_base,context,GetTickCount64(),reason);
    if(!captured) { if(view.preferred())report_failure(reason);discard();feedback="Spectator view: follow camera. "+reason;return false; }
    try {
        spectator::Memory memory(memory_read);
        const auto& id=captured->identity;
        if(memory.get<std::uintptr_t>(id.entity)!=game_base+dingosdk::addr::engine::skater_entity_vtable ||
            memory.get<std::uintptr_t>(id.entity,0x20)!=context ||
            memory.get<std::uintptr_t>(id.component)!=game_base+dingosdk::addr::engine::skater_component_vtable ||
            memory.get<std::uintptr_t>(memory.get<std::uintptr_t>(id.component,0x18))!=id.entity ||
            memory.get<std::uintptr_t>(id.component,0x70))throw std::runtime_error("Watched actor ownership changed.");
        const auto pose=dingosdk::multiplayer::read_native_pose_layout(memory_read,game_base,captured->holder,512);
        if(pose.count!=395 || pose.buffer!=captured->buffer || !spectator::same_actor(memory_read,*captured))
            throw std::runtime_error("Watched head pose is unavailable.");
        const auto eyes=spectator::eye_pose(memory_read,pose.buffer,normal);
        const auto time=now();
        if(view_camera && view_camera!=camera) { view.discard();reset_look(); }
        if(spring_owner!=id || !frame_time || time<frame_time || time-frame_time>.250 ||
            reskate_spectator::length(reskate_spectator::subtract(eyes.origin,previous_origin))>5)reset_look();
        spring_owner=id;previous_origin=eyes.origin;frame_time=time;spring_settings=spectator::look_settings(config);spring.begin(time);
        follow_matrix=normal;camera_thread=GetCurrentThreadId();
        target=captured;view_camera=camera;view_sampled=time;
        // A fresh animation buffer is often first rendered AFTER this callback.
        // Stage the view/handoff now, but leave the native follow camera intact
        // until the render hook verifies and writes that buffer in this frame.
        // Resetting here perpetually restarts entry on rotating pose buffers.
        rendered_view=render_evidence && render_evidence->actor==id && render_evidence->buffer==pose.buffer && time-render_evidence->sampled<=.250;
        const auto previous_phase=view.phase();
        output=view.apply(normal,aim_head_relative(spring.sample(eyes.camera,spring_settings,eyes.origin),board_aim),id,time);
        if(previous_phase!=view.phase())log(std::string("Spectator camera: ")+body_flip::phase_name(view.phase())+
            " player="+std::to_string(id.id)+" slot="+std::to_string(id.slot)+" generation="+std::to_string(id.generation));
        feedback=view.owns_camera() ? (view.preferred() ? "Spectator view: first person.":"Spectator view: returning to follow camera."):
            "Spectator view: follow camera. Use the First Person binding to switch.";
        if(view.owns_camera() && !rendered_view)feedback="Spectator view: follow camera. Waiting for the watched player's render pose.";
        return view.owns_camera() && rendered_view;
    } catch(const std::exception& e) { report_failure(e.what());discard();feedback=std::string("Spectator view: follow camera. ")+e.what();return false; }
}
float spectator_fov(std::uintptr_t camera,float normal) {
    std::lock_guard lock(spectator_mutex);
    const auto config=menu_snapshot().config;
    if(std::isfinite(normal) && normal>1 && normal<175)follow_fov=normal;
    if(!compatibility::enabled() || !config.spectator_first_person || camera!=view_camera || interrupted || !rendered_view || now()-view_sampled>.250)return normal;
    return view.fov(normal,config.fov,now());
}
void reset_spectator_view() { std::lock_guard lock(spectator_mutex);discard();view.observe(false,false,0); }
}
