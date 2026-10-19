# Embedded Insert menu â€” 1.34.0-native-menu-test1

## Status

Local Windows x64 test client for the exact official ReSkate 2.1.1 runtime. No release published, no game installation changed. In-game acceptance of this UI migration is pending. The user confirmed compat-test2 as the working gameplay baseline; that confirmation is not a test result for this package.

## Recommended preset update

The user confirmed this embedded UI build works in game. The bundled recommended.ini now exactly matches their Main.ini: on-foot stick smoothing off, on-board look enabled with LB modifier, glance toggle/return-to-marker memory enabled, first-person bails with third-person recovery. Main retains glance smoothing and on-board look smoothing 12. This changes only the recommended preset, not saved settings or the DLL. All 40 tests pass after updating the preset fixture. The installed recommended preset was also replaced after backing up the previous one in Release/recommended-before-main-preset.ini. The broader acceptance matrix below remains unconfirmed unless individually tested.

## Changes and preservation

The stock First Person page draw function is replaced at Insert â†’ Skater â†’ First Person. It draws within the stock frame using the stock context, fonts, card/toggle/field/slider helpers and theme. The native fork provided the visual grouping reference only. First Person and its recorder, INI preset selection/load/save/overwrite/delete/refresh, Advanced groups, all existing mod options and feature-specific binding recorders are included. Existing stock stabilization/head movement/follow-flips/board-only/camera-cycle controls remain accessible through the original native debug queue, including the existing comfort override interception.

All 199 baseline src/vendor files are byte-identical to the confirmed source archive. This includes camera math/offsets, look modes, smoothing, head attachment, body flips, spectator behavior, activation, toggle/glance emote filters, physical recording readers, settings, migrations, presets and storage. Changes live in new embedded integration files plus build, probe, manifest and documentation files. MSC remains absent. No settings.ini is supplied and personal presets are not overwritten.

The shipping DLL does not link the companion renderer or its input/UI implementation. No separate window, DX12 discovery/submission hook, backend, frame lifecycle or graphics resources are created. Font resources for the companion are not linked into the DLL. F9 is not handled by the mod. Stock owns Insert, navigation, scrolling, typing, menu lifecycle and input; raw binding recording temporarily consumes controller input, retaining the existing release quarantine. Page exit/focus loss invalidates pending recording. The DLL is pinned by the existing bootstrap; a process-lifetime worker invokes the existing guarded poll every 50 ms independently of presentation.

## Binary and ABI verification

Pinned official source: tag v2.1.1, commit db99ed4d724dd77edd2abf62f3de0dbc5bb6717e. Official DLL SHA-256 d79ff144768ce8b0a8e992c9f134c670a3ff85bb3b437a0eb915876973014301. The build is dedicated to that official binary. It does not bundle ReSkate.dll.

The independent audit verifies 11 additional function entry prefixes, First Person string references, the actual Skater tab dispatch and page/widget call graph. Allocator pointers/user data are derived from the official MemAlloc/MemFree instructions; the current context slot is independently shared by those functions and GetCurrentContext. Addresses were not inferred from a uniform shift. Runtime entry checks precede installing the page hook. Invalid UI context/allocator/frame contracts retain the original page and log failure; there is no companion fallback.

| Native boundary | Verified RVA / layout |
| --- | --- |
| First Person draw | 0xE537B0 |
| Skater page dispatcher | 0xE54F80 |
| begin/end card | 0xE0B7D0 / 0xE10970 |
| field / note | 0xE10A60 / 0xE11510 |
| float slider / toggle row | 0xE12DE0 / 0xE13700 |
| GetCurrentContext / context slot | 0xFCC5B0 / 0x18FCC38 |
| MemAlloc / MemFree | 0xFD12F0 / 0xFD13A0 |
| Alloc/free callback slots | 0x1771180 / 0x1771188 |
| Allocator user data slot | 0x18FCC40 |
| State context / menu / HWND | 1448 / 27664 / 704 bytes |
| SkateMenu page / skater_tab | 0 / 1884 bytes |
| ImGuiContext / ImGuiIO | 10576 / 3032 bytes |
| Draw vertex / index | 20 / 2 bytes |
| Debug request / queue callback | 12 bytes; bool 4, float 8 / callback offset 24 |

Release-layout probes compile against the pinned official headers with the release ABI and assert these sizes, offsets and native action IDs. Seven ImGui core/config files match official source after newline normalization, version 1.91.9b. The DLL scopes its ImGui current context and host allocators and restores its own globals on normal and exceptional exits. Host objects are borrowed opaque pointers; no stock object is constructed, enlarged or copied, and no STL ownership is transferred.

## Automated results

**40/40 CTest tests passed:** all 38 original tests plus two embedded integration regressions. The legacy presentation/input tests are retained as baseline fixtures; those implementations are not linked into the shipping DLL.

- Embedded controls: existing command routing, real settings/preset save/list/load/delete paths, advanced controls, balanced UI scopes, no pending popup window-size leak, host-frame continuity and context/allocator restoration (including exceptions).
- Actual embedded hook and worker with a simulated host: closed-menu polling, disabled integration inactivity, focus/page recording cancellation, context-contract rejection retaining stock, no duplicate page drawing, F9 non-interference, exception scope recovery and following-frame stock fallback.
- Existing physical reader tests: raw recording bypass, capture and held-button release quarantine. Existing camera/input suite includes both look modes, head attachment, bails/transitions, controller adapters and confirmed toggle/glance emote protection.
- Independent original 2.1.1 contract audit: PASS. Additional embedded page/widget/allocator audit: PASS. Release-layout probe compile/static assertions: PASS. Shipping PE: no graphics-discovery/window-creation imports and no legacy discovery-window string.
- Exact comparison: all original src/vendor files unchanged. Every original UI CommandKind remains represented in the embedded implementation. Protocol, authentication, networking and saved-data behavior are unchanged.

These are automated/mocked/offline results, not a running-game graphics or controller compatibility claim. Detailed output is in embedded-build-and-tests.txt, embedded-layout211.txt, embedded-baseline-audit.json, embedded-ui-audit.json and embedded-shipping-audit.json. Exact hashes are in embedded-provenance.json.

## In-game acceptance still required

1. Official 2.1.1 windowed and fullscreen menu: Insert â†’ Skater â†’ First Person; all groups, help, resets and native controls; stock controller, mouse and keyboard navigation; F9 does not affect the mod.
2. Personal INI presets: refresh external additions, save name entry, load, overwrite/delete confirmation, reopening the menu and restart persistence.
3. Recording on XInput and Sony USB/BT with relevant Steam Input configurations: release-first capture, cancel, timeout, controller replacement, page changes, closing Insert and focus loss; stock navigation resumes after held buttons release.
4. Resize/focus changes, map changes, chat/text entry and controller navigation without operating gameplay or a second interface.
5. Both camera look modes, repeated bails/get-up, mount/dismount, return to marker, jumps/body flips, spectator/freecam/drone/replay transitions; compare offsets/head attachment to compat-test2. Check First Person toggle and glance emote protection.

## Build and audit

Run `./build.ps1` in this source directory (VS 18 2026 C++ tools, Windows x64). This builds, runs CTest and stages locally. Run `./package.ps1` to create the matching client and source ZIPs. The client archive roots are only dinput8.dll, scripts, Mods and LICENSE.txt; existing settings are not packaged.

For independent audits, install pefile and capstone into a Python environment, then run tools/audit_211.py and tools/audit_embedded211.py with --dll pointing at the pinned official DLL and --upstream at the pinned official source. The embedded audit also takes --baseline pointing at the confirmed compat-test2 source ZIP. Configure tools/layout211 using CMake with -DUPSTREAM=<pinned source>, build Release and run layout211.exe. These tools read reference files and never load the official DLL or modify game/profile data.

The delivered patch applies relative to the compat-test2 source ZIP. Working source: local-reskate-2.1.1-native-menu-source. The original local-reskate-2.1.1-source and confirmed packages are untouched.
