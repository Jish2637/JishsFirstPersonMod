# ReSkate 2.1.0 migration provenance and build

Baseline: released Jish's First Person Mod 1.34.0, Git commit `c0d2c4788fcbc1f970d4b3dc013a70f46b43bd6f`. Baseline source ZIP SHA-256: `e951170d96afd015587bc219e49964604b9d194b353f416a80d91bd8cc07d2bd`.

Official reference: tag `v2.1.0`, commit `ba1245a5af51892e90f2be8637dd69a7e20f5b77`. Separate source directory: `upstream/source-2.1.0/ReSkate-ba1245a5af51892e90f2be8637dd69a7e20f5b77`. Pinned source archive SHA-256: `56c51fd07fa5a8664203b35e88dab80c06ebed47aa9f3c3476ab8d1579aecbe5`.

Official reference DLL: `upstream/ReSkate-2.1.0.dll`, 29,231,104 bytes, SHA-256 `310def046836772ef0dea92cfede10e524f4b7fe30255b470e85f2dcbab8c520`. The mod package does not replace or redistribute that DLL. Obtain the official matching installation separately.

The upstream first-person spring maths and first-person implementation were compared with 2.0.7. The companion's head-position, bail, offset, smoothing, look-mode and body-flip algorithms are preserved. Integration changes cover overlay callbacks/models/bindings, DebugModel revision refresh, native contracts, controller menu ownership/recording capture, camera ownership evidence, spectator state. The stock renderer submission label, graphics readiness ordering, device/swapchain validation, barriers and fence waits are retained. Live render validation remains pending.

## Build

Windows x64, Visual Studio 2026 C++ tools and Windows SDK, Release `/MT`, C++20, `_ITERATOR_DEBUG_LEVEL=0`. From this source root:

```powershell
./build.ps1
./package.ps1
```

Place the official DLL at `../upstream/ReSkate-2.1.0.dll` for the binary fixture test. This is a test input, not a game installation target. Historical ABI profiles remain as regression fixtures; runtime initialization admits only the exact official 2.1.0 identity and cannot fall back to them.

## Independent layouts and binary audit

Configure `tools/layout210` with CMake using `-DUPSTREAM=<absolute pinned source directory>`, x64 and the same Release runtime. Build and run `layout210`, `official_layout` and `spectator_layout`. Static assertions check source-derived offsets, sizes and enum values. Saved output: `ABI_LAYOUT_210.txt`.

Install Python dependencies `pefile` and `capstone`, then run:

```powershell
python tools/audit_210.py --dll ../upstream/ReSkate-2.1.0.dll --upstream ../upstream/source-2.1.0/ReSkate-ba1245a5af51892e90f2be8637dd69a7e20f5b77
```

This offline audit never loads the DLL. It checks entry bytes, status/getter/mutex relationships, callback/model construction operands, camera slots, party transform/FOV calls, gamepad sampling and HID callers. `NATIVE_AUDIT_210.json` records the result; `tools/migration210/contracts.json` records reviewed entry prefixes. No uniform RVA adjustment is used.

Older audit tools in tools/ describe historical profiles and are not the authoritative 2.1.0 verifier.

## Limits

No game files, live server, public repository or release were changed. Live graphics, camera transitions, controllers, Steam Input and multiplayer require in-game confirmation. Protocol-50 MSC server sources are not modified here. ReSkate 2.1.0 requires protocol 52; live MSC testing needs a compatible server with the existing silent capability metadata.

## compat-test2

Removed MSC first-person reporting completely following reported chat spam. Multiplayer session/command contracts are no longer part of runtime validation. This package retains the camera and menu migration. The prior MSC migration discussion above is historical, not an active feature.
