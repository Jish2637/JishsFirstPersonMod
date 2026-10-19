## On-board body-flip head lock (1.30)

Camera > Attachment and transitions > Lock head during on-board body flips defaults off. It is independent of right-stick look and the mounting/hippy-jump switches, and requires full mode. It uses the skater's body up axis and verified on-board air states 200/201; no grab is required.

Near 80 degrees of tilt from world upright, sustained increasing tilt starts a 250 ms blend into head position and rotation with the configured camera offsets. After the handoff, the camera follows directly without Spring or continuous smoothing. Settling rotation, landing, dismounting or disabling the option starts a 250 ms return. Glances pause during the handoff/lock; on-board trick input remains available. Flip targets use configured camera offsets directly; movement-induced Spring lag is excluded.

Near-vertical airborne transitions may resemble flips. This motion-based detector needs gameplay validation. Menus, focus loss, teleports, invalid poses, camera ownership changes and bails discard the flip state. The setting is saved in current settings and personal presets; older presets default it off.

## Personal presets and menu tabs (1.29)



Use the preset selector below the tabs to choose Jish's Recommended or a saved personal setup, then press Load. Save as creates a named snapshot of current camera, comfort, stick and glance settings plus controller bindings. Manual changes save your current settings automatically; use Overwrite to update a preset. Overwrite and Delete require confirmation. The recommended preset is read-only.



Preset loading keeps your current First Person on/off choice, panel shortcut, compatibility mode, native controls and camera-cycle restoration state. Conflicting bindings are paused and shown in Controls. Personal presets persist in `scripts/Custom/FirstPerson/presets` across updates and restarts.



Open Insert → Skater → First Person. The primary binding and presets are at the top; Advanced groups contain view/stabilization, automatic camera switching, glances, attachment/transitions, on-foot look and on-board look. Hover controls for help. Recording stays beside its corresponding feature.



## Detailed Information:



## Look smoothing



Link look smoothing to camera spring is enabled by default. Unlink it to edit the hotkey look smoothing checkbox and four strengths independently. FOV, camera offsets and base rotations still sync with ReSkate. First unlink copies the current smoothing; subsequent relinks retain the independent values. Changing linkage during a held glance takes effect after returning to centre. Spring off turns immediately.



## Comfort options



Enable First Person is the master switch in either menu and for the controller toggle. The third-person and board-only options default off and save automatically:



- Third person while bailing: a positive native wipeout signal selects third person. First person returns when the documented native riding or normal on-foot movement resumes, wipeout requests clear, and recovery/runout is inactive for two consecutive samples.

- Third person while getting up: with Third person while bailing off, the fall stays in first person and getting up uses third person. First person returns when normal control resumes. Requires full mode.

- Camera collision during bails: enabled by default on the supported build. This is work in progress; the camera can still stick near surfaces.


- First person on board only: walking or bailing selects third person; riding selects first person. With both options enabled, first person requires riding and no active bail/recovery.



The master stays checked while the camera temporarily uses third person. Turning it off cancels automatic restoration. Freecam, Noclip and explicit native camera restoration also disarm it. Turning a comfort option off restores the unrestricted first-person view if the master is still enabled.



Unknown or unavailable player state suspends automatic changes. A mismatched native bail/movement contract makes the bail option unavailable with a clear message. Gameplay look controls remain inhibited while in third person or while Insert is open. Bail restoration and focus/device/activation changes require neutral entry.



ReSkate owns camera activation, spring, head hiding and restoration. This mod submits requests through ReSkate's existing queue and does not modify the reskate.dll file.



This release builds successfully and passes the automated checks. Native code and movement vtable contracts were also checked against the installed executable. Live bail/foot/board transitions, recovery, respawns and multiplayer still require player testing.



Open **Insert → Skater → First Person**. Stock ReSkate owns the menu and its controller/mouse/keyboard navigation. F9 is no longer handled by the mod. Leaving the page or losing focus cancels recording; release held buttons before navigation resumes. See [controller menu controls](CONTROLLER_MENU.md).



For a tested starting point, select **Jish's Recommended** and press **Load** on the First Person page. This applies the bundled tested controller bindings, camera/spring values, independent look smoothing, comfort options and on-foot stick settings, and saves automatically. The recommended snapshot now enables Disable Spring on foot, keeping camera Spring on board and making walking and parkour more responsive. Existing settings are retained until you press the button. The preset does not toggle First Person or change stock Noclip/boost bindings; conflicting stock chords are marked and paused while the other preset settings apply. The bundled file is scripts/Custom/FirstPerson/presets/bundled/recommended.ini. On-foot right-stick smoothing is disabled and on-board right-stick look is enabled in the current recommended preset; glance smoothing and on-board look smoothing 12 remain enabled.



Climbing and mantling use native camera position unless Disable Spring on foot is enabled. There is no unconditional attachment for special off-board animation or ragdoll states. Enabled mounting/on-foot jump and hippy-jump attachment options attach position to the validated animation head. To see the effect of these attachment switches, turn Disable Spring on foot off. With it enabled, camera Spring is already disabled during off-board movement, so attachment changes may be less noticeable. Looking continues across entry and exit without another neutral-input cycle. During these actions Camera and character mode rotates only the camera; native movement retains character facing. Facing resumes on verified normal ground with its bias rebased to the current character heading.



Non-bail animation/state changes, including hippy-jump-style changes within riding mode, use Camera Spring and its directional strengths. Ordinary native movement is not smoothed twice.



Normal foot/board camera handoffs work with stick look disabled and have their own Smooth foot/board transitions checkbox and separate Getting on board / Getting off board sliders. Defaults are 70% mounting and 35% dismounting. Higher values soften the handoff for longer; zero or disabling the checkbox switches immediately. These settings save automatically and are independent of camera and look Spring. Bails, invalid player state and lifecycle resets cancel the blend. Board input is released immediately, independently of the camera transition.



Enable Right-stick look on foot under Insert. Choose Camera only or Camera and character. Camera and character turns the native physics-facing target even while stationary. The game retains native movement/collision processing; forward movement alignment and multiplayer should be checked in-game.



The feature defaults off, Camera only, 120 degrees/second horizontally, 90 vertically, 15 percent radial deadzone and non-inverted Y. Sensitivity, deadzone and invert-Y save automatically. The analog pitch range is +/-85 degrees. Releasing the stick retains the view. Horizontal turns use world-up yaw with a level horizon in both modes, preserving the native camera position and spring arm. Existing glance combos temporarily offset it using the selected smoothing.



Validated Offboard movement in active first person owns the right stick. With Allow looking during jumps and climbing enabled, this includes special climbing and jump movement; off restores native looking during climbing and enabled jump locks. Board ollies/airtime retain native trick input. Bails, invalid ownership/head data, third person, menus, focus loss and disconnection release capture and retain neutral-entry safeguards.





## ReSkate compatibility



Verified bridge profiles cover ReSkate 1.1.5 through 1.1.8 and 2.0.0 through 2.0.6. The bridge selects addresses by exact DLL SHA-256 and status-export RVA. The 1.1.6 profile fixes the camera-hook rejection after updating ReSkate. Replace only the mod DLL/bootstrap; keep your current reskate.dll.





Under Spring arm and offsets, enable **Disable Spring on foot** for more responsive walking and parkour. While First Person is active, verified Offboard ownership (including climbing and mantling) temporarily turns native camera Spring off. Getting back on board, turning this option off or leaving First Person restores your configured Spring setting. Your saved Spring enable, strengths and offsets are preserved; this option never turns a manually disabled Spring on. Linked look smoothing follows the effective Spring switch; independent look smoothing and foot/board transition controls retain their own settings. The option defaults off for existing profiles.



With Third person while bailing off in full mode, bails follow the calibrated head position and full head rotation, including roll. Custom look is blocked during uncontrolled tumbles and animation-driven get-up movement. This camera-only test captures the facing direction before the native get-up animation spins, smoothly levels pitch/roll, and holds that direction while position keeps following the head. The hold remains through controllable recovery so it cannot snap back to the head; existing transitions release it as normal movement resumes. During this stand-up phase the native motion target is also held facing the cameraâ€™s fixed view direction. Position and surface-up are preserved; the facing override releases when verified movement control returns. Right-stick look, when enabled, becomes available as soon as the owned Ground movement child and native flags prove the ordinary motion path has resumed without animation or motion overrides. The recovery/runout flag alone does not unlock look. Recovery look starts at zero after neutral stick entry, remains relative to the moving head, and never turns the character. Glances clear on bail entry and remain blocked until bail evidence clears. The existing camera transition settings blend back to ordinary camera behavior. Missing head/calibration or unknown control evidence retains the native-data safeguards. The jump/climb look toggle does not alter bail behavior.



For bail diagnostics, keep First Person enabled, turn Third person while bailing off, then bail and stand up. The automatically rotated trace writes `scripts/Custom/FirstPerson/logs/bail-camera-trace-v11.csv`, preserving older CSVs. It records native/final camera and animation poses, Spring/input/recovery evidence, attachment validity, facing eligibility, bail view phase, native control flags, motion override, calibrated target rotation and recovery look offsets. Writing runs on the settings worker rather than the camera thread.



Leave **Disable Spring on foot** off if you want to see the difference between the mounting/jump and hippy-jump attachment settings. This is guidance only: the controls remain available and no settings are automatically changed.



**Hippy-jump attachment:** The option defaults off and requires Right-stick look on foot. Confirmed hippy jumps attach head position and bypass positional Spring/handoffs while permitting looking. Ollies, flip tricks and ordinary board airtime are excluded. To see the effect of this switch, turn Disable Spring on foot off; with it enabled, attachment changes may be less noticeable.



**Version 1.6:** Preserves displayed direction through bail handover with immediate input restoration and independent orientation reconciliation. Climbing and enabled jump attachments now allow looking. Releases use `JishsFirstPersonMod-1.6-reskate-1.1.6.zip`.



**Version 1.5 recommended preset:** Updated from JishÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢s current saved settings. Hippy-jump locking, mounting/on-foot jump locking, Disable Spring on foot and Third person while bailing are enabled. Load the preset explicitly through the mod panel; loading retains your current menu key and stock reserved bindings.





## Game camera ownership (1.15)



A new independent spectator ownership observer preserves the First Person master preference and all existing restrictions. It is enabled for verified ReSkate 1.1.8 and 2.0.0 through 2.0.6. Replay pre-entry and general scripted/game-mode takeover contracts remain unimplemented; see GAME_CAMERA_OWNERSHIP.md for signals, limitations and acceptance checks.



## Compatibility mode



Enable **Compatibility mode (look controls only)** in the mod panel, or set `look_only=1` in the mod INI. Right-stick camera look and directional glances remain available with their sensitivity, bindings and look smoothing. The First Person checkbox and hotkey remain available; manual activation uses ReSkate, and Third person while bailing / First person on board only continue to apply. The mod passes through camera position, FOV, Spring and stabilization, and disables character turning, head attachment, bail holds, transition smoothing. Comfort options remain active. Other saved settings are preserved for returning to full mode. Look controls still pause during bails, menus and unsupported player states, and on-board right-stick input remains available for tricks. This mode adds look rotation to ReSkate's supported first-person camera; compatibility with another mod replacing that camera requires in-game testing.





## Toggle glance controls



The single Toggle glance controls checkbox sits beside the glance bindings and defaults off. Off retains hold-to-glance behavior. On, press a glance combo to look and hold, press the same combo again to return, or press another direction to replace the previous selection. Releasing a direction rearms its combo while a modifier may remain held. Simultaneous new direction presses are ignored. Existing angles and smoothing still apply.



Menus, focus loss, controller disconnection/change, leaving first person, blocked camera states, world/player changes and binding/angle/checkbox edits clear the selection. Neutral input is required when controls resume. The checkbox saves as toggle_glances; the selected direction is never saved. This works in full and compatibility modes.



### Third-person bails and native camera cycling



On ReSkate 2.0.6, 2.0.7 and 2.0.8, enabling **Third person while bailing** automatically disables **First Person In Camera Cycle**. The mod menu shows this next to the bail option. The native preference is retained across restarts and preset changes, then restored when third-person bails is disabled. Editing the native camera-cycle preference while the override is active updates the preference to restore; the effective setting stays off. This also applies in Compatibility mode. Older ReSkate versions are unchanged.



### Glances and emotes (1.26, ReSkate 2.0.7)



Hold LB/L1 and tap a bound D-pad direction to glance. Double tap the same direction within 250 ms to emote; hold the second tap for a held emote. The camera returns to centre for the emote. Release the D-pad to start another glance; LB/L1 may stay held. This applies to both hold and toggle glances while first-person glance controls are active. Other bindings and normal third-person emotes are unchanged.



The General, Controls and Camera tabs sit directly below the title/version, above First Person and presets. Equal-width square tiles span the menu; the selected tab uses ReSkate blue, with dark grey inactive tabs and lighter hover feedback.



Routine autosaves show “Current settings saved” for three seconds after the write completes, then clear. The footer shows pending writes as “Saving current settings”; explicit preset load/save/overwrite/delete confirmations name the preset and disappear after four seconds. Errors remain visible. Current-setting changes do not overwrite personal presets.


## On-board Right Stick Free Look

See ON_BOARD_FREE_LOOK.md for operation and fresh defaults. Existing saved choices are preserved.

Continue Auto Center Until New Look Input defaults on. A return started by modifier release continues through modifier re-presses until it reaches center or accepted right-stick camera movement interrupts it. Held native input waiting for neutral arming does not interrupt the return. Turning this option off restores the previous re-press behavior. This and all on-board settings save/load with personal presets; menu keyboard/controller access stays global.

## Menu and controller controls

See CONTROLLER_MENU.md. Open Insert → Skater → First Person; there is no separate panel or mod F9 shortcut.
Front-flip offset fix: the flip target uses animated head pose plus configured position/rotation offsets directly. It no longer inherits the transient native-camera/head delta used by bail calibration. Regression tests cover both pitch directions, zero offsets, configured offsets and a Spring-lagged normal camera. Gameplay validation of this fix is pending.

### First-person spectator view (1.30)

Enable **Camera > Attachment and transitions > First-person spectator view** to make the existing First Person hotkey/controller binding switch spectator views during throwdowns, Game of Skate and manual party spectating. Each session starts with the normal follow camera. The chosen view follows player changes within that session and resets when spectating ends. Your own First Person preference is preserved.

Head view uses ReSkate's first-person camera axes and eye midpoint, including its clearance ahead of the face. Your configured offsets, FOV, Spring and native stabilization settings apply. The rendered animation refines the camera in the same frame. A 250 ms moving-target blend handles view changes. The watched head is hidden only in the local render buffer. Missing, stale or unverified targets keep the follow camera; status explains the fallback. The option defaults off, is included in personal presets and requires the verified ReSkate 2.0.7 party spectator path. Remote camera settings and look input are not transmitted. Gameplay validation remains pending.

On-foot right-stick smoothing has its own enable switch and four directional strengths. Button-glance smoothing can still follow camera Spring. Older settings migrate once using the previous on-foot smoothing behavior, including Disable Spring on foot when linked.
