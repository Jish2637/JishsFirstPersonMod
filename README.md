# Jish’s First Person Mod 1.36.3 — ReSkate 2.1.2

This checkout now targets the supplied ReSkate 2.1.2 DLL (SHA256 `96112d915c181b696b5cb8ed17b62a74d09ca2900c4383a4500c1fa88bf76e43`). Native camera, input, spectator, menu and MSC bridge addresses and layouts have been remapped. See [2.1.2 validation](docs/reskate-2.1.2b-validation.md). Earlier release packaging and provenance below describe the original 2.1.1 release.

## What’s new

- Improved bail camera rotation so the view follows the player’s head more consistently during falls.
- Improved bail camera collision handling to reduce sticking in walls, floors and objects after hard impacts.
- Added MSC server `[FP]` tag integration. The tag stays active whenever First Person is enabled, including temporary third-person bail views and spectating.
- Added modifier-held right-stick freelook while spectating in first person during throwdowns.
- Updated compatibility for the latest ReSkate 2.1.2 DLL.

**Validation:** Build and binary checks passed; all 48 regression tests passed. In-game verification remains outstanding.

## Update from the mod menu

Open **Insert → Skater → First Person → Mod updates**, choose **Check for updates**, then **Download update**.

When it’s ready, choose **Install after I close Skate**. Close Skate normally and wait for the completion message before starting ReSkate yourself.

## Manual installation

Close Skate. Extract `JishsFirstPersonMod-1.36.3-reskate-2.1.2.zip` into your ReSkate folder beside `Skate.exe` and `ReSkateLauncher.exe`, merging **Mods** and **scripts** and replacing this mod’s existing files. Enable **Jish’s First Person Mod** in the launcher.

Existing settings and personal presets are preserved. Older presets leave **Native head movement** off; enable it again and save your preset if you want to keep it.

This rebuilt DLL requires the **reviewed ReSkate 2.1.2 DLL**. This updates the mod, not ReSkate itself.

## Building from source

Build with `./build.ps1` on Windows x64 with Visual Studio C++ tools, then package with `./package.ps1`. The public and internal build version is 1.36.3. See [runtime validation](docs/reskate-2.1.2b-validation.md) and [release notes](docs/release-1.36.3.md).
