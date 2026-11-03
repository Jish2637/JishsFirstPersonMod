# ReSkate 2.1.4 compatibility audit

Release: 1.36.8. Baseline: published 1.36.7 (commit e159963be11b9127363c5301841f814c43290c6b). The user confirmed the candidate working in game on 3 November 2026 and approved publication. The release retains the same client files.

Official release: https://github.com/Dingo-Shenanigans/ReSkate/releases/tag/v2.1.4

Tagged source: 919b04e. Installed and downloaded DLL SHA-256: `39195d824d775e5046480d66cb62dae0d709648f145414cb952ed2aa8afb99b3`.

## Layout and mutex audit

MSVC Release /MT, iterator debug level 0: 68 recorded fields/sizes. Both std::mutex and std::recursive_mutex remain 80 bytes. Overlay render_mutex stays at 0; Session.mutex stays at 783584; Remote.mutex stays at 72. The native lock thunk, lock implementation and unlock are individually guarded. The public status structure remains 16 bytes with its frame counter at +8.

| Field | 2.1.3 | 2.1.4 |
| --- | ---: | ---: |
| Overlay window | 720 | 728 |
| Overlay context | 2744 | 2752 |
| Overlay callbacks | 2752 | 2760 |
| Overlay model | 2800 | 2808 |
| Overlay menu | 28936 | 28944 |
| Remote record stride | 888 | 896 |

Other recorded model, debug, bindings, ImGui, party, session, peer and chat fields remain unchanged. Spectator tests use a physical 896-byte remote-record fixture. Native controller recording still copies 16 bytes plus a 4-byte word, verified in the new machine code; the 20-byte game-facing buffer is retained.

## Function addresses

| Role | 2.1.3 RVA | 2.1.4 RVA |
| --- | --- | --- |
| lock | `0x148a6f4` | `0x14c0354` |
| unlock | `0x148a720` | `0x14c0380` |
| fov | `0xd7f510` | `0xd9c050` |
| sampler | `0xd80cc0` | `0xd9d800` |
| queue | `0x9893a0` | `0x999de0` |
| console | `0x989080` | `0x999ac0` |
| controller | `0xfa4cc0` | `0xfbd2e0` |
| draw | `0x1021d00` | `0x104e0d0` |
| skater_page | `0x10229e0` | `0x104edb0` |
| begin_card | `0xfd0d40` | `0xffcbd0` |
| end_card | `0xfd6560` | `0x1002910` |
| field | `0xfd6650` | `0x1002a00` |
| note | `0xfd7a40` | `0x1003df0` |
| toggle_row | `0xfd9dc0` | `0x1006170` |
| mem_alloc | `0x127eb30` | `0x12b3fb0` |
| mem_free | `0x127ebe0` | `0x12b4060` |
| slider_float | `0xfd9690` | `0x1005a40` |
| msc_command | `0xa205f0` | `0xa2fa40` |
| msc_queue | `0x9895b0` | `0x999ff0` |
| ctor | `0xf88470` | `0xfb2bc0` |
| read | `0x989920` | `0x99a360` |
| msc_send | `0x9fce20` | `0xa0c430` |
| render | `0xade8d0` | `0xaed680` |
| getter | `0xf8b710` | `0xfb5ea0` |
| source_init | `0xd714a0` | `0xd8dfe0` |
| party_getter | `0xb03ab0` | `0xb16c90` |
| msc_session | `0x9d6e30` | `0x9e6620` |
| gamepad | `0xfa30c0` | `0xfbb3e0` |
| status | `0xf8be60` | `0xfb65f0` |
| context | `0x1279dc0` | `0x12af1a0` |
| remote_getter | `0xade6e0` | `0xaed490` |
| hid | `0xf98990` | `0xfc7300` |
| lock_impl | `0x148a744` | `0x14c03a4` |

## Globals and call sites

| Role | 2.1.4 RVA |
| --- | --- |
| state | `0x1d945a0` |
| party | `0x1be0e68` |
| session | `0x1be0470` |
| alloc | `0x1ba3f70` |
| userdata | `0x1e2dc20` |
| context | `0x1e2dc18` |
| free | `0x1ba3f78` |
| source_initialized | `0x1b99fc0` |
| source_base | `0x1b9a2d8` |
| source_transform | `0x1b9a2e8` |
| party_transform_return | `0xd9108f` |
| party_fov_return | `0xd910af` |
| hid_callers | `0xfc6269`, `0xfc6280`, `0xfc6b97` |
| remote | `0x1be0c70` |

## Verification

- Audited the actual installed DLL and official tagged source without loading the DLL into a process.
- Exact SHA-256 and export identity; 33 function prefixes; status/getter/lock/unlock relationships; source camera publication; overlay, ImGui, party, remote and session global pointers; widget dispatch; spectator camera/FOV return sites; all three shared HID call sites.
- Private runtime entry points and compiled globals are cross-checked against the reviewed inventory, not inferred from a common address delta.
- TLS-adjusted getter candidates are checked against the native 896-byte remote stride and independently compiled source layout. The HID handler is identified by its unique diagnostic string and direct callers.
- The underlying recursive-mutex implementation matches the prior implementation under instruction-aware matching; native thunk destination and implementation prefix are checked before activation.
- Dedicated runtime fixture tests reject 2.1.3/unreviewed identities, changed native entries and mutex/camera references. Existing input, spectator, camera, settings, loader and updater tests pass: 51/51.
- Game SHA-256 remains fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9. All 45 game code/vtable checks, 9 collision checks and the emote address audit pass. Existing game address declarations still match the tag.
- ImGui declarations/config remain identical to the companion. The PlayStation report helper is imported from the 2.1.4 tag and pinned in vendor/source-manifest.json. The stock handler remains in the HID call chain, preserving its new input-layer processing.

Evidence: tools/migration214/{contracts.json,data-contracts.json,layout.txt,audit-report.json,installed-audit.txt,game-offset-audit.txt,bail-audit.txt,emote-audit.txt,input-disassembly.txt}.

This is an offline compatibility audit and automated regression result. The user separately confirmed gameplay working; the checklist in VALIDATION_214.md records suggested coverage and does not claim every optional controller/feature was tested.
