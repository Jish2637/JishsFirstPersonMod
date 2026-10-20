# Return-to-marker heading reset — local test1

## Source finding

The on-foot look state stores an absolute world yaw. Resetting StickLookState alone can reseed it from the cached displayed view (note_camera_view), retaining pre-teleport yaw. The previous handoff recenter applies only to Camera and character after board/bail; it does not implement a marker episode for both modes.

## Change

An owner-bound marker generation observes the validated entity teleport flag from controller, camera-transform and character-motion paths, plus a >5 m body-position discontinuity fallback. Teleport invalidates aim requests and the cached displayed yaw/pitch. A generation change resets mod camera blends; the first valid native post-teleport pose seeds fresh look. Both modes wait for neutral input; character facing starts from the current body direction. Native pose sampling keeps updating while neutral is pending. Glance memory remains a separate relative direction option. World/player changes, disabling first person and stock camera takeover invalidate pending state. No native addresses, settings formats or multiplayer changes.

## In-game acceptance still required

- Camera only and Camera and character: look away from your marker, return, check immediate facing before moving the stick. Repeat with same-position/different-heading markers and distant markers.
- Hold the stick across return, wait, then centre it; camera must already face correctly and resume from the current view.
- Repeat with smoothing enabled/disabled, on foot/on board, after bail/get-up, and rapidly repeated returns.
- Glance memory off clears the old glance; on restores only its selected direction relative to the new marker heading.
- Check mounting/dismounting, body flips, freecam/spectator/replay restoration, and toggle/glance emote suppression.

This is a local test, not a published release. Existing game files remain untouched. Automated results are recorded separately after the build.

## Automated validation

- Final Release build: PASS.
- All 43 baseline tests plus the marker-reset regression executable: **44/44 passed**.
- Regression cases cover reset generations, callback ordering, same-position flag events, positional fallback, both look modes, smoothing, stale requests, fresh yaw/pitch/body bias, held-stick neutrality, repeated returns, owner replacement, board modifiers and optional button-glance memory. These are automated policy/math checks, not live gameplay.
- Independent official 2.1.1 native contract audit: PASS. All vendored ABI declarations are byte-identical to released 1.36.0.
- Head attachment, base camera maths, bail/body-flip algorithms, emote filtering and updater implementation unchanged.
- Runtime reset timing and visible alignment still require the in-game tests above.
