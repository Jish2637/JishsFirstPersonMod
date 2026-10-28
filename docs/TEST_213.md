# ReSkate 2.1.3 local test candidate

Mod version: 1.36.7, based on published 1.36.6. The user confirmed this candidate working in game and approved publication as 1.36.7. The checklist below records the suggested validation steps; it does not claim every optional feature or controller was tested.

## Install

1. Close Skate. Back up your existing `dinput8.dll`, `scripts/Custom/JishsFirstPersonMod.dll`, and personal FirstPerson settings/presets.
2. Verify ReSkate is Release 2.1.3. The supported DLL SHA-256 is `139e932ad6328b898b2ff8c2aac3e0cd5ced6545c2f041aa8534239f97e59f9b`.
3. Extract the full test ZIP into the game folder beside `Skate.exe`. Merge `Mods` and `scripts`, replacing the bundled mod files and loader. Keep other mods and your mod list.
4. Enable Jish's First Person Mod in the launcher, restart, and open **Insert > Skater > First Person**.

## Check in game

- Startup: no crash, compatibility error, or missing First Person page. The mod log should identify version 1.36.7 and validated ReSkate 2.1.3.
- Toggle first person on foot and on board. Change FOV, Spring and offsets. Leave/reopen the menu and restart to check persistence.
- Test head attachment, right-stick look, button glances, marker reset, bails/getting up and camera transitions using your usual settings.
- Record and cancel a controller binding, including a held chord. Confirm normal stock menu navigation resumes after release.
- Test your usual controllers. If available, check DS4 Bluetooth specifically: glance/toggle D-pad consumption, other buttons, sticks and menu capture should work without dropped input.
- In multiplayer, check remote players and first-person spectating. On an MSC-enabled server, check the optional [FP] presence indicator.
- Open the new Mod Manager tab and return to the First Person page; check the embedded menu remains usable.
- Check other installed mods still load normally.

Logs are in `scripts/Custom/FirstPerson/logs`. Report any crash, compatibility message, missing menu, camera issue or controller regression before publishing. Automated tests and offline address checks do not establish live gameplay correctness.

## Roll back

Restore your backed-up mod files and loader. The previous 1.36.6 build requires its matching ReSkate 2.1.2 runtime; it is not a compatible fallback on 2.1.3.
