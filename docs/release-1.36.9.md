# 1.36.9 — ReSkate 2.1.4-2

Updates the first-person mod for the exact official ReSkate Release 2.1.4-2 DLL. All private function/global addresses were re-derived, including native mutex thunks and implementation, camera callbacks, controller/HID capture, embedded menu/ImGui, spectator and MSC bridges.

Corrected the shifted debug revision at 26128 and menu at 28968, plus Session mutex 783624, view 781368 and multiplayer connected flag 826. The runtime identity used by startup, private bridges and the updater now targets the official 2.1.4-2 DLL.

The released client files match the verified local build. All 51 automated tests pass, including native ABI/entry tamper checks, original 2.1.4 rejection and debug revision memory boundaries. The offline audit checks 33 native entries and 68 compiled layout fields/sizes, plus global/call relationships and the supported game addresses. See MIGRATION_214_2.md and VALIDATION_214_2.md.

Close Skate and back up the existing mod and dinput8.dll. Extract the complete client ZIP beside Skate.exe, merge Mods and scripts, enable the mod in the launcher and restart. Existing personal settings, presets, other mods and dinput8_mods.txt are preserved. Older updaters may require a manual installation.
