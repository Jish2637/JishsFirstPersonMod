# Jish's First Person Mod 1.36.5 - ReSkate 2.1.2

First-person camera controls, native head movement, bail camera collision handling, MSC [FP] presence and first-person throwdown spectator freelook for the reviewed ReSkate 2.1.2 build.

## What's new

This release bundles a multi-mod dinput8 loader, restores updater compatibility with recognized multi-mod loaders, improves Insert-menu startup and isolates optional hook failures from gesture blocking. It adds optional on-foot camera angle limits and experimental native emote aiming. See [release notes](docs/release-1.36.5.md) and [settings](SETTINGS.md).

## Installation

Close Skate, back up your existing dinput8.dll, and extract JishsFirstPersonMod-1.36.5-reskate-2.1.2.zip beside Skate.exe, merging Mods and scripts. Install the whole ZIP, enable the mod in the launcher and restart. Open Insert > Skater > First Person.

Your personal settings, presets, other mods and scripts/Custom/dinput8_mods.txt are preserved. The bundled loader discovers DLLs in scripts/Custom and respects their order and disabled entries. See [installation details](LAUNCHER_INSTALL.md).

## Updates

Install this release manually if an older updater rejects your loader or runtime. Version 1.36.5 recognizes the bundled multi-mod loader and the verified original UniversalModLoader v2. Future First Person updates preserve these loaders and your other mods. Unknown loader builds still require manual installation.

Use Mod updates > Check for updates, then Download update and Install after I close Skate. Close Skate normally, wait for completion and restart yourself. Only newer published releases are offered.

## New camera options

Advanced > On-foot right stick look contains Limit on-foot camera angles (off by default), with separate up/down/left/right limits. Native emote aiming is experimental and off by default. Stationary pointing follows the view; running can temporarily point forward until stopping.

## Build and validation

Windows x64 with Visual Studio 2026 C++ tools: run ./build.ps1, then ./package.ps1. To omit the bundled loader, both commands accept -UniversalLoader. External integration fixtures and driver-test options are documented in [startup validation](docs/startup-buildfix.md).

Release build and 51 automated tests pass. The user confirmed the multi-mod build in game; live install-after-exit updating remains unverified. Runtime support remains restricted to ReSkate 2.1.2 SHA256 96112d915c181b696b5cb8ed17b62a74d09ca2900c4383a4500c1fa88bf76e43. No game addresses changed in this release.
