# ReSkate 2.1.1 compatibility-test validation

Package: JishsFirstPersonMod-1.34.0-compat-test2-reskate-2.1.1.

All 38 automated tests passed, including the explicit old-DLL rejection test. The full result is recorded in CTEST_211.txt. The independent native audit is NATIVE_AUDIT_211.json and the release-header layout output is ABI_LAYOUT_211.txt. The suite retains the working camera, bail/head-attachment, look-mode, smoothing, input, binding-recording, data-preservation and renderer mocks. The dedicated compatibility test additionally rejects the previous official 2.1.0 DLL, including forced resolution, and checks native entry tampering and partial-model revision handling.

All three release-header layout probes passed against pinned 2.1.1 source. The 17-entry binary audit and direct code/data relationship checks passed. The installed game hash matches the unchanged supported game identity. Camera algorithms, offsets and look-mode behavior are retained; native integration contracts were updated.

## In-game validation

Pending for this package. The user confirmed the 2.1.0 baseline works; this is not a live 2.1.1 result. Check menu opening with Insert/F9, stock windowed/fullscreen UI, binding recording and release, keyboard/text input, focus/resize, XInput and Sony USB/HID paths as applicable. Test camera-only and camera-and-character modes through repeated bails/get-up, mounting/dismounting, return-to-marker, jumps, flips and spectator/freecam/drone/replay transitions.

No MSC [FP] implementation is present. No capability requests, presence commands or heartbeats should run. The game installation and server are untouched. This package is local and unpublished. Reapply your preferred preset after updating; preserve existing saved data.

## Toggle emote regression

This is compat-test2. The user confirmed compat-test1 works in-game, then reported native LB+RB+D-pad toggles triggering emotes. See TOGGLE_EMOTE_TEST2.md for the fix and expanded regression coverage. The current fix still needs in-game confirmation. Existing native ABI contracts are unchanged.
