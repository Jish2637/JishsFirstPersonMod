# Installation - 1.36.7 multi-mod loader release for ReSkate 2.1.3

Close Skate. Back up your existing dinput8.dll outside the game folder (or as dinput8.dll.bak), then extract the full client ZIP beside Skate.exe, merging Mods and scripts and replacing dinput8.dll. Existing personal settings, other DLL mods and scripts/Custom/dinput8_mods.txt are preserved.

The bundled dinput8 now supports multiple mods using UniversalModLoader v2 behavior. Put DLL mods in scripts/Custom. The loader discovers new DLLs automatically, removes missing entries, preserves your load order and keeps '# Mod.dll' entries disabled. It never treats dinput8*.dll as a mod. The ReSkate launcher enable switch still applies to First Person.

On restart, check Insert > Skater > First Person. Loader diagnostics are in scripts/Custom/UniversalLoader/loader.log; First Person diagnostics remain in scripts/Custom/FirstPerson/logs/JishsFirstPersonMod.log.

First Person updates recognize the bundled multi-mod loader and the verified original UniversalModLoader v2 build (SHA256 438e671de5910e04e1af3ade92f3e72f14cbf12fb63df23676863794a43a0559). Updates preserve these loaders, their mod list, chained proxy, other mods, personal settings and presets. Unknown loader builds still require manual installation. Future releases retain the known loader digests.

For updates, use Mod updates > Check for updates, then Download update and Install after I close Skate. Close Skate normally and restart it yourself when installation completes. Only newer published GitHub releases are offered. Older updater builds may require this release to be installed manually.

Optional proxy chaining: an existing dinput8_chain.dll beside Skate.exe receives DirectInput forwarding. Otherwise forwarding uses the Windows system DLL. Do not rename the old universal loader to dinput8_chain.dll just to back it up.

Build the full multi-mod package with .\build.ps1 followed by .\package.ps1. To omit the loader for an existing supported installation, both commands also accept -UniversalLoader.
