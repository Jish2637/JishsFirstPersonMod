# 1.36.7 — ReSkate 2.1.3

Updates the first-person mod for the exact official ReSkate Release 2.1.3 DLL. Re-derived and audited all private runtime addresses, globals, mutex calls, embedded UI offsets, controller/HID readers and spectator/MSC interfaces. The overlay context, callback table, model and menu each moved by 1,024 bytes.

DS4 Bluetooth handling now accepts Windows-padded 547-byte reads and refreshes the CRC at byte 74 while preserving padding. The updater and startup checks use the reviewed 2.1.3 runtime hash.

The published client uses the same DLL, updater and loader binaries as the test candidate the user confirmed working in game. All 51 automated tests and the binary/source audits pass. See MIGRATION_213.md for the audit inventory.

Close Skate and back up your current mod files and loader. Extract the complete client ZIP beside Skate.exe, merge Mods and scripts, enable the mod in the launcher, and restart. Existing settings, personal presets, other mods and dinput8_mods.txt are retained. Older updaters may require this update to be installed manually.
