# ReSkate Build 20261116 compatibility audit

Mod 1.36.12 is based on commit `819b89d`. The installed DLL matches the official 20261116 release asset. Tagged upstream source is `8e4aefc`.

- DLL SHA-256 `bb3a36fef26c0c779159a76b6a75669aeb8797a3192a2ab11c68b7b099c87969`
- DLL code section SHA-256 `5a81e8a7145308bbfd3d804a320a4cfddccdd55ed9d41bff0413e336ccad4611`

The compiled Release probe checked 68 layout fields and sizes. The overlay render mutex is at byte 0, the remote mutex at 72, and the multiplayer session mutex at 787688. Both MSVC mutex types are 80 bytes. The new lock thunk branches to the verified lock implementation, and the status accessor calls the new getter, lock and unlock addresses.

| Field | 2.1.4-5 | 20261116 |
| --- | ---: | ---: |
| O.menu | 29064 | 29128 |
| M.menu_scale | 24504 | 24568 |
| M.bindings | 24536 | 24600 |
| M.multiplayer | 24704 | 24768 |
| M.menu_layout | 24508 | 24572 |
| M.revisions | 26216 | 26280 |
| Model.size | 26248 | 26312 |
| Model debug revision | 26224 | 26288 |

The new DLL independently confirms the layout through the constructor, partial model loads, session allocation, and remote accessor. The audit also verifies menu dispatch, the camera publisher, ImGui allocations, HUD spectators and HID calls. The installed Skate.exe passed 45 game address and vtable checks, nine bail collision checks, and the emote address audit. The Release build and all 51 automated tests passed. The maintainer verified the packaged build in live gameplay.

| Function | 2.1.4-5 RVA | 20261116 RVA |
| --- | --- | --- |
| lock | `0x1517e44` | `0x15449e4` |
| unlock | `0x1517e70` | `0x1544a10` |
| fov | `0xda53f0` | `0xdb2bf0` |
| sampler | `0xda6ba0` | `0xdb43a0` |
| queue | `0x99b000` | `0x99bee0` |
| console | `0x99ace0` | `0x99bbc0` |
| controller | `0xff3e70` | `0x1002700` |
| skater_page | `0x10a8700` | `0x10bc8d0` |
| mem_alloc | `0x130abb0` | `0x1334c00` |
| mem_free | `0x130ac60` | `0x1334cb0` |
| msc_queue | `0x99b210` | `0x99c0f0` |
| ctor | `0xfe9640` | `0xff7ed0` |
| render | `0xae53f0` | `0xae7410` |
| status | `0xfed180` | `0xffba10` |
| hid | `0xffde90` | `0x100ce60` |
| lock_impl | `0x1517e94` | `0x1544a34` |
| msc_command | `0xa26330` | `0xa28440` |
| msc_send | `0xa02e70` | `0xa05000` |
| read | `0x99b580` | `0x99c460` |
| note | `0x10398b0` | `0x104ce50` |
| toggle_row | `0x103db60` | `0x10512e0` |
| gamepad | `0xff1f70` | `0x1000800` |
| draw | `0x10a7820` | `0x10bb9f0` |
| begin_card | `0x1030110` | `0x10431f0` |
| end_card | `0x10361a0` | `0x1049740` |
| field | `0x1037020` | `0x104a5c0` |
| slider_float | `0x103d1a0` | `0x1050920` |
| party_getter | `0xb0f8e0` | `0xb11c00` |
| msc_session | `0x9da4c0` | `0x9dc150` |
| remote_getter | `0xae5200` | `0xae7220` |
| getter | `0xfeca30` | `0xffb2c0` |
| source_init | `0xd97400` | `0xda4c00` |
| context | `0x1305d20` | `0x132fd70` |

| Global or call site | 2.1.4-5 RVA | 20261116 RVA |
| --- | --- | --- |
| state | `0x1e6bf80` | `0x1ea1248` |
| party | `0x1c58448` | `0x1c8d628` |
| remote | `0x1c58250` | `0x1c8d430` |
| session | `0x1c57950` | `0x1c8cb38` |
| alloc | `0x1c0f240` | `0x1c417e0` |
| userdata | `0x1f05890` | `0x1f3ac20` |
| context | `0x1f05888` | `0x1f3ac18` |
| free | `0x1c0f248` | `0x1c417e8` |
| source_initialized | `0x1c05270` | `0x1c37730` |
| source_base | `0x1c05588` | `0x1c37a48` |
| source_transform | `0x1c05598` | `0x1c37a58` |
| party_transform_return | `0xd9a4af` | `0xda7caf` |
| party_fov_return | `0xd9a4cf` | `0xda7ccf` |
| hid_callers | `0xffcdf9`, `0xffce10`, `0xffd727` | `0x100be39`, `0x100be50`, `0x100c767` |
