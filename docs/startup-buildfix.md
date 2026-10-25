# 1.36.4 startup and portable build fix

## Runtime

Core camera hooks and Insert settings initialize before optional controller probing. The observed older game log spent approximately 80 seconds probing DirectInput before installing the menu. XInput, Raw Input and native HID adapters now attach before slow DirectInput device discovery too. DirectInput coverage can arrive later; core startup stays active if optional setup fails.

An early-loading universal loader no longer permanently fails startup when ReSkate.dll has not loaded yet: the worker waits up to ten minutes outside DllMain. The supplied UniversalModLoader v2 already waits for overlay readiness, so this is additional compatibility, not a confirmed explanation for that loader's failure.

The module still validates the exact supported Skate.exe and ReSkate 2.1.2 binaries. No native addresses changed. The universal-loader ZIP excludes dinput8.dll; preserve your existing loader. Do not force the automatic updater over an unrecognized loader; use manual updates if its loader-ownership guard declines installation.

## Building and tests

Run `.\build.ps1 -UniversalLoader` with Visual Studio 2026 C++ tools. Staged output is in `release/`. The ordinary build still supports a standalone bundled loader.

Three integration tests require separately obtained external artifacts; the original source ZIP hard-coded paths to files outside the checkout. Missing artifacts are now reported explicitly as skipped, rather than failing with 'Cannot read update file' or 'fixture missing'. Synthetic updater and compatibility tests still run. Do not report skipped integration checks as passed.

Supply these CMake FILEPATH cache variables to run all external checks:

- `JISH_RUNTIME_FIXTURE`: verified ReSkate-2.1.2b.dll, SHA256 96112d915c181b696b5cb8ed17b62a74d09ca2900c4383a4500c1fa88bf76e43.
- `JISH_PREVIOUS_RUNTIME_FIXTURE`: historical ReSkate-2.1.0.dll for rejection testing.
- `JISH_CLIENT_FIXTURE`: published JishsFirstPersonMod-1.35.0-reskate-2.1.1.zip for updater layout/state tests.

The shared HID/controller regression previously mixed synthetic checks with real Windows DirectInput device discovery under a 15-second timeout. Real driver discovery can exceed that timeout. Deterministic adapter checks remain in the standard test; opt into the separate `windows_directinput_driver_discovery` test with `-DJISH_DRIVER_TESTS=ON` (180-second timeout). This does not remove DirectInput support from the mod.

After configuring, run `cmake --build build --config Release --parallel 1`, then `ctest --test-dir build -C Release --output-on-failure`.

## In-game check

Close Skate before replacing the DLL. Keep Mods/JishsFirstPersonMod enabled, and check the universal loader has not commented out the DLL. Confirm the startup log reaches 'Core initialized' and the Insert page responds while optional DirectInput probing is still running. Then check normal controller look, modifier capture, bail recovery, throwdown spectating and MSC presence. In-game success must be confirmed separately from automated test results.

## Validation result (25 October 2026)

- Release build and `build.ps1 -UniversalLoader` completed successfully.
- 50/50 tests passed with the three external fixtures and `JISH_DRIVER_TESTS=ON`; none skipped. This includes the real Windows DirectInput discovery, native HID/logging lock regression, updater state and published-client layout checks, exact 2.1.2 compatibility, and host rejection checks.
- Separate fixture-free configuration correctly marked the three external integration checks skipped (not failed).
- Current game ReSkate.dll hash matches the reviewed 2.1.2 identity. Existing universal-loader log confirms it loads JishsFirstPersonMod.dll alongside JishsVirtualTV.dll.
- Built DLL SHA256: `acf99bbd27dc55d60f399f212f312f242c54964653042f3f3c04a43c21d46780`.
- Game installation and universal loader were left unchanged. In-game Insert responsiveness and experimental emote aiming remain unverified for this build.
