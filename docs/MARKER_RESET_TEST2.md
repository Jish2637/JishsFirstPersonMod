# Return-to-marker reset — 1.36.1 marker-reset test2

## Why test1 did not resolve the report

The installed test1 log records return detection repeatedly without the fresh-native-view completion message. Test1 cleared the mod look state, but ReSkate's upstream code can retain its previous animation capture and its own Spring/stabilization history. A reset also needed to survive verified local rig replacement within the same world during a known return episode. A plain incoming camera transform was not sufficient evidence of a fresh head sample.

## Revised implementation

- Use the already verified native sampler hook to reset the native Spring/stabilization history through temporary per-call settings. No native Spring layout guesses, direct history-memory writes, new hook addresses, settings changes or activation toggles.
- Keep the temporary reset for every tick/render sample through the completion frame, because native FrameSpring render samples share the earlier tick's frame_start.
- Wait past the first native frame after teleport; validate the sampler head against the current animation orientation/root and live body position before resuming stick aim.
- The final transform callback can refresh an already accepted heading but cannot certify its own incoming old camera transform as fresh.
- Keep a pending return across verified local rig replacement in the same world for up to five seconds; reject previous requests and rebind only to the verified local owner. World changes still cancel the episode.
- Rebuild the base view even when remember-glance is enabled. Only an actually selected button glance is restored as a relative offset, when its existing memory option permits it. No previous absolute camera transform is restored.
- Preserve configured offsets, both look modes, neutral-stick quarantine, normal camera algorithms, emote protections and updater behavior.

## Automated validation

Final Release build passed **45/45 tests**. This includes all 43 released-baseline tests, marker-reset state/math cases, and a new test compiled against an exact test-only copy of the pinned official ReSkate 2.1.1 native camera math. The native test reproduces retained heading at an unchanged position with Spring/stabilization enabled, checks fresh reset output and same-frame tick/render behavior, restores saved settings on the next frame, and checks inactive/active relative glance behavior. The marker test includes pending reset across verified rig replacement.

The official native contract audit passed. No native addresses or ABI layouts changed. The test-only official header is byte-identical to upstream commit db99ed4d724dd77edd2abf62f3de0dbc5bb6717e.

## In-game acceptance pending

Close Skate before extracting the client ZIP beside Skate.exe, merging Mods and scripts. Settings and presets are retained.

1. With no active glance, turn away from a marker and return: camera should reset to the head-facing view with remember-glance both off and on.
2. Repeat in Camera only and Camera and character, on foot and on board, at a distant marker and at the same position with a different heading.
3. With an active toggled glance, verify the memory option restores only that direction relative to the new marker heading. With no glance active, it must never retain the previous facing.
4. Hold the stick through return, then centre it; fresh look must start from the current reset view.
5. Repeat after bails/get-up, mounting, rapid returns, smoothing changes and stock camera transitions. Recheck toggle/glance emote protection.

Look for the completion log: `Return to marker: native spring and stabilization rebuilt from current head; previous camera transform discarded.`

These are automated results, not a claim that live gameplay has passed. Game installation untouched during preparation; nothing published.
