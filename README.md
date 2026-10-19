# Jish’s First Person Mod 1.35.0 — ReSkate 2.1.1

A DLL mod for the exact official ReSkate 2.1.1 runtime, with first-person camera controls, directional glances, on-foot and on-board right-stick look, head attachment, spectator options, bindings and personal INI presets.

## Installation and use

Close Skate. Extract `JishsFirstPersonMod-1.35.0-reskate-2.1.1.zip` beside Skate.exe and ReSkateLauncher.exe, merging Mods and scripts and replacing this mod’s existing files. Enable Jish’s First Person Mod in the launcher.

Open **Insert → Skater → First Person**. Stock ReSkate handles menu navigation using controller, mouse or keyboard. Explicit binding recording captures a raw controller chord until release. The separate panel and its F9 shortcut have been removed.

Existing settings and personal presets remain in `scripts/Custom/FirstPerson`. **Reapply your preferred preset after updating.** Select Jish’s Recommended and Load to use the updated recommended settings. Recommended disables on-foot right-stick smoothing and enables on-board right-stick look with LB; glance smoothing and on-board look smoothing 12 remain as configured in the tested Main preset.

Only official ReSkate 2.1.1 is supported by this package. ReSkate.dll is not included. MSC first-person nametag reporting is removed.

## Source and validation

Run `./build.ps1` with VS 18 2026 C++ tools on Windows x64 to build, run the 40 tests and stage locally. Run `./package.ps1` for client/source ZIPs. The install ZIP contains only dinput8.dll, scripts, Mods and LICENSE.txt. These scripts never install to the game or publish.

The 1.35.0 release changes version metadata and documentation from the user-tested native-menu build; runtime source and the Main-based Recommended preset are preserved. See [release notes](docs/RELEASE_1.35.0.md), [validation](docs/RELEASE_1.35.0_VALIDATION.md), and [provenance](docs/release-1.35.0-provenance.json). Other reports in docs are historical records for their named builds.

Source and build tools are available in this repository. GitHub releases attach only the client installation ZIP. The embedded DLL uses the stock ImGui frame/context, fonts, theme and verified widget helpers. A separate process-lifetime worker keeps existing guarded 50 ms camera/settings refresh independent of presentation. No companion graphics backend is started.
