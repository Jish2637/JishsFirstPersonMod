# Jish's First Person Mod 1.36.9 — ReSkate 2.1.4-2

Compatibility update based on the published 1.36.8 release. All private ReSkate addresses and globals have been re-derived for the official 2.1.4-2 DLL. This includes mutex thunks and their implementation, camera callbacks, controller/HID capture, embedded menu and ImGui allocation, spectator records and the MSC session bridge.

Updated the model debug revision offset, embedded menu position, session view/mutex/connected offsets, and every moved native function/global address. The remote-player stride remains 896 bytes. Existing camera settings and features are retained. The exact runtime identity is shared by startup, private bridges and the updater.

Close Skate and back up your current mod and dinput8.dll. Extract the complete release ZIP beside Skate.exe, merge Mods and scripts, enable the mod in the launcher and restart. Existing personal settings, presets, other mods and dinput8_mods.txt are retained. The ZIP contains the standard multi-mod loader; the ReSkate runtime itself is not bundled.

Required ReSkate.dll SHA-256: `f7b1562667892187356b8617a3964ceaefea04324bfa7850c05d340da01c09cd`. Runtime support is pinned to this exact official release.

Release build and all 51 automated tests pass; detailed results are recorded in docs/CTEST_214_2.txt. The offline audit verifies 33 native function entries, 68 compiled layout fields/sizes, globals and call relationships; separate game code/vtable, collision and emote checks also pass. Publication was authorized on 4 November 2026. The released client files match the verified local build. Live gameplay and hardware-specific checks are described in the validation checklist.

See [audit details](docs/MIGRATION_214_2.md) and [installation/testing](docs/VALIDATION_214_2.md). Historical documents retain their original version pins.

Build on Windows x64 using Visual Studio 2026 C++ tools: ./build.ps1, then ./package.ps1. The offline tools use the pinned dependencies in tools/requirements.txt and the official tagged source. Compile tools/layout214_2 against that source, then run tools/audit_214_2.py with the DLL and --upstream source folder. The historical 2.1.4 DLL at ../upstream/ReSkate-2.1.4.dll is used for the independent mutex implementation comparison.
