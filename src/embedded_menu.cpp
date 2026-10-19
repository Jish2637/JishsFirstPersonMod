#include "release_updates.h"
#include "embedded_menu.h"
#include <imgui_internal.h>
#include "bail_collision.h"
#include "comfort_policy.h"
#include "player_state_contract.h"
#include "stick_runtime.h"
#include "stick_input_filter.h"
#include "game_camera_ownership.h"
#include "spectator_runtime.h"
#include "Extension/UI/skate_theme.h"
#include "Extension/UI/Overlay/skate_style.h"
#include <windowsx.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

namespace jishsfirstpersonmod {
namespace {
BindingRecorder recorder;
std::string recorder_feedback,graphics_error;
void* host_menu{};const EmbeddedWidgets* widgets{};
void section(const char* text) { ImGui::Spacing();widgets->note(text);ImGui::Separator(); }
void help_marker(const char* text) {
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("%s",text);
}
const char* hint(const char* label) {
if(std::strcmp(label,"Link button-glance smoothing to camera Spring")==0) return "Uses camera Spring settings for button glances. Turn this off to adjust glance smoothing separately. Changes made during a glance take effect when you return to centre.";
if(std::strcmp(label,"Spring")==0) return "Smooths how the camera follows your skater. Position offsets move the camera; rotation offsets change where it points. Higher smoothing values make it follow more slowly.";
if(std::strcmp(label,"Disable Spring on foot")==0) return "Turns camera Spring off while walking, jumping and climbing, then restores your Spring setting on board. Linked glance smoothing follows this; separate glance and board transition smoothing are unchanged. Leave this off to see jump attachment changes more clearly.";
if(std::strcmp(label,"Compatibility mode (look controls only)")==0) return "Keeps right-stick look, button glances, First Person controls and comfort options. ReSkate handles camera position, field of view and Spring. Head attachment, first-person bail movement, board transition smoothing and character turning are off. Your settings are kept for full mode.";
if(std::strcmp(label,"Third person while bailing")==0) return "First person returns when you regain control. Button glances pause until the bail ends. With this off in full mode, the camera follows your head during bails. ReSkate stabilization and Follow flips pause while the mod follows your head; your saved settings are kept.";
if(std::strcmp(label,"Third person while getting up")==0) return "Keeps the fall in first person, then lets ReSkate handle the camera while you get up. First person returns when you regain control.";
if(std::strcmp(label,"Camera collision during bails")==0) return "Also applies while getting up if you stay in first person. The camera may move slightly away from your head near surfaces.";
if(std::strcmp(label,"First person on board only")==0) return "Uses third person while walking. First person returns when you get on the board.";
if(std::strcmp(label,"Remember glance on return to marker")==0) return "Restores a toggled glance after returning to your marker. Other teleport actions may also retain it. Menus, bails, leaving first person and changed controls still clear the remembered direction.";
if(std::strcmp(label,"First-person spectator view")==0) return "During throwdown turns, Game of Skate or party spectating, use your First Person binding to switch between the watched head view and the follow camera. Each session starts in follow view. Uses received animation and your camera offsets/FOV; the other player's exact camera input is not transmitted.";
if(std::strcmp(label,"Lock head during on-board body flips")==0) return "Detects front/back flips and body rolls near 90 degrees of tilt while airborne on the board. Blends to your head over 250 ms, follows position and rotation directly, then blends back. Grabs are not required. Near-vertical transitions may also trigger it.";
if(std::strcmp(label,"Attach head position during mounting / jumping")==0) return "Follows your head position while getting on the board and during on-foot jumps. Disable Spring on foot can make the difference less noticeable. ReSkate stabilization and Follow flips pause while the mod follows your head; your settings are kept.";
if(std::strcmp(label,"Attach head position during hippy jumps")==0) return "Follows your head position during hippy jumps. Ollies and other board jumps are unchanged. Disable Spring on foot can make the difference less noticeable.";
if(std::strcmp(label,"Smooth foot/board transitions")==0) return "Adjusts getting on and off the board separately. These settings are separate from camera Spring and button-glance smoothing.";
if(std::strcmp(label,"Right-stick look on foot")==0) return "Move the right stick to look around while walking in first person. Looking pauses while getting on the board.";
if(std::strcmp(label,"Allow looking during jumps and climbing")==0) return "Enables right-stick look and button glances during climbing and supported jumps. Leave this off to keep ReSkate's normal view during climbing and attached jumps. These actions turn only the camera. This does not change head attachment, and look still pauses while getting on the board.";
if(std::strcmp(label,"Enable on-board Right Stick Free Look")==0) return "Hold your modifier to look around while on board in first person. The right stick stops controlling tricks while held. Release the modifier to restore skating controls.";
if(std::strcmp(label,"Invert Y")==0) return "Limits free look left/right of forward and up/down from your centre pitch. Button glance angles stay independent.";
if(std::strcmp(label,"Modifier Key Keeps Right Stick Input Held")==0) return "Keeps the native skating stick position from when the modifier was pressed. Centre the stick to start looking, then look freely while that skating input stays held. Releasing the modifier restores the current physical stick immediately.";
if(std::strcmp(label,"Auto Center On Right Stick Let Go")==0) return "Return to forward and your centre pitch when the stick enters its deadzone while you keep holding the modifier.";
if(std::strcmp(label,"Auto Center On Modifier Key Let Go")==0) return "Releasing the modifier returns the camera even when the stick is still deflected. This takes priority over the right-stick auto-center option.";
if(std::strcmp(label,"Continue Auto Center Until New Look Input")==0) return "After modifier release starts auto-centering, pressing the modifier again keeps the return moving. Only new right-stick camera movement interrupts it; the native held-stick neutral requirement still applies. Turn off for the previous behavior.";
return "";
}
bool readable_checkbox(const char* label,bool* value) {return widgets->toggle(host_menu,label,hint(label),value,true,"N/A");}
void fit_control(const char*,bool=false) {ImGui::SetNextItemWidth(-FLT_MIN);}
bool widget_slider(const char* label,float* value,float low,float high,const char* format,ImGuiSliderFlags flags) {
    ImGui::PushID(label);widgets->field(host_menu,label,nullptr);
    const bool changed=widgets->slider("##value",value,low,high,format,flags);ImGui::PopID();return changed;
}
void settings_change(const Config& config) {
    Command cmd;cmd.kind=CommandKind::camera_settings;cmd.spring=config.spring;cmd.value=config.fov;submit_command(cmd);
}
void record_binding(unsigned action,const Config& config,const MenuSnapshot& model,const dingosdk::ControllerInput& controller) {
    ImGui::PushID(static_cast<int>(action));
    if((action==0 && model.conflicts.toggle) || (action>0 && model.conflicts.glance[action-1])) widgets->note("Paused: buttons already used.");
    const auto mask=action==0 ? config.toggle:config.look[action-1].combo;
    ImGui::TextWrapped("%s",recorder.recording && recorder.action==action ? "Recording...":dingosdk::controller_combo_label(mask,controller.style).c_str());
    if(recorder.recording && recorder.action==action) {
        if(ImGui::Button("Cancel recording")) {recorder.cancel();recorder_feedback="Recording cancelled. Buttons unchanged.";}
    } else {
        ImGui::BeginDisabled(recorder.recording || !controller.available);
        if(ImGui::Button("Record buttons")) {recorder.begin(action,GetTickCount64());recorder_feedback.clear();}
        ImGui::EndDisabled();ImGui::SameLine();ImGui::BeginDisabled(recorder.recording || !mask);
        if(ImGui::Button("Clear")) {Command command;command.kind=CommandKind::binding;command.action=action;submit_command(command);}
        ImGui::EndDisabled();
    }
    ImGui::PopID();
}
}
void publish_menu_error(const std::string& error) {graphics_error=error;log(error);}
void cancel_panel_recording() {recorder.cancel();recorder_feedback.clear();embedded_recording_state(false);}
void draw_embedded_page(void* menu,const EmbeddedWidgets& native,dingosdk::ControllerInput controller,bool focused) {
    host_menu=menu;widgets=&native;
    if(!focused) cancel_panel_recording();
    auto& io=ImGui::GetIO();const float dpi=1;
    auto model=menu_snapshot();auto config=model.config;
    const bool was_recording=recorder.recording;const auto action=recorder.action;
    if(const auto mask=recorder.update(controller,true,focused,GetTickCount64())) {
        Command command;command.kind=CommandKind::binding;command.action=action;command.mask=*mask;
        submit_command(command);recorder_feedback.clear();model=menu_snapshot();config=model.config;
    } else if(was_recording && !recorder.recording) recorder_feedback="Recording cancelled. Buttons unchanged.";
    const bool cancel=ImGui::IsKeyPressed(ImGuiKey_Escape,false);
    if(cancel) cancel_panel_recording();
    native.begin_card(menu,"first-person","FIRST PERSON","Jish's First Person Mod");
        const auto comfort=comfort_snapshot();
        bool enabled=model.requested_enabled.value_or(comfort.initialized ? comfort.master:model.active);
        if(readable_checkbox("Enable First Person",&enabled)) { Command command; command.kind=CommandKind::set_enabled; command.enabled=enabled; submit_command(command); }
        record_binding(0,config,model,controller);
        if(config.native.active) ImGui::TextWrapped("ReSkate's First Person hotkey also works here.");
        if(model.requested_enabled) ImGui::TextWrapped("Waiting for the camera to change view.");
        if(!model.status.empty()) ImGui::TextWrapped("%s",model.status.c_str());
        if(!game_camera_suspended() && *game_camera_status()) ImGui::TextWrapped("%s",game_camera_status());
        static std::string selected,pending;
        static std::vector<std::string> names;
        static bool refresh=true;
        static char name_text[128]{};
        static int operation=0;
        if(refresh) { names=personal_presets();refresh=false;
            if(std::find(names.begin(),names.end(),selected)==names.end()) selected.clear(); }
        ImGui::SetNextItemWidth(-FLT_MIN);
        if(ImGui::BeginCombo("##Preset",selected.empty() ? "Jish's Recommended":selected.c_str())) {
            if(ImGui::Selectable("Jish's Recommended",selected.empty())) selected.clear();
            for(const auto& name:names) if(ImGui::Selectable(name.c_str(),selected==name)) selected=name;
            ImGui::EndCombo();
        }
        if(ImGui::Button("Load")) { cancel_panel_recording();
            if(selected.empty()) load_recommended_preset();else load_personal_preset(selected);
            model=menu_snapshot();config=model.config;
        }
        ImGui::SameLine();
        if(ImGui::Button("Save as...")) { cancel_panel_recording();name_text[0]=0;ImGui::OpenPopup("Save preset"); }
        ImGui::SameLine();
        if(ImGui::Button("Refresh presets")) refresh=true;
        ImGui::BeginDisabled(selected.empty());
        if(ImGui::Button("Overwrite")) { pending=selected;operation=1;ImGui::OpenPopup("Confirm preset change"); }
        ImGui::SameLine();
        if(ImGui::Button("Delete")) { pending=selected;operation=2;ImGui::OpenPopup("Confirm preset change"); }
        ImGui::EndDisabled();
        if(ImGui::IsPopupOpen("Save preset")) ImGui::SetNextWindowSize({std::min(340*dpi,io.DisplaySize.x-16),0});
        if(ImGui::BeginPopupModal("Save preset",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
            if(cancel) ImGui::CloseCurrentPopup();
            ImGui::TextUnformatted("Save current settings and bindings");
            ImGui::SetNextItemWidth(-ImGui::CalcTextSize("Name").x-ImGui::GetStyle().ItemInnerSpacing.x);
            if(ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::InputText("Name",name_text,sizeof(name_text));
            if(ImGui::Button("Save") && save_personal_preset(name_text)) {
                std::string text=name_text;auto first=text.find_first_not_of(' ');
                selected=text.substr(first,text.find_last_not_of(' ')-first+1);refresh=true;ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();if(ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
            ImGui::TextWrapped("%s",menu_snapshot().feedback.c_str());ImGui::EndPopup();
        }
        if(ImGui::IsPopupOpen("Confirm preset change")) ImGui::SetNextWindowSize({std::min(340*dpi,io.DisplaySize.x-16),0});
        if(ImGui::BeginPopupModal("Confirm preset change",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
            if(cancel) ImGui::CloseCurrentPopup();
            ImGui::TextWrapped("%s preset '%s'?",operation==1 ? "Overwrite":"Delete",pending.c_str());
            if(ImGui::Button("Confirm") && (operation==1 ? save_personal_preset(pending,true):delete_personal_preset(pending))) {
                refresh=true;ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();if(ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
            ImGui::TextWrapped("%s",menu_snapshot().feedback.c_str());ImGui::EndPopup();
        }
    const auto stick_change=[&]() {Command command;command.kind=CommandKind::stick_settings;command.stick=config.stick;submit_command(command);};
if(ImGui::CollapsingHeader("Mod updates")) {
    using namespace release_updates;
    const auto update=snapshot();
    if(update.available) {
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.30f,0.90f,0.40f,1.0f));
        ImGui::TextWrapped("%s",update.status.c_str());
        ImGui::PopStyleColor();
    } else native.note(update.status.c_str());
    if(!update.installed.empty()) ImGui::Text("Installed manifest: %s",update.installed.c_str());
    if(!update.latest.empty()) ImGui::Text("Latest compatible release: %s",update.latest.c_str());
    bool enabled=update.automatic;
    if(native.toggle(menu,"Check for updates on launch","One background check per launch. Download and installation require your choice.",&enabled,true,"N/A")) automatic(enabled);
    ImGui::BeginDisabled(update.busy || update.armed);
    ImGui::BeginDisabled(update.ready);
    if(ImGui::Button("Check for updates")) check();
    ImGui::EndDisabled();
    if(update.available) {
        if(ImGui::Button("Download update")) {cancel_panel_recording();download();}
        ImGui::SameLine();if(ImGui::Button("Later")) later();
    }
    if(update.ready) {
        if(ImGui::Button("Install after I close Skate")) {cancel_panel_recording();install_after_exit();}
        native.note("The one-shot installer waits for you to exit Skate normally. Wait for its completion message before starting again. Nothing relaunches automatically.");
    }
    ImGui::EndDisabled();
    if(update.busy && update.total) {
        ImGui::ProgressBar(static_cast<float>(double(update.received)/update.total));
        ImGui::Text("%.1f / %.1f MB",update.received/1048576.0,update.total/1048576.0);
    }
    if(update.busy || update.armed) {if(ImGui::Button("Cancel update")) release_updates::cancel();}
    if(update.ready || update.armed) {if(ImGui::Button("Open update folder")) open_folder();}
}
if(ImGui::CollapsingHeader("Advanced")) {
if(ImGui::TreeNodeEx("View and stabilization",ImGuiTreeNodeFlags_None)) {
        bool stabilize=config.native.stabilize;
        if(native.toggle(menu,"True first person","Keep the horizon level and the view steady; the head's heading is still followed.",&stabilize,config.native.active,"N/A")) native.request_native(26,stabilize,0);
        ImGui::BeginDisabled(!stabilize);
        const auto native_percent=[&](const char* label,float value,int action) {
            ImGui::PushID(label);native.field(menu,label,nullptr);
            if(native.slider("##native-value",&value,0,100,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) native.request_native(action,false,value);
            ImGui::PopID();
        };
        native_percent("Stabilization smoothing",config.native.smoothing,28);
        native_percent("Head nod (up / down)",config.native.head_pitch,29);
        native_percent("Head tilt (horizon roll)",config.native.head_roll,30);
        native_percent("Head bob",config.native.bob,31);
        bool flips=config.native.follow_flips;
        if(native.toggle(menu,"Follow flips","Turn the view with the skater during flips and bails.",&flips,config.native.active,"N/A")) native.request_native(27,flips,0);
        ImGui::EndDisabled();
        bool board_only=config.native.board_only;
        if(native.toggle(menu,"ReSkate third person on foot","Use the game's camera while walking; first person returns on board.",&board_only,config.native.active,"N/A")) native.request_native(32,board_only,0);
        bool cycle=config.native.camera_cycle;
        if(native.toggle(menu,"First person in the camera cycle","R3 cycles through high, low and first person when camera height control is enabled.",&cycle,config.native.camera_cycle_available,"N/A")) native.request_native(33,cycle,0);
        ImGui::Spacing(); section("Camera settings");
        if(readable_checkbox("Link button-glance smoothing to camera Spring",&config.link_smoothing)) {
            Command command; command.kind=CommandKind::link_smoothing; command.enabled=config.link_smoothing;
            submit_command(command); config=menu_snapshot().config;
        }
        help_marker("Uses camera Spring settings for button glances. Turn this off to adjust glance smoothing separately. Changes made during a glance take effect when you return to centre.");
        if(!config.link_smoothing) {
            ImGui::TextUnformatted("Button-glance smoothing");
            const auto look_change=[&]() { Command command; command.kind=CommandKind::look_settings;
                command.spring=config.look_spring; submit_command(command); };
            if(readable_checkbox("Smooth button glances",&config.look_spring.enabled)) look_change();
            const auto strength=[&](const char* label,float& value) {
                fit_control(label);
                if(widget_slider(label,&value,0,100,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) look_change();
            };
            strength("Look up smoothing",config.look_spring.up); strength("Look down smoothing",config.look_spring.down);
            strength("Look left smoothing",config.look_spring.left); strength("Look right smoothing",config.look_spring.right);
            ImGui::TextWrapped("Smooths button glances and their return to centre.");
            ImGui::Separator();
        }
        ImGui::TextUnformatted("On-foot right-stick smoothing");
        const auto stick_smoothing_change=[&]() { Command command; command.kind=CommandKind::stick_smoothing;
            command.spring=config.stick_spring; submit_command(command); };
        if(readable_checkbox("Smooth on-foot right-stick look",&config.stick_spring.enabled)) stick_smoothing_change();
        const auto stick_strength=[&](const char* label,float& value) {
            fit_control(label);
            if(widget_slider(label,&value,0,100,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) stick_smoothing_change();
        };
        stick_strength("Stick up smoothing",config.stick_spring.up); stick_strength("Stick down smoothing",config.stick_spring.down);
        stick_strength("Stick left smoothing",config.stick_spring.left); stick_strength("Stick right smoothing",config.stick_spring.right);
        ImGui::TextWrapped("Smooths on-foot right-stick look independently of button glances and camera Spring.");
        ImGui::Separator();
        ImGui::BeginDisabled(config.look_only);
        bool game_fov=config.fov==0;
        if(readable_checkbox("Use game's default FOV",&game_fov)) { config.fov=game_fov ? 0.0f:90.0f; settings_change(config); }
        ImGui::BeginDisabled(game_fov);
        float fov=config.fov>0 ? config.fov:90;
        fit_control("Field of view");
        if(widget_slider("Field of view",&fov,40,120,"%.0f degrees",ImGuiSliderFlags_AlwaysClamp)) { config.fov=fov; settings_change(config); }
        ImGui::EndDisabled();
        if(ImGui::TreeNode("Spring arm and offsets")) {
            if(readable_checkbox("Spring",&config.spring.enabled)) settings_change(config);
            help_marker("Smooths how the camera follows your skater. Position offsets move the camera; rotation offsets change where it points. Higher smoothing values make it follow more slowly.");
            if(readable_checkbox("Disable Spring on foot",&config.disable_spring_on_foot)) {
                Command command;command.kind=CommandKind::spring_on_foot;command.enabled=config.disable_spring_on_foot;
                submit_command(command);config=menu_snapshot().config;
            }
            help_marker("Turns camera Spring off while walking, jumping and climbing, then restores your Spring setting on board. Linked glance smoothing follows this; separate glance and board transition smoothing are unchanged. Leave this off to see jump attachment changes more clearly.");
            if(config.disable_spring_on_foot) ImGui::TextWrapped("Spring is off on foot and returns on board. Linked glance smoothing follows this setting.");
            const auto slider=[&](const char* label,float& value,float low,float high,const char* format) {
                fit_control(label);
                if(widget_slider(label,&value,low,high,format,ImGuiSliderFlags_AlwaysClamp)) settings_change(config);
            };
            ImGui::TextUnformatted("Position offset (metres)");
            slider("Right",config.spring.offset[0],-3,3,"%.3f"); slider("Up",config.spring.offset[1],-3,3,"%.3f"); slider("Forward",config.spring.offset[2],-3,3,"%.3f");
            ImGui::TextUnformatted("Rotation offset (degrees)");
            slider("Pitch",config.spring.rotation[0],-180,180,"%.1f"); slider("Yaw",config.spring.rotation[1],-180,180,"%.1f"); slider("Roll",config.spring.rotation[2],-180,180,"%.1f");
            ImGui::TextUnformatted("Smoothing strength");
            slider("Moving up",config.spring.up,0,100,"%.0f%%"); slider("Moving down",config.spring.down,0,100,"%.0f%%");
            slider("Moving left",config.spring.left,0,100,"%.0f%%"); slider("Moving right",config.spring.right,0,100,"%.0f%%");
            if(ImGui::Button("Reset arm settings")) { config.spring={}; settings_change(config); }
            ImGui::TextWrapped("Higher values make the camera follow more slowly. Offsets work with Spring off.");
            ImGui::TreePop();
        }
        ImGui::EndDisabled();

ImGui::TreePop();}
if(ImGui::TreeNodeEx("Automatic camera switching",ImGuiTreeNodeFlags_None)) {
        if(readable_checkbox("Compatibility mode (look controls only)",&config.look_only)) {
            Command command;command.kind=CommandKind::look_only;command.enabled=config.look_only;submit_command(command);config=menu_snapshot().config;
        }
        help_marker("Keeps right-stick look, button glances, First Person controls and comfort options. ReSkate handles camera position, field of view and Spring. Head attachment, first-person bail movement, board transition smoothing and character turning are off. Your settings are kept for full mode.");
        ImGui::Spacing(); section("Comfort options");
        const auto comfort_change=[&]() {
            Command command; command.kind=CommandKind::comfort_options;
            command.third_person_bailing=config.third_person_bailing; command.on_board_only=config.on_board_only;
            submit_command(command);
        };
        ImGui::BeginDisabled(!comfort.bail_available && !config.third_person_bailing);
        if(readable_checkbox("Third person while bailing",&config.third_person_bailing)) comfort_change();
        help_marker("First person returns when you regain control. Button glances pause until the bail ends. With this off in full mode, the camera follows your head during bails. ReSkate stabilization and Follow flips pause while the mod follows your head; your saved settings are kept.");
        ImGui::EndDisabled();
        if(!config.third_person_bailing) {
            ImGui::BeginDisabled(config.look_only || !comfort.bail_available);
            if(readable_checkbox("Third person while getting up",&config.third_person_recovery)) {
                Command command;command.kind=CommandKind::third_person_recovery;command.enabled=config.third_person_recovery;submit_command(command);
            }
            help_marker("Keeps the fall in first person, then lets ReSkate handle the camera while you get up. First person returns when you regain control.");
            ImGui::EndDisabled();
                if(config.look_only) ImGui::TextWrapped("Turn off Compatibility mode to use this option.");
        }
        if(config.native.camera_cycle_available && config.third_person_camera_switching()) {
            ImGui::TextWrapped("First Person In Camera Cycle stays off while this is enabled. Your previous setting returns when you turn it off.");
            help_marker("Your camera-cycle preference is kept across restarts. Changes made in ReSkate while this is on become the setting restored later. The First Person checkbox and hotkey still work.");
        } else if(config.native.camera_cycle_available && config.cycle_restore_pending)
            ImGui::TextWrapped("Restoring your First Person In Camera Cycle setting...");
        if(!config.look_only && !config.third_person_bailing) ImGui::TextWrapped("The camera follows your head during bails. Stick look returns when you regain control.");
        ImGui::BeginDisabled(config.look_only || config.third_person_bailing || !bail_collision_available());
        if(readable_checkbox("Camera collision during bails",&config.bail_camera_collision)) {
            Command command;command.kind=CommandKind::bail_collision;command.enabled=config.bail_camera_collision;submit_command(command);
        }
        help_marker("Also applies while getting up if you stay in first person. The camera may move slightly away from your head near surfaces.");
        ImGui::EndDisabled();
        if(config.look_only) ImGui::TextWrapped("Turn off Compatibility mode to use bail camera collision.");
        else if(config.third_person_bailing) ImGui::TextWrapped("Turn off Third person while bailing to use bail camera collision.");
        else if(!bail_collision_available()) ImGui::TextWrapped("Bail camera collision is unavailable. See the mod log.");
        if(!comfort.bail_available) ImGui::TextWrapped("Automatic bail switching is unavailable. See the mod log.");
        ImGui::BeginDisabled(!comfort.board_available);
        if(readable_checkbox("First person on board only",&config.on_board_only)) comfort_change();
        help_marker("Uses third person while walking. First person returns when you get on the board.");
        ImGui::EndDisabled();
        if(!comfort.board_available) ImGui::TextWrapped("Board-only switching is unavailable. See the mod log.");
        if(config.native.board_only) ImGui::TextWrapped("ReSkate's Third person on foot is also enabled. Turn it off to use first person while walking.");
        if(comfort.suppressed) ImGui::TextWrapped("%s",config.third_person_bailing && comfort.bailing ?
            "First person will return after the bail.":config.third_person_recovery && comfort.bailing ?
            "First person will return when you regain control.":"First person will return when you get on the board.");
        if(comfort.suspended) ImGui::TextWrapped("Automatic camera switching is paused. Waiting for your skater.");


ImGui::TreePop();}
if(ImGui::TreeNodeEx("Glances",ImGuiTreeNodeFlags_None)) {
for(unsigned i=1;i<5;++i) {
    ImGui::PushID(static_cast<int>(i));native.field(menu,dingosdk::look_action_labels[i-1],nullptr);
    float angle=config.look[i-1].angle;
    if(native.slider("##angle",&angle,1,dingosdk::look_angle_limit(i-1),"%.0f degrees",ImGuiSliderFlags_AlwaysClamp)) {
        Command command;command.kind=CommandKind::angle;command.action=i;command.value=angle;submit_command(command);
    }
    record_binding(i,config,model,controller);ImGui::PopID();
}
        if(readable_checkbox("Toggle glance controls",&config.toggle_glances)) {
            Command command;command.kind=CommandKind::toggle_glances;command.enabled=config.toggle_glances;submit_command(command);
        }
        ImGui::BeginDisabled(!config.toggle_glances);
        if(readable_checkbox("Remember glance on return to marker",&config.remember_marker_glance)) {
            Command command;command.kind=CommandKind::remember_marker_glance;command.enabled=config.remember_marker_glance;submit_command(command);
        }
        ImGui::EndDisabled();
        help_marker("Restores a toggled glance after returning to your marker. Other teleport actions may also retain it. Menus, bails, leaving first person and changed controls still clear the remembered direction.");
        if((config.toggle&0x100) && (config.toggle&15)) ImGui::TextWrapped("LB/L1 + D-pad First Person buttons do not trigger emotes.");
        help_marker("Opening menus, leaving first person or interrupted controls clears a held glance. With toggle off, hold the buttons to glance; opposite directions cancel.");
        if(glance_emote_filter_ready()) ImGui::TextWrapped("Hold LB/L1 and tap a bound D-pad direction to glance. Double tap to emote; hold the second tap for a held emote.");
        else ImGui::TextWrapped("Glance emote blocking is unavailable. See the mod log.");
        if(recorder.recording) ImGui::TextWrapped("%s",recorder.capture.ready ? "Hold the buttons together, then release them to save.":"Release all controller buttons first.");
        else if(!controller.available) ImGui::TextWrapped("Connect a controller and keep the game focused to record.");
        else if(config.toggle_glances) ImGui::TextWrapped("Press once to hold a glance. Press again to return. Another direction changes the glance.");
        else ImGui::TextWrapped("Hold the buttons to glance. Release to return.");
        if(!recorder_feedback.empty()) ImGui::TextWrapped("%s",recorder_feedback.c_str());


ImGui::TreePop();}
if(ImGui::TreeNodeEx("Attachment and transitions",ImGuiTreeNodeFlags_None)) {
        ImGui::Spacing(); section("Attachment and transitions");
        if(readable_checkbox("First-person spectator view",&config.spectator_first_person)) {
            Command command;command.kind=CommandKind::spectator_first_person;command.enabled=config.spectator_first_person;submit_command(command);
        }
        help_marker("During throwdown turns, Game of Skate or party spectating, use your First Person binding to switch between the watched head view and the follow camera. Each session starts in follow view. Uses received animation and your camera offsets/FOV; the other player's exact camera input is not transmitted.");
        if(!spectator_available() || config.spectator_first_person)ImGui::TextWrapped("%s",spectator_status().c_str());
        ImGui::BeginDisabled(config.look_only);
        if(readable_checkbox("Lock head during on-board body flips",&config.lock_body_flips)) {
            Command command;command.kind=CommandKind::lock_body_flips;command.enabled=config.lock_body_flips;submit_command(command);
        }
        help_marker("Detects front/back flips and body rolls near 90 degrees of tilt while airborne on the board. Blends to your head over 250 ms, follows position and rotation directly, then blends back. Grabs are not required. Near-vertical transitions may also trigger it.");
        ImGui::EndDisabled();
        if(config.look_only) ImGui::TextWrapped("Turn off Compatibility mode to use body-flip head lock.");
        ImGui::BeginDisabled(config.look_only);
        if(readable_checkbox("Attach head position during mounting / jumping",&config.stick.lock_mount_jump)) stick_change();
        help_marker("Follows your head position while getting on the board and during on-foot jumps. Disable Spring on foot can make the difference less noticeable. ReSkate stabilization and Follow flips pause while the mod follows your head; your settings are kept.");
        if(readable_checkbox("Attach head position during hippy jumps",&config.stick.lock_hippy_jump)) stick_change();
        help_marker("Follows your head position during hippy jumps. Ollies and other board jumps are unchanged. Disable Spring on foot can make the difference less noticeable.");
        ImGui::Separator();
        if(readable_checkbox("Smooth foot/board transitions",&config.stick.smooth_transitions)) stick_change();
        help_marker("Adjusts getting on and off the board separately. These settings are separate from camera Spring and button-glance smoothing.");
        ImGui::BeginDisabled(!config.stick.smooth_transitions);
        fit_control("Getting on board smoothing");
        if(widget_slider("Getting on board smoothing",&config.stick.mount_smoothing,0,100,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        fit_control("Getting off board smoothing");
        if(widget_slider("Getting off board smoothing",&config.stick.dismount_smoothing,0,100,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        ImGui::EndDisabled();
        ImGui::TextWrapped("Higher values make getting on and off the board smoother. Bails switch immediately.");
        ImGui::EndDisabled();


ImGui::TreePop();}
if(ImGui::TreeNodeEx("On-foot right stick look",ImGuiTreeNodeFlags_None)) {
        ImGui::Spacing(); section("Off-Board Right Stick Free Look");
        ImGui::PushID("off_board_look");
        ImGui::BeginDisabled(!stick_runtime_ready() && !config.stick.enabled);
        if(readable_checkbox("Right-stick look on foot",&config.stick.enabled)) stick_change();
        help_marker("Move the right stick to look around while walking in first person. Looking pauses while getting on the board.");
        ImGui::EndDisabled();
        if(!stick_runtime_ready()) ImGui::TextWrapped("Right-stick look is unavailable. See the mod log.");
        ImGui::BeginDisabled(!config.stick.enabled);
        ImGui::BeginDisabled(config.look_only);
        int mode=config.stick.turn_character ? 1:0;
        fit_control("Look mode",true);
        if(ImGui::Combo("Look mode",&mode,"Camera only\0Camera and character\0")) {
            config.stick.turn_character=mode==1;stick_change();
        }
        help_marker("Camera only lets you look without turning your skater. Camera and character turns your skater too. Compatibility mode uses camera only.");
        ImGui::EndDisabled();
        fit_control("Horizontal sensitivity");
        if(widget_slider("Horizontal sensitivity",&config.stick.yaw_speed,1,720,"%.0f deg/s",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        fit_control("Vertical sensitivity");
        if(widget_slider("Vertical sensitivity",&config.stick.pitch_speed,1,720,"%.0f deg/s",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        float deadzone=config.stick.deadzone*100;
        fit_control("Stick deadzone",true);
        if(widget_slider("Stick deadzone",&deadzone,1,50,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) {
            config.stick.deadzone=deadzone*.01f;stick_change();
        }
        help_marker("Ignore small stick movements to prevent camera drift. Increase this if the camera moves by itself.");
        if(readable_checkbox("Invert Y",&config.stick.invert_y)) stick_change();
        if(readable_checkbox("Allow looking during jumps and climbing",&config.stick.look_during_parkour)) stick_change();
        help_marker("Enables right-stick look and button glances during climbing and supported jumps. Leave this off to keep ReSkate's normal view during climbing and attached jumps. These actions turn only the camera. This does not change head attachment, and look still pauses while getting on the board.");
        ImGui::EndDisabled();
        ImGui::TextWrapped("Centre the stick after entering first person or changing look mode. On board, it still controls tricks.");
        ImGui::PopID();

ImGui::TreePop();}
if(ImGui::TreeNodeEx("On-board right stick look",ImGuiTreeNodeFlags_None)) {
        ImGui::Spacing(); section("On-board Right Stick Free Look");
        ImGui::PushID("board_look");
        const auto board_change=[&]() { Command command;command.kind=CommandKind::board_settings;
            command.board=config.board;submit_command(command); };
        ImGui::BeginDisabled(!stick_runtime_ready() && !config.board.enabled);
        if(readable_checkbox("Enable on-board Right Stick Free Look",&config.board.enabled)) board_change();
        help_marker("Hold your modifier to look around while on board in first person. The right stick stops controlling tricks while held. Release the modifier to restore skating controls.");
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!config.board.enabled);
        ImGui::TextWrapped("Modifier: %s",recorder.recording && recorder.action==5 ? "Recording...":dingosdk::controller_combo_label(config.board.modifier,controller.style).c_str());
        if(recorder.recording && recorder.action==5) {
            if(ImGui::Button("Cancel")) { recorder.cancel();recorder_feedback="Recording cancelled. Buttons unchanged."; }
        } else {
            ImGui::BeginDisabled(recorder.recording || !controller.available);
            if(ImGui::Button("Record modifier")) { recorder.begin(5,GetTickCount64());recorder_feedback.clear(); }
            ImGui::EndDisabled();ImGui::SameLine();
            ImGui::BeginDisabled(recorder.recording || !config.board.modifier);
            if(ImGui::Button("Clear modifier")) { Command command;command.kind=CommandKind::binding;command.action=5;submit_command(command); }
            ImGui::EndDisabled();
        }
        help_marker("Hold the recorded button or combo while looking. Other actions assigned to the modifier still work. Release the modifier and centre the stick before starting a new look.");
        const auto board_slider=[&](const char* label,float& value,float low,float high,const char* format) {
            fit_control(label);
            if(widget_slider(label,&value,low,high,format,ImGuiSliderFlags_AlwaysClamp)) board_change();
        };
        board_slider("Horizontal sensitivity",config.board.yaw_speed,1,720,"%.0f deg/s");
        board_slider("Vertical sensitivity",config.board.pitch_speed,1,720,"%.0f deg/s");
        float board_deadzone=config.board.deadzone*100;
        fit_control("Stick deadzone",true);
        if(widget_slider("Stick deadzone",&board_deadzone,1,50,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) { config.board.deadzone=board_deadzone*.01f;board_change(); }
        if(readable_checkbox("Invert Y",&config.board.invert_y)) board_change();
        board_slider("Horizontal look limit",config.board.yaw_limit,0,180,"%.0f degrees");
        board_slider("Vertical look limit",config.board.pitch_limit,0,85,"%.0f degrees");
        help_marker("Limits free look left/right of forward and up/down from your centre pitch. Button glance angles stay independent.");
        board_slider("On Board Free Look Smoothing",config.board.look_smoothing,0,100,"%.0f%%");
        help_marker("Smooths the camera while looking, like a dolly camera. Zero responds immediately; higher values add more lag. Native skating input is never smoothed by this setting.");
        if(readable_checkbox("Modifier Key Keeps Right Stick Input Held",&config.board.keep_native_input)) board_change();
        help_marker("Keeps the native skating stick position from when the modifier was pressed. Centre the stick to start looking, then look freely while that skating input stays held. Releasing the modifier restores the current physical stick immediately.");
        if(readable_checkbox("Auto Center On Right Stick Let Go",&config.board.center_on_stick)) board_change();
        help_marker("Return to forward and your centre pitch when the stick enters its deadzone while you keep holding the modifier.");
        if(readable_checkbox("Auto Center On Modifier Key Let Go",&config.board.center_on_modifier)) board_change();
        help_marker("Releasing the modifier returns the camera even when the stick is still deflected. This takes priority over the right-stick auto-center option.");
        if(readable_checkbox("Continue Auto Center Until New Look Input",&config.board.continue_auto_center)) board_change();
        help_marker("After modifier release starts auto-centering, pressing the modifier again keeps the return moving. Only new right-stick camera movement interrupts it; the native held-stick neutral requirement still applies. Turn off for the previous behavior.");
        ImGui::BeginDisabled(!config.board.center_on_modifier && !config.board.center_on_stick);
        board_slider("Auto-center smoothing",config.board.center_smoothing,0,100,"%.0f%%");
        help_marker("Uses the same damping as other look controls. Higher values return more slowly.");
        ImGui::EndDisabled();
        board_slider("Center pitch angle",config.board.center_pitch,-85,85,"%.0f degrees");
        help_marker("The resting on-board view while this feature is enabled. Zero looks forward, positive looks up and negative looks down. Auto-center returns to this angle.");
        if(recorder.recording && recorder.action==5) ImGui::TextWrapped("%s",recorder.capture.ready ? "Hold the buttons together, then release them to save.":"Release all controller buttons first.");
        if(!recorder_feedback.empty()) ImGui::TextWrapped("%s",recorder_feedback.c_str());
        ImGui::EndDisabled();
        if(ImGui::Button("Reset on-board look")) {const auto modifier=config.board.modifier;config.board=BoardLookSettings{};config.board.modifier=modifier;board_change();}
        ImGui::PopID();

ImGui::TreePop();}
}
    model=menu_snapshot();
    if(!model.feedback.empty()) native.note(model.feedback.c_str());
    if(model.save_failed) {
        native.note(model.save_error.c_str());
        if(ImGui::Button("Retry saving")) {Command command;command.kind=CommandKind::retry_save;submit_command(command);}
    }
    if(!graphics_error.empty()) native.note(graphics_error.c_str());
    native.end_card();
    embedded_recording_state(focused && recorder.recording);
    host_menu=nullptr;widgets=nullptr;
}
}
