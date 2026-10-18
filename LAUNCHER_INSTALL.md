# Local ReSkate 2.1.0 compatibility test

This package has not been installed into the working game by the migration build.

Close Skate. Back up your working package, then extract Mods, scripts and dinput8.dll beside Skate.exe and ReSkateLauncher.exe in a ReSkate 2.1.0 installation. Merge folders. Enable Jish's First Person Mod in the launcher. Keep existing settings and presets under scripts/Custom/FirstPerson. Resolve any existing third-party DirectInput proxy conflict before replacing dinput8.dll.

Only the exact official ReSkate 2.1.0 DLL is supported: SHA-256 310def046836772ef0dea92cfede10e524f4b7fe30255b470e85f2dcbab8c520. Older versions and unknown binaries are refused before private hooks, including with the old force-detection environment variable set.

Insert opens the stock menu and companion. F9 hides/shows the companion within the stock menu. Use mouse and keyboard in the companion; controller navigation belongs to ReSkate.

Settings, presets and logs retain scripts/Custom/FirstPerson. No settings reset. Live camera, graphics, controller and multiplayer checks are pending; use docs/COMPATIBILITY_210_TEST_REPORT.md.
