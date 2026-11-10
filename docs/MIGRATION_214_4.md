# ReSkate 2.1.4-4 compatibility audit

Mod 1.36.10, based on published 1.36.9 / commit 369b7fd6d05a48690f978b3eb77308619ac6b5d9.

Official tag: https://github.com/Dingo-Shenanigans/ReSkate/releases/tag/v2.1.4-4 (source be6f262).
Installed and downloaded official DLL SHA-256: `1bcd400b6c487f497ab0110263444864c0269a6309f7143d22b5bc04863d544e`.

## Mutex and layouts

The MSVC /MT Release source probe compiles with explicit static assertions. Both mutex representations remain 80 bytes. Overlay render_mutex remains at 0, Remote.mutex at 72. Session.mutex moves from 783624 to 787688. Lock/unlock thunks and the recursive mutex implementation are re-derived and byte-guarded. All 68 recorded fields/sizes are checked, including unchanged fields; the removed menu_fullscreen bool is replaced by menu_layout int.

| Field | 2.1.4-2 | 2.1.4-4 |
| --- | ---: | ---: |
| Session.chat_view | 782904 | 786968 |
| O.menu | 28968 | 29064 |
| S.bold | 35576 | 35616 |
| S.heading | 35592 | 35632 |
| M.bindings | 24520 | 24536 |
| M.multiplayer | 24688 | 24704 |
| M.menu_layout | new | 24508 |
| M.revisions | 26120 | 26216 |
| Model.size | 26152 | 26248 |
| Remote.size | 896 | 912 |
| Session.mutex | 783624 | 787688 |
| Session.mode | 780680 | 784664 |
| Session.epoch | 780720 | 784704 |
| Session.host_id | 780736 | 784720 |
| Session.secret | 780712 | 784696 |
| Session.used_slots | 779680 | 783664 |
| Session.view | 781368 | 785352 |
| Peer.size | 3120 | 3136 |
| MultiplayerModel.dedicated | 324 | 380 |
| MultiplayerModel.connected | 826 | 882 |
| MultiplayerModel.lobby_name | 592 | 648 |

Debug revision now occupies only bytes 26224..26231 of the 26248-byte model. Overlay callbacks/model remain at 2760/2808; window/context remain at 728/2752. Native recording gamepad copying remains 16 + 4 bytes. Remote records now stride 912 bytes and peers 3136 bytes.

## Address discovery

Full signatures mask only relative references. Stack/layout/TLS changes require additional review. The menu rewrite made superficial prologue matches misleading: the final First Person page is anchored by first-person/FIRST PERSON strings and skater-tabs dispatch. Each widget is confirmed at its matching call site and against its source declaration. No global address delta is used. See tools/finalize_214_4.py and the full relationship audit in tools/audit_214_4.py.

| Function | 2.1.4-2 RVA | 2.1.4-4 RVA |
| --- | --- | --- |
| lock | `0x14d1554` | `0x1517e44` |
| unlock | `0x14d1580` | `0x1517e70` |
| fov | `0xda6500` | `0xda53f0` |
| sampler | `0xda7cb0` | `0xda6ba0` |
| queue | `0x999ec0` | `0x99b000` |
| console | `0x999ba0` | `0x99ace0` |
| controller | `0xfc9b40` | `0xff3e70` |
| skater_page | `0x105cc40` | `0x10a8700` |
| mem_alloc | `0x12c5630` | `0x130abb0` |
| mem_free | `0x12c56e0` | `0x130ac60` |
| msc_queue | `0x99a0d0` | `0x99b210` |
| ctor | `0xfbf420` | `0xfe9640` |
| render | `0xaf0ec0` | `0xae53f0` |
| status | `0xfc2e50` | `0xfed180` |
| hid | `0xfd3b60` | `0xffde90` |
| lock_impl | `0x14d15a4` | `0x1517e94` |
| msc_command | `0xa31900` | `0xa26330` |
| getter | `0xfc2700` | `0xfeca30` |
| source_init | `0xd98490` | `0xd97400` |
| msc_send | `0xa0e4e0` | `0xa02e70` |
| read | `0x99a440` | `0x99b580` |
| note | `0x10109b0` | `0x10398b0` |
| toggle_row | `0x1012d30` | `0x103db60` |
| gamepad | `0xfc7c40` | `0xff1f70` |
| draw | `0x105bf60` | `0x10a7820` |
| begin_card | `0x1009430` | `0x1030110` |
| end_card | `0x100f4d0` | `0x10361a0` |
| field | `0x100f5c0` | `0x1037020` |
| slider_float | `0x1012600` | `0x103d1a0` |
| party_getter | `0xb1ac90` | `0xb0f8e0` |
| msc_session | `0x9e6870` | `0x9da4c0` |
| remote_getter | `0xaf0cd0` | `0xae5200` |
| context | `0x12c0820` | `0x1305d20` |

## Globals and call sites

| Role | RVA |
| --- | --- |
| state | `0x1e6bf80` |
| party | `0x1c58448` |
| remote | `0x1c58250` |
| session | `0x1c57950` |
| alloc | `0x1c0f240` |
| userdata | `0x1f05890` |
| context | `0x1f05888` |
| free | `0x1c0f248` |
| source_initialized | `0x1c05270` |
| source_base | `0x1c05588` |
| source_transform | `0x1c05598` |
| party_transform_return | `0xd9a4af` |
| party_fov_return | `0xd9a4cf` |
| hid_callers | `0xffcdf9`, `0xffce10`, `0xffd727` |

## Verification

Release build and all 51/51 CTest checks passed. Offline DLL relationship audit and compiled source layout assertions passed. The installed Skate.exe remains the supported 20260929 image: 45 game code/vtable checks, bail-collision and emote address audits passed. Live gameplay results are not recorded in this audit.
