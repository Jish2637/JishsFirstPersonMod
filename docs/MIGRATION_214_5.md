# ReSkate 2.1.4-5 compatibility audit

Jish's First Person Mod 1.36.11 uses the official ReSkate 2.1.4-5 DLL. The installed DLL at `C:\Games\ReSkate\ReSkate.dll` matches the official release asset byte for byte.

- Tagged source revision: `8320081`
- DLL SHA-256: `0820fa71c1cb716f4222eb9027130933aa9e3db4ce20b46ef555760be997dbde`
- DLL code section SHA-256: `e52ba34b1a7e5d9bec0440601f589f0a1a09481c6dfacfdded3c771bffd03b8e`

The Release layout probe compiled against the new tagged source. All 68 reported fields and sizes match 2.1.4-4. Both mutex types remain 80 bytes. The overlay render mutex is at offset 0, the remote mutex is at 72, and the session mutex is at 787688. The native lock thunk, unlock thunk and lock implementation resolve at `0x1517e44`, `0x1517e70` and `0x1517e94`. The status accessor still calls the verified getter, lock and unlock, and the lock thunk still targets the verified implementation.

The DLL code section is byte-identical to 2.1.4-4 at the same RVA. The audit also independently resolved and checked all 33 private function addresses and prefixes, 11 global slots, two spectator return sites and three HID callers. The audit verifies the menu dispatch, camera callbacks, ImGui allocator, and multiplayer session references. The remote record stride is 912 bytes and peer stride is 3136 bytes. The installed Skate.exe passed the existing 45 game address checks, nine bail collision checks and the emote contract audit.

Startup and the updater now require the exact official 2.1.4-5 DLL. See `tools/migration214_5` for the full address inventory, compiled layout and audit outputs. The Release build and all 51 automated tests passed. The user confirmed the release works in game.
