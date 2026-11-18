# Updater improvements for ReSkate Build 20261116

The checker selects the newest compatible stable release from the GitHub release list. Background checks start earlier in the DLL startup worker, and recent published loaders are recognized.

With Skate closed, run scripts/Custom/FirstPerson/updater/FirstPersonUpdater.exe to check, download and install an update. Installation requires confirmation.

All 51 automated tests passed. The user confirmed the updater worked, and the isolated installation recorded a completed update.
