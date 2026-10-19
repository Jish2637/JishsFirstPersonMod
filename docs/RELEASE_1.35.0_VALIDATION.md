# Release 1.35.0 — validation and provenance

## Result

Release metadata rebuild: **40/40 tests passed, zero failures**. Original 2.1.1 binary audit, additional embedded page/widget/allocator audit and Release layout probes all passed. Shipping PE has the 1.35.0 version log and no graphics discovery/window creation imports.

The 249 runtime, vendored and test files are byte-identical to the user-tested embedded-menu source archive. Recommended is an exact copy of the tested Main preset (SHA-256 5685eda8e331a2a1801125408d4c50081042cfe49fc57a5d2ed454d0c566e6c9). Changes from that tested build are version metadata, launcher description, current instructions, release documentation and build-directory ignore rules. No new runtime behavior is introduced.

## Automated scope

The suite includes the existing 38 tests plus embedded page/settings/preset routing, scope and allocator restoration, actual hook/worker simulation, closed-Insert camera refresh, contract rejection retaining the stock page, recording isolation/release quarantine, exception recovery and F9 non-interference. The legacy companion test fixtures remain test-only; the shipping DLL does not link their renderer/input owner. Existing camera, head attachment, both look modes and toggle/glance emote regressions pass.

The binary audits independently verify the exact official DLL and reviewed private entry points, page dispatch, helper call relationships, host allocator/current-context slots and matching ImGui source/configuration. The release-layout probe compiles against official pinned headers and asserts ABI sizes, offsets and request enum IDs. See release-1.35.0-baseline-audit.json, release-1.35.0-ui-audit.json, release-1.35.0-layout.txt and release-1.35.0-shipping-audit.json.

## User-confirmed testing

The user confirmed the embedded-menu build works well in game. Compat-test2 camera/input behavior and the earlier emote fix were also user-confirmed. The 1.35.0 metadata rebuild has not been separately tested in game. No claim is made that every window/fullscreen, controller/Steam Input, resizing/focus or camera/spectator transition combination received live testing. The detailed acceptance matrix remains in EMBEDDED_MENU_TEST1.md.

## Packages and publication

Client roots: dinput8.dll, scripts, Mods and LICENSE.txt only. ReSkate.dll, saved settings.ini, personal Main.ini, logs, build outputs and source are not installed by the client ZIP. The source ZIP includes matching source and build tools. Existing user data stays under scripts/Custom/FirstPerson. Reapply the preferred preset after updating; load Jish’s Recommended for the new defaults.

Source baseline on GitHub: c0d2c4788fcbc1f970d4b3dc013a70f46b43bd6f (1.34.0, ReSkate 2.0.7). The earlier 2.1.0 release uploaded a compatibility client asset without updating that source. The supplied patch represents the complete source update from that GitHub baseline.

Tag planned: 1.35.0-reskate-2.1.1. Source and packages are prepared locally, awaiting final wording approval before any GitHub push, tag or release upload. The game installation and earlier working source/package trees remain unchanged by release preparation.

## Build

Run ./build.ps1, then ./package.ps1 on Windows x64 with VS18 2026 C++ tools. Audit scripts require Python pefile/capstone and the pinned official DLL/source; embedded audit also takes the compat-test2 source archive as its baseline. Configure tools/layout211 with -DUPSTREAM=<official pinned source>, build Release, and run layout211.exe. Full build/test output and hashes are included in this directory.
