# Jish's First Person Mod - Animator Studio release

This is a separate compatibility build of 1.36.7 for FourthStudio's release tag **1.0.101**, whose release notes identify the DLL as **Studio 1.0.11 / ReSkate 2.1.3**. Use this package with that Studio DLL. Use the normal 1.36.7 package with official ReSkate. This test build does not switch between both runtime profiles in one DLL.

## Install and test

1. Close Skate and the ReSkate launcher. Back up your existing First Person package and settings.
2. Install official ReSkate 2.1.3 and the Animator Studio DLL following the Studio release instructions: https://github.com/FourthStudio/ReSkate-Animator-Studio/releases/tag/1.0.101
3. Extract the compatibility ZIP beside Skate.exe, merging Mods and scripts. Enable Jish's First Person Mod in the launcher, then restart through ReSkateLauncher.exe.
4. Open Insert > Skater > First Person. F8 remains Animator Studio's shortcut.
5. Test on-foot and on-board first person, right-stick look, head attachment, jumping, bails/get-up, return to marker, and spectator view. Activate a Studio animation/playlist and repeat. Check controller binding recording and confirm Studio's F8 UI still opens.

The mod requires exact ReSkate.dll SHA-256 `ed8f07863bcc3de1d83371de0f90b368ca55602d793d1bd84dbbff44b6d76801`. Other Studio DLLs are rejected before private hooks activate. An unrecognized DLL needs a separate audit; toggling the camera's look-only compatibility setting does not change the runtime profile.

Existing personal settings and presets are preserved by merging the package. The bundled multi-mod loader discovers scripts/Custom DLLs. If you already use UniversalModLoader, build/package with -UniversalLoader to keep that loader.

## Test-build limitations

- The user confirmed this exact test 1 client working in game and selected it for release. All 51 automated tests and the offline binary/layout audit passed. This does not establish compatibility with every Studio animation or camera option.
- MSC [FP] presence announcements are disabled because Studio's private command dispatcher is not yet audited. Multiplayer itself is not disabled.
- Automatic First Person updates are disabled to prevent an official-only package replacing the Studio build. Install Studio-compatible updates manually.
- Studio adds its own camera behavior. The test above must verify that its animation and first-person settings work with your chosen mod settings.

To revert, close the game and restore the regular First Person package alongside the official ReSkate DLL, or restore your backups.

## Build and audit

Run `./build.ps1`, then `./package.ps1` using Windows x64 Visual Studio 2026 C++ tools. This source tree is the pinned Studio profile; the official source tree is unchanged.

`tools/animator_studio/` records exact function addresses, data slots, compiled source layout and the offline audit result. Run `python tools/audit_animator_studio.py <ReSkate.dll> --upstream <Studio source directory>` with pefile and capstone installed. Compile `tools/layout212` against the pinned Studio source to independently verify its ABI. The source revision is `ce058bfd5b7a3d28094676e70bb05454276a77f3`.

See SETTINGS.md for camera controls. Historical migration documents describe earlier official releases, not validation of this Studio test build.

## Separate release channel

This branch is `animator-studio`. Keep Studio changes here and publish Studio-specific tags and assets from commits on this branch. The official source and release channel stay on `main`. Studio releases must use `make_latest=false` so the official updater keeps its existing release channel. This release preserves the original test 1 ZIP and its internal version `1.36.7-studio-test1`; test 2 and MSC announcements are excluded. See docs/ANIMATOR_STUDIO_RELEASE.json for exact artifact hashes.
