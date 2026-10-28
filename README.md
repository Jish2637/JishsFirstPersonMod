# Jish's First Person Mod 1.36.7 - ReSkate 2.1.3

First-person camera controls, native head movement, bail camera collision handling, MSC [FP] presence and first-person throwdown spectator freelook for the reviewed ReSkate 2.1.3 build.

## What's new

Updates the published 1.36.6 mod for the exact official ReSkate 2.1.3 DLL. All private function addresses, mutex calls and globals have been re-derived. The overlay context, callbacks, model and menu moved by 1,024 bytes. DS4 Bluetooth now accepts Windows-padded 547-byte reads and refreshes the CRC at byte 74 while preserving padding. See [the audit](docs/MIGRATION_213.md) and [testing instructions](docs/TEST_213.md).

## Installation

Close Skate, back up your existing dinput8.dll, and extract JishsFirstPersonMod-1.36.7-reskate-2.1.3.zip beside Skate.exe, merging Mods and scripts. Install the whole ZIP, enable the mod in the launcher and restart. Open Insert > Skater > First Person.

Your personal settings, presets, other mods and scripts/Custom/dinput8_mods.txt are preserved. The bundled loader discovers DLLs in scripts/Custom and respects their order and disabled entries. See [installation details](LAUNCHER_INSTALL.md).

## Updates

Install this release manually if an older updater rejects your loader or runtime. Version 1.36.7 recognizes the bundled multi-mod loader and the verified original UniversalModLoader v2. Future First Person updates preserve these loaders and your other mods. Unknown loader builds still require manual installation.

Use Mod updates > Check for updates, then Download update and Install after I close Skate. Close Skate normally, wait for completion and restart yourself. Only newer published releases are offered.

## New camera options

Advanced > On-foot right stick look contains Limit on-foot camera angles (off by default), with separate up/down/left/right limits. Native emote aiming is experimental and off by default. Stationary pointing follows the view; running can temporarily point forward until stopping.

## Build and validation

Windows x64 with Visual Studio 2026 C++ tools: run ./build.ps1, then ./package.ps1. To omit the bundled loader, both commands accept -UniversalLoader. External integration fixtures and driver-test options are documented in [startup validation](docs/startup-buildfix.md).

The user confirmed the 1.36.7 candidate working in game before approving publication. All 51 automated tests passed. Runtime support is restricted to ReSkate 2.1.3 SHA256 139e932ad6328b898b2ff8c2aac3e0cd5ced6545c2f041aa8534239f97e59f9b. The supported game executable and its addresses are unchanged and were audited separately. Historical tools and documents retain their original version pins.
