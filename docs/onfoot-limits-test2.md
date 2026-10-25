# On-foot limits and gesture blocking test 2

The previous live log shows core and Insert startup succeeding, followed by optional emote hook attachment failure, MSC attachment failure, and input-filter attachment failure (Detours attach target / Win32 8). Input setup returned in 156 ms, so that session's missing gesture blocking was not caused by slow polling.

A failed optional queued hook remained queued in the shared hook backend. Later optional transactions retried that same failed target, potentially blocking all controller adapters. Experimental emote aiming and MSC presence now enable their own targets individually, with the existing bounded thread-access retry policy. Their failures no longer queue work into gesture-filter installation. A regression exercises optional attachment failure followed by an unrelated queued transaction and confirms no unexpected hook activation.

This retains the verified input routes rather than introducing unverified native gesture writes. In-game gesture blocking still needs confirmation with both mods loaded. If an input hook independently fails, the log remains the authority; the mod does not claim filters are active after failure.

Optional on-foot right-stick limits are off by default. Separate up/down/left/right angles clamp both integrated targets and smoothed view relative to skater forward. No accumulated overshoot while holding against a stop. Bail/recovery and spectator view limits are unchanged, as are button-glance angles. Persistence, missing-key defaults, presets and rejection of invalid values are covered by regression tests.

The user confirmed pointing aims with the first-person camera when stationary, but running can make pointing face forward until stopping. No explicit speed gate was found in the experimental adapter. Native animation consumption during locomotion remains unverified; no speculative gesture or animation memory writes were added. The working emote implementation is preserved.

Test procedure: keep the existing universal loader, close Skate, merge this client ZIP into the game folder, enable the mod in the launcher, then restart. Check normal single-tap gesture suppression/double-tap emotes, toggle chord suppression, and modifier capture. Check asymmetric camera limits, changing limits while looking, turning the skater, smoothing, full/compatibility mode, and bail/respawn transitions.

## Validation

Release build and all 50 regression/integration tests passed, with no skipped tests. Added coverage includes asymmetric limits, reversing off a hard stop, skater heading across the wrap boundary, zero limits with smoothing, disabling limits, settings/preset round trips, missing-key default, invalid values and isolation of a failed optional hook from a later queued transaction.

Test DLL SHA256: `186c9bff94871fbe639e71d7ca95cebe94d4c25505be6d443591f58006b778f6`.

This is a local 1.36.4 test distribution, not a GitHub release. Neither the game installation nor GitHub main was changed. In-game gesture-blocking recovery and camera feel remain to be confirmed.
