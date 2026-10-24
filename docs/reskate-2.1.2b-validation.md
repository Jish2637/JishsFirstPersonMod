# Updated ReSkate 2.1.2 binary compatibility

The current rebuild targets the replacement DLL supplied on 24 October 2026:
SHA256 `96112d915c181b696b5cb8ed17b62a74d09ca2900c4383a4500c1fa88bf76e43`.
It reports the same 2.1.2 version as the previous DLL, so identity is checked by
hash and guarded native entry points. This build replaces the previous 2.1.2 mapping.

Remapped 32 native functions and the camera, overlay, spectator, HID and MSC
pointer/call sites. Updated all MSC Session fields after the new plugin fields
shifted the peer array and later fields by 56 bytes. Overlay, model, menu,
spectator record and controller layouts remain unchanged. Camera behavior was
not changed.

Source reference: Dingo-Shenanigans/ReSkate commit
`55a7342dc6e7e7be724a17cfc5733b7e265de8e1`. The compiled layout probe output,
source hashes and reviewed function/data mappings are in `tools/migration212b/`.

Offline verification: `tools/audit_212b.py <ReSkate.dll>` checks the exact hash,
32 function prefixes, compiled native/UI/MSC guards, pointer references, HID
call sites, spectator FOV call, source camera setter and sender Session offsets.
The source layout probe was rebuilt against the updated upstream revision.

In-game validation is still required for launch/menu navigation, bail collision
recovery, controller input, throwdown spectating/free look and MSC tag updates.

Release build succeeded and the full regression suite passed 48/48. The offline
binary audit passed against the supplied DLL. Built companion SHA256:
`db451c5df792520746883346319d69de64185ab4f4ba7903839a343e124fb699`.
