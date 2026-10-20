# Native head movement menu option — local test2

The input-passthrough experiment was user-confirmed working. This build moves its switch into the existing Insert menu and saved settings. Camera transforms, head bones, animation callbacks, networking and the tested passthrough eligibility remain unchanged.

## Use

Close Skate before extracting JishsFirstPersonMod-1.36.1-native-head-look-test2-reskate-2.1.1.zip beside Skate.exe, merging Mods and scripts. Open **Insert → Skater → First Person → Advanced → On-foot right stick look → Native head movement**. Enable Right-stick look on foot and select Camera only to use the option.

The new option defaults OFF, preserving the existing camera feel. It is saved through the normal settings commands, survives restarting, and is included in personal presets. Older settings and presets without the new option use OFF. Applying an older preset therefore turns this option off. No settings reset or bundled preset change is introduced.

The separate native-head-look-test.ini switch is no longer read or included in the package. An old copy left in your game folder has no effect. All control now comes from the menu.

## Hover text

> Lets Skate move your character's head when you use right-stick look on foot in Camera only mode, so other players can see you looking around. This changes how the camera feels. Turn it off to keep the previous camera feel. Does not apply to Camera and character or on-board look.

The same text is supplied to the native checkbox hover and help marker. The option is disabled when on-foot right-stick look is off, or when Camera and character is selected. Compatibility mode retains its existing effective camera-only behavior.

## Behavior and persistence

Settings field: StickSettings::native_head_look. Saved key: stick_native_head_look (normal settings/preset INI; users need not edit it). Reuses CommandKind::stick_settings and the existing settings/preset owner. The runtime reads the current option directly, removing the test atomic flag, file polling and separate INI. Existing focus, menu, owner, device, first-person, board and recovery guards remain intact.

## In-game acceptance

- Enable/disable the option in the menu using controller, mouse or keyboard; check hover wording and changed camera feel.
- Ask an observer to confirm native head movement in Camera only mode with the option ON; confirm OFF restores the previous behavior.
- Check saved state after restarting and save/load a personal preset with the option enabled.
- Camera and character should disable the option without erasing its saved selection. On-board input, glances and toggle emote protection should remain unchanged.
- Confirm any leftover native-head-look-test.ini cannot override the menu choice.

This is a local build. The preceding passthrough experiment was confirmed in game; the new menu/persistence integration still needs user acceptance. The inherited marker-reset test2 retains its separate acceptance status. Game installation untouched during preparation; nothing published.

## Automated validation

- Final Release build: PASS; **47/47 tests passed**.
- Embedded UI regression exercises the actual new checkbox command and verifies the camera-feel warning is supplied to the native widget. Context/allocator restoration and UI scope checks pass.
- Settings regression checks default OFF, legacy settings without the new key, normal stick-settings commands, saving/enabling/disabling, and settings/preset round trips. Existing personal-preset UI integration retains the option.
- Existing scoped passthrough regressions pass. Camera transforms, native sampler, head/animation, marker handling and physical filter implementations match the user-confirmed test1 source. The camera-math header adds only the saved flag; its algorithm is unchanged.
- Official ReSkate 2.1.1 native binary audit: PASS; no native address/ABI changes.
- Separate diagnostic INI is absent from the package and no production source reads it. Bundled Recommended and personal-data preservation remain unchanged.
- New menu/persistence behavior still awaits in-game acceptance. No game installation changes or publication.
