# Jish's First Person Mod 1.36.8 — ReSkate 2.1.4

Compatibility update based on the published 1.36.7 release. All private ReSkate addresses and globals have been re-derived for the official 2.1.4 DLL. This includes mutex thunks and their implementation, camera callbacks, controller/HID capture, embedded menu and ImGui allocation, spectator records and the MSC session bridge.

Five overlay fields moved by 8 bytes, and the remote-player stride grew from 888 to 896 bytes. Both are corrected. Existing camera settings and features are retained. The exact runtime identity is shared by startup, private bridges and the updater.

Close Skate and back up your current mod and dinput8.dll. Extract the complete release ZIP beside Skate.exe, merge Mods and scripts, enable the mod in the launcher and restart. Existing personal settings, presets, other mods and dinput8_mods.txt are retained. The ZIP contains the standard multi-mod loader; the ReSkate runtime itself is not bundled.

Required ReSkate.dll SHA-256: `39195d824d775e5046480d66cb62dae0d709648f145414cb952ed2aa8afb99b3`. Runtime support is pinned to this exact official release.

Release build and all 51 automated tests pass. The offline audit verifies 33 native function entries, 68 compiled layout fields/sizes, globals and call relationships; separate game code/vtable, collision and emote checks also pass. The user confirmed the candidate working in game on 3 November 2026 and approved this release. The release client files are identical to that tested candidate.

See [audit details](docs/MIGRATION_214.md) and [installation/testing](docs/VALIDATION_214.md). Historical documents retain their original version pins.

Build on Windows x64 using Visual Studio 2026 C++ tools: ./build.ps1, then ./package.ps1. The offline tools use the pinned dependencies in tools/requirements.txt and the official tagged source. Compile tools/layout214 against that source, then run tools/audit_214.py with the DLL and --upstream source folder. The historical 2.1.3 DLL at ../upstream/ReSkate-2.1.3.dll is used for the independent mutex implementation comparison.
