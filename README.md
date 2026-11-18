# Jish's First Person Mod 1.36.13 - ReSkate 20261116

Input-blocking patch for ReSkate Build 20261116. Game controller polls forwarded through ReSkate now receive the mod's stick, glance, toggle and menu input filters. Forwarded XInput calls are filtered once, with scoped physical reads for controller sampling and binding capture.

The mod also continues preparing Raw Input and HID filters when XInput readers are unavailable. Regression coverage includes forwarded polls, private physical reads, stick capture and menu capture.

Close Skate and back up your current mod and dinput8.dll. Extract the complete release ZIP beside Skate.exe, merge Mods and scripts, enable the mod in the launcher and restart. Existing personal settings, presets, other mods and dinput8_mods.txt are retained. The ZIP contains the standard multi-mod loader; the ReSkate runtime itself is not bundled.

Required ReSkate.dll SHA-256: `bb3a36fef26c0c779159a76b6a75669aeb8797a3192a2ab11c68b7b099c87969`. Runtime support is pinned to this exact official release.

Release build and all 51 automated tests pass; detailed results are recorded in docs/CTEST_1.36.13.txt. The offline audit verifies 33 native function entries, 68 compiled layout fields/sizes, globals and call relationships; separate game code/vtable, collision and emote checks also pass. Live gameplay testing of the 1.36.13 input patch is pending. The validation checklist covers additional hardware-specific checks.

See [audit details](docs/MIGRATION_20261116.md) and [installation/testing](docs/VALIDATION_20261116.md). Historical documents retain their original version pins.

Build on Windows x64 using Visual Studio 2026 C++ tools: ./build.ps1, then ./package.ps1. The offline tools use the pinned dependencies in tools/requirements.txt and the official tagged source. Compile tools/layout20261116 against that source, then run tools/audit_20261116.py with the DLL and --upstream source folder. The historical 2.1.4-5 DLL at ../upstream/ReSkate-2.1.4-5.dll is used for the independent mutex implementation comparison.
