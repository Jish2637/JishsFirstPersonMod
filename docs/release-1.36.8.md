# 1.36.8 — ReSkate 2.1.4

Updates the first-person mod for the exact official ReSkate Release 2.1.4 DLL. All private function addresses and globals were re-derived, including mutex thunks/implementation, camera callbacks, embedded UI and ImGui allocation, controller/HID readers, spectator records and MSC session interfaces.

Five overlay offsets moved by 8 bytes. Remote-player records grew from 888 to 896 bytes. Both changes are handled, and exact runtime identity checks in startup and the updater now target 2.1.4.

The user confirmed the candidate working in game before approving publication. Release client files are identical to the tested candidate. All 51 automated tests pass; the offline audit verifies 33 native entry points and 68 compiled layout fields/sizes. See MIGRATION_214.md and VALIDATION_214.md.

Close Skate and back up your existing mod and dinput8.dll. Extract the complete release ZIP beside Skate.exe, merge Mods and scripts, enable the mod in the launcher and restart. Existing personal settings, presets, other mods and dinput8_mods.txt are preserved. Older updaters may require this update to be installed manually.
