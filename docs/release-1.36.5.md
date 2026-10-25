# Jish's First Person Mod 1.36.5 - ReSkate 2.1.2

- Added a bundled multi-mod dinput8 loader based on UniversalModLoader v2. It discovers DLL mods in scripts/Custom and preserves load order, comments and disabled entries in dinput8_mods.txt.
- Updated First Person's updater to recognize the bundled multi-mod loader and the verified original UniversalModLoader v2. Updates preserve these loaders, their mod lists, chained proxies and other mods.
- Fixed the installer checking an outdated ReSkate runtime hash. Downloader and installer now use the same verified ReSkate 2.1.2 identity.
- Improved startup so camera hooks and Insert settings initialize before slow optional controller probing. XInput, Raw Input and HID filters can attach before DirectInput discovery finishes.
- Fixed failed optional emote/MSC hook attachments interfering with later gesture-blocking setup.
- Added optional on-foot right-stick camera limits with separate up, down, left and right angles. Off by default; available under Advanced > On-foot right stick look. Limits persist in settings and presets.
- Added experimental native emote aiming in Camera only mode. Pointing follows the first-person view while stationary; running can temporarily return pointing to forward until stopping.

Existing bail camera, collision improvements, MSC [FP] tags, native head movement and throwdown spectator freelook are retained.

Validation: release build and 51 automated regression/integration tests pass. The user confirmed the multi-mod build works in game. Actual install-after-exit auto-updating has not been exercised against the live game installation.

## Installation

Close Skate and back up your existing dinput8.dll, then extract JishsFirstPersonMod-1.36.5-reskate-2.1.2.zip beside Skate.exe and ReSkateLauncher.exe, merging Mods and scripts. Install the whole ZIP so the First Person DLL and updater match the loader. Existing settings, personal presets, other mods and dinput8_mods.txt are preserved. Enable Jish's First Person Mod in the launcher, restart, then open Insert > Skater > First Person.

The verified original UniversalModLoader v2 (SHA256 438e671de5910e04e1af3ade92f3e72f14cbf12fb63df23676863794a43a0559) is also supported by the new updater. Unknown loader builds still need manual installation. Loader upgrades are manual; First Person updates preserve recognized multi-mod loaders.

Requires the reviewed ReSkate 2.1.2 DLL, SHA256 96112d915c181b696b5cb8ed17b62a74d09ca2900c4383a4500c1fa88bf76e43. ReSkate itself is not included.

For this release, manual installation is recommended for older mod/updater builds: their installer may still reject the current runtime or loader. After installing 1.36.5, future newer published releases can be checked/downloaded from Mod updates and queued for installation after closing Skate.
