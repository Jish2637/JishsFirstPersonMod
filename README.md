# Jish’s First Person Mod 1.36.0 — ReSkate 2.1.1

## What’s new

- Added an updater inside **Insert → Skater → First Person → Mod updates**.
- New updates appear in **green**, making them easier to spot.
- The mod can check for updates when you start the game. You can turn this off and check manually whenever you like.
- You choose when to download and install. Your saved settings and personal presets are kept.

## How updates work

1. Open **Mod updates**. When an update is available, choose **Download update**.
2. Once the download is ready, choose **Install after I close Skate**.
3. Close Skate normally and wait for the update’s completion message.
4. Start ReSkate yourself when you’re ready to play again.

The updater does not close the game or restart it for you. You can choose **Later** to leave an available update for another time, or **Cancel update** before installation starts.

**Install this release manually once to get the updater. Future compatible mod updates can then be downloaded from the menu.** The updater updates this mod; it does not update ReSkate itself.

## Installation

Close Skate. Extract `JishsFirstPersonMod-1.36.0-reskate-2.1.1.zip` into your ReSkate folder beside `Skate.exe` and `ReSkateLauncher.exe`, merging **Mods** and **scripts** and replacing this mod’s existing files. Enable **Jish’s First Person Mod** in the launcher.

Open **Insert → Skater → First Person**.

Existing settings and personal presets are preserved. Requires **official ReSkate 2.1.1**.

## Building from source

On Windows x64 with Visual Studio C++ tools, run `./build.ps1`, then `./package.ps1`. The build runs the automated tests before staging the files. See [updater documentation](docs/UPDATER.md) for supported installations and behavior.
