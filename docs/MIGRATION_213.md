# ReSkate 2.1.3 compatibility audit

Candidate: JishsFirstPersonMod 1.36.7. Baseline: published 1.36.6 / GitHub main archive 106a506. The user confirmed the candidate working in game and approved publication.

Official release: https://github.com/Dingo-Shenanigans/ReSkate/releases/tag/v2.1.3

Tagged source: 9506a39 (v2.1.3). DLL SHA-256: `139e932ad6328b898b2ff8c2aac3e0cd5ced6545c2f041aa8534239f97e59f9b`.

The release was audited offline; the DLL was never loaded for disassembly. Function candidates were matched by instruction signatures, then checked using exports, source layout, string references, call graphs and RIP-relative data relationships. The shared HID handler was identified using its unique menu-preview diagnostic and all three direct callers.

## Mutex and layout verification

Native mutex lock: `0x148a6f4`; unlock: `0x148a720`. The status export calls the mapped getter/lock/unlock, and the mod verifies these relationships before activation. Overlay render mutex offset remains 0; MSC Session mutex offset remains 783584. Local C++ mutexes in the companion have no ReSkate RVA.

| Overlay field | 2.1.2 offset | 2.1.3 offset |
| --- | ---: | ---: |
| ImGui context | 1720 | 2744 |
| Callbacks | 1728 | 2752 |
| Model | 1776 | 2800 |
| Skate menu | 27912 | 28936 |

The MSVC release (/MT, iterator debug level 0) probe compiled against the v2.1.3 tag checks 65 fields/sizes. Other recorded model, debug, bindings, ImGui, spectator, Remote, Session, Peer and chat fields retain their prior values. Gamepad reads still use the verified 20-byte game-facing buffer. The exact ImGui declarations/config remain identical to the companion headers.

## Function RVA inventory

| Role | 2.1.2 RVA | 2.1.3 RVA |
| --- | --- | --- |
| lock | `0x13d6914` | `0x148a6f4` |
| unlock | `0x13d6940` | `0x148a720` |
| fov | `0xd2c230` | `0xd7f510` |
| sampler | `0xd2d9e0` | `0xd80cc0` |
| queue | `0x988600` | `0x9893a0` |
| console | `0x9882e0` | `0x989080` |
| controller | `0xf34550` | `0xfa4cc0` |
| draw | `0xfa0ec0` | `0x1021d00` |
| skater_page | `0xfa1ba0` | `0x10229e0` |
| begin_card | `0xf56160` | `0xfd0d40` |
| end_card | `0xf5b6d0` | `0xfd6560` |
| field | `0xf5b7c0` | `0xfd6650` |
| note | `0xf5cbb0` | `0xfd7a40` |
| toggle_row | `0xf5efe0` | `0xfd9dc0` |
| mem_alloc | `0x11cca60` | `0x127eb30` |
| mem_free | `0x11ccb10` | `0x127ebe0` |
| status | `0xf1d8e0` | `0xf8be60` |
| slider_float | `0xf5e890` | `0xfd9690` |
| msc_command | `0xa1f4b0` | `0xa205f0` |
| msc_queue | `0x988810` | `0x9895b0` |
| ctor | `0xf19f60` | `0xf88470` |
| read | `0x988b80` | `0x989920` |
| msc_send | `0x9fbd00` | `0x9fce20` |
| getter | `0xf1d190` | `0xf8b710` |
| source_init | `0xd1e1c0` | `0xd714a0` |
| render | `0xadcf30` | `0xade8d0` |
| gamepad | `0xf32950` | `0xfa30c0` |
| context | `0x11c7cf0` | `0x1279dc0` |
| remote_getter | `0xadcd40` | `0xade6e0` |
| party_getter | `0xb02000` | `0xb03ab0` |
| msc_session | `0x9d5e10` | `0x9d6e30` |
| hid | `0xf2c110` | `0xf98990` |

## Data slots and call sites

| Role | 2.1.3 RVA |
| --- | --- |
| state | `0x1cf3ca0` |
| party | `0x1ba0968` |
| remote | `0x1ba0810` |
| session | `0x1ba0010` |
| alloc | `0x1b63d20` |
| userdata | `0x1d8d2e0` |
| context | `0x1d8d2d8` |
| free | `0x1b63d28` |
| source_initialized | `0x1b59dd0` |
| source_base | `0x1b5a0e8` |
| source_transform | `0x1b5a0f8` |
| party_transform_return | `0xd7454f` |
| party_fov_return | `0xd7456f` |
| hid_callers | `0xf97749`, `0xf97760`, `0xf98077` |

## Source and binary evidence

- `tools/migration213/contracts.json`: 32 function addresses and exact byte prefixes.
- `tools/migration213/data-contracts.json`: globals, HID callers and spectator return sites.
- `tools/migration213/layout.txt`: independently compiled official source layout.
- `tools/migration213/audit-report.json`: source hashes and relationship audit result.
- `tools/migration213/game-offset-audit.txt`: 45 game code/vtable checks.
- `tools/migration213/bail-audit.txt`: 9 engine collision checks.
- `tools/migration213/emote-audit.txt`: emote type/getter/parameter checks.

Supported game SHA-256: `fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`. Existing game-side address declarations still match the official source; additions upstream do not change the declarations this mod uses.

The new DS4 handler retains the same HidRead POD and call ABI but changes implementation and adds a third direct call site. The companion imports the official 2.1.3 report helper and uses its padded-report CRC behavior.

Runtime initialization, multiplayer hooks and the updater require the exact 2.1.3 identity. Dedicated regression tests reject 2.1.2 and unreviewed identities, modified native prefixes, and altered mutex/camera references. Existing camera algorithms are retained.

See TEST_213.md for installation and live gameplay checks. Passing offline audits and automated tests does not establish in-game correctness.

## Release validation

Final Release build succeeded. All 51 automated tests passed, including the 2.1.3 identity/native guards, 2.1.2 rejection, mutex/camera reference tamper checks, padded DS4 CRC/padding behavior, controller capture, camera policies, hooks, loaders, settings and updater tests. The automated validation did not perform live driver discovery or gameplay; the user separately confirmed the candidate working in game. The packaged client passes the updater package validator (7 owned files).
