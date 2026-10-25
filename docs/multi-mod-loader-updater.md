# Bundled multi-mod loader and First Person updater

## Implementation

Imported the loader and portable mod-list tests from the user-supplied UniversalModLoader-v2-source.zip (GPL-3.0). Source layout adapted to src/proxy_loader.cpp and src/loader/mod_list.h; the executable identity check and Windows x64 export table remain unchanged.

The loader synchronizes scripts/Custom/dinput8_mods.txt, honors order/comments/disabled entries, skips duplicates and dinput8 proxy DLLs, waits for the verified game/ReSkate overlay, then loads each enabled mod outside DllMain. Existing modules are skipped. Each mod owns its launcher switch and initialization. Optional dinput8_chain.dll forwarding is retained; otherwise DirectInput goes to System32.

The updater recognizes the generated bundled loader digest, the pinned multi-mod test distribution digest, the supplied original v2 loader digest and the existing legacy First Person proxy digest. Unknown loaders remain rejected. Both the downloader and separate installer use the same shared exact ReSkate 2.1.2 identity constant. The installer previously retained an obsolete runtime digest even though the downloader had been updated.

For a recognized multi-mod loader, the verified archive is unpacked normally but dinput8.dll is excluded from the installation transaction. First Person, its Lua bootstrap, bundled preset, manifest and updater still update. Other mods, load-order list, optional chained proxy and personal settings are outside the installer target list. Loader ownership is still checked, even in preserve mode. Transaction backup, rollback and refusal of locked/read-only files remain active.

Loader preservation prevents a First Person update from replacing multi-mod support with a legacy single-mod proxy. When publishing future builds, retain existing pinned multi-loader hashes in src/loader_update_policy.h and append any newly distributed loader hash. Loader upgrades themselves are manual; First Person updates preserve the supported loader.

Client packaging remains within the updater's strict allowlist. Research docs and developer notes are in the source ZIP rather than adding unsupported paths to an auto-update archive.

## Validation

Regression coverage includes mod discovery/order/disabled entries, stale entry removal and duplicate/path rejection; all six proxy names/ordinals and genuine Windows DirectInput forwarding; refusal to load First Person in unrelated hosts; preserved-loader update with another mod, exact mod-list contents and chained proxy; preserved-loader rollback; changed/unknown loader refusal; shared runtime identity; download/staging and the existing integration suite.

In-game multi-mod loading and an actual install-after-exit update require confirmation. The user already runs the supplied v2 loader successfully. No game installation, GitHub push or release publication was performed here.

## Local build result

Release build and all 51 regression/integration tests passed (none skipped), including real Windows DirectInput discovery. The updated updater state/download/staging tests also passed in an isolated directory containing a copy of the user's actual UniversalModLoader v2 binary; its digest remained unchanged. Transfers used deterministic fixture data, not a live published release. Real post-exit installation was not armed against the user's game.

- dinput8.dll: SHA256 `3117aad80316d4c8f3e9120b979d5a3bd26a6d47542f77d53fb16b0239d6ba1d`.
- JishsFirstPersonMod.dll: SHA256 `2f549c3a5b317efb9fcedf1df0a0806762ca634c9a420f5eaba7129a84db6383`.
- FirstPersonUpdater.exe: SHA256 `824608a4b2e9054d8ffd75aa0f8f8ab1874d635a49e65a4fc0b59075be3633b4`.
