# Jish's First Person Mod 1.36.11 â€” ReSkate 2.1.4-5

Compatibility update for the official ReSkate 2.1.4-5 DLL. All 33 native function entries, mutex contracts, global pointers, and 68 compiled layout fields and sizes were rechecked against the new build.

The DLL code section is byte-identical to 2.1.4-4. The existing mutex, offset and address values all passed verification. Startup, private bridges and the updater now use the new runtime identity.

Close Skate and back up your current mod and dinput8.dll. Extract the complete release ZIP beside Skate.exe, merge Mods and scripts, enable the mod in the launcher and restart. Existing personal settings, presets, other mods and dinput8_mods.txt are retained. The ZIP contains the standard multi-mod loader; the ReSkate runtime itself is not bundled.

Required ReSkate.dll SHA-256: `0820fa71c1cb716f4222eb9027130933aa9e3db4ce20b46ef555760be997dbde`. Runtime support is pinned to this exact official release.

Release build and all 51 automated tests pass; detailed results are recorded in docs/CTEST_214_5.txt. The offline audit verifies 33 native function entries, 68 compiled layout fields/sizes, globals and call relationships; separate game code/vtable, collision and emote checks also pass. The user confirmed the release works in game. The validation checklist covers additional hardware-specific checks.

See [audit details](docs/MIGRATION_214_5.md) and [installation/testing](docs/VALIDATION_214_5.md). Historical documents retain their original version pins.

Build on Windows x64 using Visual Studio 2026 C++ tools: ./build.ps1, then ./package.ps1. The offline tools use the pinned dependencies in tools/requirements.txt and the official tagged source. Compile tools/layout214_5 against that source, then run tools/audit_214_5.py with the DLL and --upstream source folder. The historical 2.1.4-4 DLL at ../upstream/ReSkate-2.1.4-4.dll is used for the independent mutex implementation comparison.
