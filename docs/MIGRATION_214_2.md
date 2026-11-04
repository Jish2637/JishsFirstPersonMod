# ReSkate 2.1.4-2 compatibility audit

Release 1.36.9, based on published 1.36.8 / commit f66581dbc9a527649713d39aa0d8b5efd6b0fb18. Publication was authorized on 4 November 2026. The client files match the verified local build; live gameplay validation is not recorded in this automated audit.

Official release: https://github.com/Dingo-Shenanigans/ReSkate/releases/tag/v2.1.4-2

Tagged source: 88540f2. Installed and official downloaded DLL SHA-256: `f7b1562667892187356b8617a3964ceaefea04324bfa7850c05d340da01c09cd`.

## Mutex and layout audit

MSVC Release /MT, iterator debug level 0: 68 recorded fields/sizes. Native mutex thunks and lock implementation were independently re-derived and byte-guarded. Both MSVC mutex types remain 80 bytes. Overlay render_mutex remains at 0; the multiplayer Session mutex moved from 783584 to 783624. Remote.mutex remains at 72.

| Field | Original 2.1.4 | 2.1.4-2 |
| --- | ---: | ---: |
| Overlay menu | 28944 | 28968 |
| Model revisions | 26096 | 26120 |
| Debug revision within Model | 26104 | 26128 |
| Model size | 26128 | 26152 |
| Party state size | 48640 | 48704 |
| Session view | 781352 | 781368 |
| Session chat view | 782864 | 782904 |
| Session mutex | 783584 | 783624 |
| Multiplayer connected flag | 802 | 826 |

Overlay window/context/callbacks/model offsets remain 728/2752/2760/2808. Remote record stride remains 896. Other recorded model/debug/bindings/ImGui/party/remote/session/peer/chat fields remain unchanged. The debug refresh now zeroes only the eight-byte field at 26128; its memory-boundary regression fixture uses the new 26152-byte model.

## Function inventory

| Role | Original 2.1.4 RVA | 2.1.4-2 RVA |
| --- | --- | --- |
| lock | `0x14c0354` | `0x14d1554` |
| unlock | `0x14c0380` | `0x14d1580` |
| fov | `0xd9c050` | `0xda6500` |
| sampler | `0xd9d800` | `0xda7cb0` |
| queue | `0x999de0` | `0x999ec0` |
| console | `0x999ac0` | `0x999ba0` |
| controller | `0xfbd2e0` | `0xfc9b40` |
| draw | `0x104e0d0` | `0x105bf60` |
| skater_page | `0x104edb0` | `0x105cc40` |
| begin_card | `0xffcbd0` | `0x1009430` |
| end_card | `0x1002910` | `0x100f4d0` |
| field | `0x1002a00` | `0x100f5c0` |
| note | `0x1003df0` | `0x10109b0` |
| toggle_row | `0x1006170` | `0x1012d30` |
| mem_alloc | `0x12b3fb0` | `0x12c5630` |
| mem_free | `0x12b4060` | `0x12c56e0` |
| slider_float | `0x1005a40` | `0x1012600` |
| msc_queue | `0x999ff0` | `0x99a0d0` |
| ctor | `0xfb2bc0` | `0xfbf420` |
| msc_send | `0xa0c430` | `0xa0e4e0` |
| render | `0xaed680` | `0xaf0ec0` |
| gamepad | `0xfbb3e0` | `0xfc7c40` |
| status | `0xfb65f0` | `0xfc2e50` |
| remote_getter | `0xaed490` | `0xaf0cd0` |
| hid | `0xfc7300` | `0xfd3b60` |
| lock_impl | `0x14c03a4` | `0x14d15a4` |
| getter | `0xfb5ea0` | `0xfc2700` |
| source_init | `0xd8dfe0` | `0xd98490` |
| party_getter | `0xb16c90` | `0xb1ac90` |
| context | `0x12af1a0` | `0x12c0820` |
| msc_session | `0x9e6620` | `0x9e6870` |
| read | `0x99a360` | `0x99a440` |
| msc_command | `0xa2fa40` | `0xa31900` |

## Globals and call sites

| Role | 2.1.4-2 RVA |
| --- | --- |
| state | `0x1da88f0` |
| party | `0x1bf5168` |
| remote | `0x1bf4f70` |
| alloc | `0x1bb7fc0` |
| userdata | `0x1e41f70` |
| context | `0x1e41f68` |
| free | `0x1bb7fc8` |
| source_initialized | `0x1bae010` |
| source_base | `0x1bae328` |
| source_transform | `0x1bae338` |
| party_transform_return | `0xd9b53f` |
| party_fov_return | `0xd9b55f` |
| hid_callers | `0xfd2ac9`, `0xfd2ae0`, `0xfd33f7` |
| session | `0x1bf4730` |

## Verification scope

- Exact SHA-256 and status export, 33 native function prefixes, status/getter/lock/unlock relationships and mutex implementation comparison.
- All global data slots and compiled contract literals, source camera publication, widget call graph, spectator camera/FOV return sites, all three HID callers, and host ImGui context/allocation slots.
- Callback bodies with changed stack frames were matched by instructions while masking stack/relative operands, preserving other operands, then checked against the new model revision loads and native allocation sizes.
- Native Session allocation 783704 equals its shifted mutex end; native Party allocation is 48704, matching the independent source compiler probe.
- Model and session offset changes are checked against compiled source and the literals actually used in the companion; no common address shift is assumed.
- Game executable SHA-256 remains fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9. The 45 game code/vtable checks, 9 collision checks and emote address audit pass.
- ImGui declarations/config and imported PlayStation helper match the pinned tag. Existing camera maths and the stock HID chain are retained.

Evidence: tools/migration214_2/{contracts.json,data-contracts.json,layout.txt,audit-report.json,audit-output.txt,game-offset-audit.txt,bail-audit.txt,emote-audit.txt}. The official source compiler probe is tools/layout214_2.

Automated results: docs/CTEST_214_2.txt. See VALIDATION_214_2.md for installation and live gameplay checks.

## Final automated validation

Release build succeeded; 51/51 CTest tests passed, including native 2.1.4-2 identity/ABI guards, rejection of original 2.1.4, changed mutex/camera references, exact debug-revision memory boundaries, controller capture, spectator behavior, settings, loader and updater checks. The independent source compiler probe also checks all party record/player/position and remote fields consumed by the spectator bridge. Live gameplay checks are documented separately in VALIDATION_214_2.md.
