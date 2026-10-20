# Jish’s First Person Mod 1.36.2 — ReSkate 2.1.1

## What’s new

- Added **Native head movement** under **Insert → Skater → First Person → Advanced → On-foot right stick look**.
- In **Camera only** mode, this lets your character’s head move as you look around, so other players can see it too.
- **This changes how the camera feels.** It starts off; enable it if you prefer the new feel. Turn it off to return to the previous behavior.
- The option saves with your settings and personal presets. It does not apply to **Camera and character** or on-board look.
- Updated return-to-marker camera reset handling.

## Update from the mod menu

Open **Insert → Skater → First Person → Mod updates**, choose **Check for updates**, then **Download update**.

When it’s ready, choose **Install after I close Skate**. Close Skate normally and wait for the completion message before starting ReSkate yourself.

## Manual installation

Close Skate. Extract `JishsFirstPersonMod-1.36.2-reskate-2.1.1.zip` into your ReSkate folder beside `Skate.exe` and `ReSkateLauncher.exe`, merging **Mods** and **scripts** and replacing this mod’s existing files. Enable **Jish’s First Person Mod** in the launcher.

Existing settings and personal presets are preserved. Older presets leave **Native head movement** off; enable it again and save your preset if you want to keep it.

Requires **official ReSkate 2.1.1**. This updates the mod, not ReSkate itself.

## Building from source

Build with `./build.ps1` on Windows x64 with Visual Studio C++ tools, then package with `./package.ps1`. The public version is 1.36.2. This release reuses the tested native-head-look-test2 binaries unchanged, so internal build labels retain that test name. See [release provenance](docs/release-1.36.2-provenance.json) and [validation](docs/release-1.36.2-validation.md).
