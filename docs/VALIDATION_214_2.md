# 1.36.9 for ReSkate 2.1.4-2

Publication authorized on 4 November 2026. All 51 automated tests passed. The checklist below records suggested live validation; it does not claim every optional feature or controller was tested.

## Install

Close Skate and back up dinput8.dll and scripts/Custom/JishsFirstPersonMod.dll, plus personal FirstPerson settings/presets. Extract the complete ZIP into C:/Games/ReSkate beside Skate.exe, merging Mods and scripts. Enable Jish's First Person Mod in the launcher and restart. Keep other mods and scripts/Custom/dinput8_mods.txt.

The required ReSkate.dll SHA-256 is f7b1562667892187356b8617a3964ceaefea04324bfa7850c05d340da01c09cd. The installed DLL was rechecked after the user updated it and matches the official 2.1.4-2 release.

## Gameplay checks

- Startup and Insert > Skater > First Person: correct page, no compatibility errors or crash, and normal menu navigation after switching tabs.
- First-person toggle on foot/on board, FOV, Spring, offsets, attachment, stick look, glances, marker reset, bails/getting up and transitions with your usual settings.
- Record and cancel a controller binding, including a held chord; menu navigation resumes after release.
- Verify changes made in the First Person menu update live and persist across restart. This build corrects the shifted partial-model debug revision.
- Check usual controllers (DS4/DualSense USB/Bluetooth if available), other buttons/sticks and the existing padded DS4 report handling.
- Check multiplayer spectating and the optional MSC [FP] presence indicator. The session mutex/view layout changed in 2.1.4-2.
- Confirm other installed DLL mods, settings and presets are preserved.

Logs: scripts/Custom/FirstPerson/logs. Report any crash, compatibility message, camera/menu problem or controller regression. Automated tests and offline audits do not establish live gameplay correctness.

Rollback requires the backed-up mod/loader together with their matching original 2.1.4 runtime. Version 1.36.8 is not compatible with 2.1.4-2.
