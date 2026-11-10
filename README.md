# Jish's First Person Mod 1.36.10 â€” ReSkate 2.1.4-4

Compatibility update based on the published 1.36.9 release. All private ReSkate addresses and globals have been re-derived for the official 2.1.4-4 DLL. This includes mutex thunks and their implementation, camera callbacks, controller/HID capture, embedded menu and ImGui allocation, spectator records and the MSC session bridge.

Updated the model debug revision offset, embedded menu position, session view/mutex/connected offsets, and every moved native function/global address. The remote-player stride is now 912 bytes and the peer stride is 3136 bytes. Existing camera settings and features are retained. The exact runtime identity is shared by startup, private bridges and the updater.

Close Skate and back up your current mod and dinput8.dll. Extract the complete release ZIP beside Skate.exe, merge Mods and scripts, enable the mod in the launcher and restart. Existing personal settings, presets, other mods and dinput8_mods.txt are retained. The ZIP contains the standard multi-mod loader; the ReSkate runtime itself is not bundled.

Required ReSkate.dll SHA-256: `1bcd400b6c487f497ab0110263444864c0269a6309f7143d22b5bc04863d544e`. Runtime support is pinned to this exact official release.

Release build and all 51 automated tests pass; detailed results are recorded in docs/CTEST_214_4.txt. The offline audit verifies 33 native function entries, 68 compiled layout fields/sizes, globals and call relationships; separate game code/vtable, collision and emote checks also pass. Live gameplay and hardware-specific checks remain pending and are described in the validation checklist.

See [audit details](docs/MIGRATION_214_4.md) and [installation/testing](docs/VALIDATION_214_4.md). Historical documents retain their original version pins.

Build on Windows x64 using Visual Studio 2026 C++ tools: ./build.ps1, then ./package.ps1. The offline tools use the pinned dependencies in tools/requirements.txt and the official tagged source. Compile tools/layout214_4 against that source, then run tools/audit_214_4.py with the DLL and --upstream source folder. The historical 2.1.4-2 DLL at ../upstream/ReSkate-2.1.4-2.dll is used for the independent mutex implementation comparison.
