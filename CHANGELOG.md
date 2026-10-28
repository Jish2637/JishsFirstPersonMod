# Changelog

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


## 1.36.4 — ReSkate 2.1.2 (startup ordering fix)

- Core camera hooks and the native Insert > Skater > First Person settings page now initialize before optional XInput, Raw Input, DirectInput and HID filters.
- Slow or failing optional controller setup no longer blocks successful core startup; detailed per-stage logging identifies driver enumeration stalls.
- No game or ReSkate hook RVAs/signatures were changed. The same verified 2.1.2 binaries are supported.
- In-game behaviour and Windows build/testing still need verification.

## 1.36.3 — ReSkate 2.1.2

- Improved bail camera rotation so the view follows the player’s head more consistently during falls.
- Improved bail camera collision handling to reduce sticking in walls, floors and objects after hard impacts.
- Added MSC server `[FP]` tag integration. The tag stays active whenever First Person is enabled, including temporary third-person bail views and spectating.
- Added modifier-held right-stick freelook while spectating in first person during throwdowns.
- Updated compatibility for the latest ReSkate 2.1.2 DLL.

**Validation:** Build and binary checks passed; all 48 regression tests passed. In-game verification remains outstanding.
