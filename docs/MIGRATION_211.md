# ReSkate 2.1.1 source and binary provenance

Baseline: the working 1.34.0-compat-test2 ReSkate 2.1.0 source and DLL, with MSC reporting removed. New source tree: local-reskate-2.1.1-source. Previous sources, release packages and game installation are unchanged.

Upstream repository: https://github.com/Dingo-Shenanigans/ReSkate
Tag: v2.1.1. Commit: db99ed4d724dd77edd2abf62f3de0dbc5bb6717e.
Pinned source archive SHA-256: aeaa700023625a78bac55debbf7437ff2beedd80a2a6ea85a32b013f120c3458.
Source reference: upstream/source-2.1.1/ReSkate-db99ed4d724dd77edd2abf62f3de0dbc5bb6717e.
Official ReSkate DLL SHA-256: d79ff144768ce8b0a8e992c9f134c670a3ff85bb3b437a0eb915876973014301.
Game SHA-256: fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9. The installed Skate.exe matches the existing identity; game contracts were not changed.

39 upstream files changed and three were added. Camera algorithms and the relevant release layouts remain unchanged. Upstream added menu flags in existing padding; the compiled layout probes still confirm the model size, callbacks, revisions, bindings, controller and spectator contracts. Native functions moved by differing amounts; several callbacks did not move. No uniform address adjustment was used.

## Build

Windows x64, Visual Studio 2026 C++ tools, Windows SDK, Release /MT, C++20. Run ./build.ps1 from this source directory. It runs the 38-test suite and stages output locally. Tests require official fixture DLLs at ../upstream/ReSkate-2.1.1.dll and ../upstream/ReSkate-2.1.0.dll. The latter is a negative compatibility fixture, not a supported runtime.

Run ./package.ps1 to produce the client ZIP and matching source ZIP. The client archive contains only dinput8.dll, scripts, Mods and LICENSE.txt.

## Independent native verification

Configure tools/layout211 with CMake, x64, -DUPSTREAM=<absolute pinned upstream source>. Build Release and run layout211, official_layout and spectator_layout. Use the matching /MT release ABI and _ITERATOR_DEBUG_LEVEL=0.

For the offline binary audit, install Python pefile and capstone, then run:

    python tools/audit_211.py --dll ../upstream/ReSkate-2.1.1.dll --upstream ../upstream/source-2.1.1/ReSkate-db99ed4d724dd77edd2abf62f3de0dbc5bb6717e

The audit never loads the DLL. It verifies all 17 exact entry prefixes plus source-derived code/data relationships, including status/getter/mutex, constructor model operands, native camera slots, spectator transform/FOV calls, HID callers and gamepad sampling. The live initializer requires the exact official 2.1.1 identity and cannot fall back to 2.1.0 or force an unknown binary.

No presence sender, capability discovery or MSC command hook is included. No server, network protocol, authentication, Steam dependency, game data or release is modified by this migration. Older 210-named documents are historical; use the 211 test report for this build.
