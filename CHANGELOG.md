# Changelog

## 1.36.11 - ReSkate 2.1.4-5

- Updated the exact runtime hash used by the mod and updater for ReSkate 2.1.4-5.
- Verified all mutex contracts, native addresses and layout offsets against the official DLL and tagged source. Every value matches 2.1.4-4.

## 1.36.10 - ReSkate 2.1.4-4

- Re-derived and guarded all 33 native function entries and runtime globals for the exact official DLL.
- Updated overlay menu/bindings/revisions, spectator remote stride, multiplayer session mutex/view/fields and peer stride from a compiled source layout probe.
- Verified both 80-byte MSVC mutex representations, overlay mutex offset 0 and remote mutex offset 72.
- Updated the rewritten native First Person page and shared widget addresses using page strings and the native call graph.
- Updated startup and updater runtime identity; previous 2.1.4-2 and unknown images are refused.
- Gameplay validation is pending.

## 1.36.9 - ReSkate 2.1.4-2

- Re-derived every private native function/global address, including mutex thunks and implementation, camera callbacks, embedded UI/ImGui, controller/HID, spectator and MSC bridges.
- Corrected debug revision offset 26128 and menu offset 28968 for the expanded model.
- Corrected multiplayer Session mutex 783624, view 781368 and connected-field offset 826.
- Updated startup and updater runtime identity to the exact official 2.1.4-2 DLL. Previous 2.1.4 and unreviewed images are refused.
- Release build and all 51 automated tests pass. Publication authorized on 4 November 2026.


## 1.36.8 - ReSkate 2.1.4

- Re-derived all private function/global addresses, including native mutex lock/unlock and the underlying lock implementation.
- Corrected five overlay offsets, each moved by 8 bytes, and the spectator Remote stride, now 896 bytes.
- Updated controller/HID entry points, all three HID callers, embedded menu/ImGui and spectator/MSC bridges.
- Updated exact runtime identity checks in startup and the updater; 2.1.3 and unreviewed DLLs are refused.
- Updated the PlayStation report helper from the pinned 2.1.4 tag.
- Release build and all 51 tests pass; 33 native entry guards and 68 source layout fields/sizes audited. The user confirmed the candidate working in game before publication.


## 1.36.7 - ReSkate 2.1.3

- Re-derived all private ReSkate function/global addresses, mutex calls, camera callbacks, controller/HID entry points, embedded UI allocation and spectator/MSC bridges against the official v2.1.3 DLL and tagged source.
- Updated overlay context, callback, model and menu offsets, each moved by 1,024 bytes.
- Updated the exact runtime hash used by initialization and the updater; older runtimes are refused.
- Added DS4 Bluetooth support for padded 547-byte reads, with the CRC still at byte 74 and padding preserved.
- Added an offline audit covering 32 function prefixes, globals/call relationships and 65 MSVC layout fields, plus runtime mutex/camera-reference tamper tests and a padded DS4 input regression test.

The user confirmed the candidate working in game before approving publication.

## 1.36.6 - ReSkate 2.1.2

- Fixed a crash when recording controller buttons for custom bindings. Confirmed working in game.

## 1.36.5 - ReSkate 2.1.2

- Added a bundled multi-mod dinput8 loader based on UniversalModLoader v2. It discovers DLL mods in scripts/Custom and preserves load order, comments and disabled entries in dinput8_mods.txt.
- Updated First Person's updater to recognize the bundled multi-mod loader and the verified original UniversalModLoader v2. Updates preserve these loaders, their mod lists, chained proxies and other mods.
- Fixed the installer checking an outdated ReSkate runtime hash. Downloader and installer now use the same verified ReSkate 2.1.2 identity.
- Improved startup so camera hooks and Insert settings initialize before slow optional controller probing. XInput, Raw Input and HID filters can attach before DirectInput discovery finishes.
- Fixed failed optional emote/MSC hook attachments interfering with later gesture-blocking setup.
- Added optional on-foot right-stick camera limits with separate up, down, left and right angles. Off by default; available under Advanced > On-foot right stick look. Limits persist in settings and presets.
- Added experimental native emote aiming in Camera only mode. Pointing follows the first-person view while stationary; running can temporarily return pointing to forward until stopping.

Existing bail camera, collision improvements, MSC [FP] tags, native head movement and throwdown spectator freelook are retained.

Validation: release build and 51 automated regression/integration tests pass. The user confirmed the multi-mod build works in game. Actual install-after-exit auto-updating has not been exercised against the live game installation.


## 1.36.4 â€” ReSkate 2.1.2 (startup ordering fix)

- Core camera hooks and the native Insert > Skater > First Person settings page now initialize before optional XInput, Raw Input, DirectInput and HID filters.
- Slow or failing optional controller setup no longer blocks successful core startup; detailed per-stage logging identifies driver enumeration stalls.
- No game or ReSkate hook RVAs/signatures were changed. The same verified 2.1.2 binaries are supported.
- In-game behaviour and Windows build/testing still need verification.

## 1.36.3 â€” ReSkate 2.1.2

- Improved bail camera rotation so the view follows the playerâ€™s head more consistently during falls.
- Improved bail camera collision handling to reduce sticking in walls, floors and objects after hard impacts.
- Added MSC server `[FP]` tag integration. The tag stays active whenever First Person is enabled, including temporary third-person bail views and spectating.
- Added modifier-held right-stick freelook while spectating in first person during throwdowns.
- Updated compatibility for the latest ReSkate 2.1.2 DLL.

**Validation:** Build and binary checks passed; all 48 regression tests passed. In-game verification remains outstanding.
