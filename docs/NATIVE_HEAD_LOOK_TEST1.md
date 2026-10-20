# Native head-look input test1 — ReSkate 2.1.1

This is the small input-passthrough experiment described in ReSkate_FirstPerson_Native_HeadLook_Test.md. It tests whether Skate animates the head when it receives physical right-stick input while the mod still drives its existing first-person camera. It does not redesign the camera or implement custom head tracking.

## Baseline and boundaries

Based on the exact 1.36.1 marker-reset-test2 source. That marker fix still requires its own in-game acceptance. Camera transforms, native sampler, head attachment, bone transforms, animation callbacks and multiplayer code are unchanged by this diagnostic.

The input policy returns an empty stick-capture policy only after the existing live owner, matching controller, foreground focus, menu, camera ownership and actual first-person-active/requested checks pass. It also requires off-board capture with an active aim, stick look enabled, no active character-turn request, no recovery look, and effective camera-only mode. Camera-only means compatibility/look_only is enabled OR stick_turn_character is disabled. A selected character-turning mode does not qualify merely because turning is temporarily unavailable during an action.

On-board policy is unchanged. The filter implementations are unchanged, so XInput and Sony/HID/DirectInput receive the same policy. In the test case no replacement axes are supplied: the original physical values remain untouched. Existing button/glance/emote filtering and stock menu capture still run.

## Switch

Installed file: `scripts/Custom/FirstPerson/native-head-look-test.ini`.

```ini
[NativeHeadLookTest]
enabled=0
```

Default OFF, including when the file is missing. Set `enabled=1` to enable the experiment, and `enabled=0` to restore normal stick filtering. The existing controller tick polls the separate file every 500 ms; no restart is required for changing the switch. Main settings, personal presets and menu UI are not modified. After refocusing the game, centre the stick before looking as usual.

## Installation and in-game test

Close Skate. Extract JishsFirstPersonMod-1.36.1-native-head-look-test1-reskate-2.1.1.zip beside Skate.exe, merging Mods and scripts. The new switch is supplied OFF. Keep another player observing your head.

1. With the switch OFF, check third-person controls and off-board first-person Camera only look. Record whether the observer sees your head turn.
2. Set the switch ON. Refocus Skate, centre the right stick, then look left/right and up/down while off-board in first person.
3. Ask the observer whether your head now moves. Note any double camera movement, character turning or conflicting native controls. These are possible diagnostic results, not behavior to compensate for in this test.
4. Select Camera and character, then get on the skateboard. Check that their input behavior remains the baseline behavior; the diagnostic cannot activate there.
5. Set the switch OFF and check that baseline behavior returns.

If the observer sees head movement, report it before any further changes. If the head remains still, disable the switch and report that result. No custom bone manipulation or networking will be added automatically.

## Exact source changes

- `src/stick_runtime.cpp`: new default-off atomic flag and `poll_native_head_look_test()` called by `tick_stick_look()`; narrow passthrough branch in `stick_input_policy()` after its existing guards.
- `src/native_head_look_test.h`: small pure eligibility predicate for the diagnostic branch.
- CMake adds a regression executable and installs the separate default-off INI; build/package labels identify this diagnostic. No new menu control or general settings machinery.

Build and test results are added below after completion. Live head animation and remote visibility can only be confirmed in game with an observer. Nothing is published or installed into the working game during preparation.

## Automated results

- Release build and **46/46 tests passed**: all 45 baseline tests plus the scoped passthrough regression.
- Official ReSkate 2.1.1 native contract audit: PASS; no addresses or ABI layouts changed.
- Runtime source comparison: only stick_runtime.cpp and the small new native_head_look_test.h predicate differ from marker-reset-test2. All camera transforms, native camera interop, head attachment/math, animation/networking and filter implementation files are byte-identical.
- Eligibility tests cover default OFF, active camera-only look, compatibility semantics, neutral-wait, disabled look, selected/active character turning, recovery exclusion, physical-axis preservation, and unchanged ordinary/board policy.
- Installed switch is OFF by default. No UI, main settings or presets changed.
- Remote head movement, controller-specific live behavior and possible native control conflicts remain untested until the observer test.
- Game installation untouched and no GitHub publication.
