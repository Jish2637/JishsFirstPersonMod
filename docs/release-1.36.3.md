# Jish’s First Person Mod 1.36.3 — ReSkate 2.1.2

- Improved bail camera rotation so the view follows the player’s head more consistently during falls.
- Improved bail camera collision handling to reduce sticking in walls, floors and objects after hard impacts.
- Added MSC server `[FP]` tag integration. The tag stays active whenever First Person is enabled, including temporary third-person bail views and spectating.
- Added modifier-held right-stick freelook while spectating in first person during throwdowns.
- Updated compatibility for the latest ReSkate 2.1.2 DLL.

**Validation:** Build and binary checks passed; all 48 regression tests passed. In-game verification remains outstanding.

## Installation

Close Skate, then extract `JishsFirstPersonMod-1.36.3-reskate-2.1.2.zip` into your ReSkate folder beside `Skate.exe` and `ReSkateLauncher.exe`, merging Mods and scripts. Enable Jish’s First Person Mod in the launcher. Existing settings and personal presets are preserved.

Requires the reviewed latest ReSkate 2.1.2 DLL, SHA256 `96112d915c181b696b5cb8ed17b62a74d09ca2900c4383a4500c1fa88bf76e43`. The ZIP updates this mod; it does not include ReSkate itself. Earlier 2.1.2 DLLs with a different hash require the updated runtime.
