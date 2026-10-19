# Updater test1 — ReSkate 2.1.1

## What was found

The handover ZIP contained an experimental updater that was not included in CMake, bootstrap or the UI. There was no failed-build log in the archive. The untouched baseline built successfully with Windows/MSVC here and passed all 40 tests, so the earlier compiler failure was not reproduced. Its README reports that a Windows/MSVC environment was unavailable for the earlier attempt.

The prototype was not usable as an updater: it hard-coded the installed version, treated any different version as an update, did not reject draft/prerelease releases, did not hash or save downloads, did not validate/install ZIP contents, and did not actually validate redirected hosts. Its signed-asset query handling was also incomplete. It was left as an uncompiled historical prototype; the integrated implementation is under src.

## Implemented test behavior

- Installed version is read from Mods/JishsFirstPersonMod/manifest.json; it is not taken from a hard-coded equality check. Stable three-part numeric versions compare numerically, and equal/older releases are not offered.
- One asynchronous automatic check per launch (can be disabled), a manual Check for updates, Download update, Later, progress/cancellation, retained downloads and Install after I close Skate controls live in the existing First Person page under Mod updates. No extra renderer, controller navigation owner or camera changes.
- GitHub's exact compatible client asset is selected from latest stable release metadata. TLS verification stays enabled, redirect hosts are restricted and bounded, signed URL queries are preserved, time/size limits apply, and the GitHub SHA-256 digest and declared size must match.
- miniz from pinned official ReSkate 2.1.1 source validates/decompresses the ZIP with CRC checking and bounded entry/expanded sizes. Only this mod's known client files and directories are accepted; traversal, absolute/device/alternate-stream names, symlinks/reparse paths, duplicate/unsupported entries, source packages and unexpected targets are rejected. Package manifest version and x64 PE identities are checked.
- Download and verification only write under scripts/Custom/FirstPerson/updates. Installation is a separate explicit user action. The one-shot FirstPersonUpdater.exe is copied into its job folder, binds to the exact running game process, and waits for that process to exit. It never kills the game, installs a service/task/autorun, or launches the game/launcher.
- After exit it verifies the actual supported Skate.exe and exact official ReSkate.dll hashes, refuses other running Skate instances for the same installation, checks the owned DirectInput loader hash, and preflights destination locks/write access. Each original file is backed up and new files are staged durably before replacements. Files are verified after replacement. Normal installation errors roll back; the journal and backups support retry/recovery.
- Saved settings.ini, personal presets including Main.ini, logs and Mods/mods.json launcher enablement are never release targets. The current mapped proxy and companion are not unloaded or replaced live. DirectInput forwarding code is unchanged.
- On success a completion message tells the user to launch when ready. There is **no automatic relaunch**. On failure the archive/backups remain and the error is shown/logged in the update job folder. There are no multiplayer/chat messages.

## How to test the current public release

1. Close Skate. Install JishsFirstPersonMod-1.35.0-updater-test1-reskate-2.1.1.zip normally beside Skate.exe, merging Mods/scripts. Keep the known-working release ZIP available.
2. Edit only Mods/JishsFirstPersonMod/manifest.json: set version_number to **1.34.0**. The test package intentionally ships that field as 1.35.0 so it does not offer an update unless you lower it. Do not change ReSkate's own manifest or any DLL version.
3. Start via your usual launcher. Open Insert → Skater → First Person → Mod updates. It should report installed 1.34.0 and latest 1.35.0. Use Check for updates if needed.
4. Choose Download update and wait for verification, then choose Install after I close Skate. You can cancel before exiting.
5. Exit Skate normally. Wait for the installer completion message before launching anything again. Start ReSkate yourself afterwards.
6. Confirm manifest version_number is back to 1.35.0 and your settings, Main preset, launcher enablement, camera/look modes, embedded menu and emote protection remain correct.

**Expected:** GitHub's current public 1.35.0 does not contain this updater. Installing it successfully replaces the updater test companion, so the Mod updates section disappears. This test proves pulling/installing the current release, not retaining an unreleased feature inside that older public binary. The helper/staged backups may remain as inert files.

## Automated and live results

**43/43 CTest tests passed**, including all 40 original tests. New tests cover semantic versions, strict release metadata, redirects/query preservation, bad digest, actual published client layout, unsafe/malicious ZIP paths and entry types, oversized/decompression failures, destination locks/read-only files, unknown loader refusal, successful transactions, injected mid-install rollback, stale journal recovery and user-data/launcher preservation. Background state tests cover disabled checks, lowered manifest detection, decline, offline responses, check/download cancellation, download without installation, and pending-update restoration. The expanded Mod updates UI also passes existing scope/context tests.

A real WinHTTP check/download against the public GitHub release passed with an isolated manifest set to 1.34.0: latest 1.35.0 was offered, downloaded, SHA-256/size/ZIP-validated and staged. The installer was **not armed** in this probe; no game files were changed. See updater-live-github.log. Original 2.1.1 native binary audit passes; no private UI/camera addresses changed.

Full installation after a real running Skate session, visible menu/controller operation and notification timing remain user acceptance tests. Loaded-DLL, permission, rollback and recovery behavior have filesystem/unit coverage, not a completed in-game update claim. Reparse-path checks are implemented; protected-drive/junction and forced-power-loss scenarios are not live-validated. Abrupt power/process loss can leave a durable recovery journal; the next queued installer attempts recovery. Backups are retained for manual restoration if a damaged installation cannot start the test mod. Reopen the game only after completion.

Automatic installation is limited to the standard scripts/Custom layout and this mod's verified loader. Custom data-path/third-party loader layouts fail closed. Packages targeting another ReSkate build are not installed. No reboot-time replacement, hot reload or upstream ReSkate modification is used.

## Build and deliverables

Run ./build.ps1 (Windows x64, VS18 2026 C++ tools), then ./package.ps1. A CMake-generated digest binds the helper/companion to the included proxy build. The helper is staged under scripts/Custom/FirstPerson/updater, keeping the client archive roots to dinput8.dll, scripts, Mods and LICENSE.txt. The miniz license is included in the combined client LICENSE.txt. Source archive and patch are provided locally; nothing is published by these scripts.

Only bootstrap startup and embedded-menu presentation changed among the original src files. Camera algorithms, input/emote filters, original proxy/Lua loader, settings/preset implementations and saved data behavior remain byte-identical to the supplied handover. The test was built from the supplied handover rather than substituted from another checkout. User authorization superseded the handover's no-helper restriction, specifically allowing installation after manual exit with no relaunch.
