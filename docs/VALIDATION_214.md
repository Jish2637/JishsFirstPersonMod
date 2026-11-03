# 1.36.8 for ReSkate 2.1.4

The user confirmed the candidate working in game and approved release on 3 November 2026. All 51 automated tests passed. The checklist below records suggested coverage; it does not assert that every optional feature/controller was tested.

## Install

1. Close Skate. Back up dinput8.dll and scripts/Custom/JishsFirstPersonMod.dll, plus your personal FirstPerson settings/presets.
2. Extract the complete release ZIP into C:/Games/ReSkate, beside Skate.exe. Merge Mods and scripts; replace the bundled mod files and loader. Preserve other mods and scripts/Custom/dinput8_mods.txt.
3. Enable Jish's First Person Mod in the launcher and restart. Open Insert > Skater > First Person.
4. The log should report version 1.36.8 and validated ReSkate 2.1.4. Logs are in scripts/Custom/FirstPerson/logs.

## Checks

- Startup and embedded First Person menu: no crash, missing page, wrong navigation or compatibility errors. Return to the page after visiting other tabs.
- Toggle first person on foot/on board. Check FOV, Spring, offsets, head attachment, right-stick look, glances, marker reset, bails/getting up and transitions with your usual settings.
- Record and cancel a binding, including a held controller chord. Menu navigation must resume after release.
- Check your usual controllers, including DS4/DualSense USB or Bluetooth if available. Other buttons/sticks and 547-byte DS4 Bluetooth reports should remain usable.
- In multiplayer, check first-person spectating with a remote player. This build corrects the remote-record stride from 888 to 896 bytes. Check the optional [FP] marker on an MSC-enabled server.
- Confirm personal settings/presets persist and other DLL mods still load.

The automated suite and offline audits do not establish live gameplay correctness. Report any crash, compatibility message, menu/camera problem or controller regression when reporting an issue.

To roll back, restore the backed-up mod and loader along with their matching ReSkate 2.1.3 runtime. The previous 1.36.7 mod is not a compatible fallback on 2.1.4.
