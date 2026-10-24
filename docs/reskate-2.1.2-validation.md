# ReSkate 2.1.2 compatibility validation

This rebuild targets the supplied `ReSkate.dll`, version 2.1.2, SHA256
`60be9605476fdd76f3cc2e4b130dda57d645bf3e9d0246c73137884b70cd7fce`.
Other DLL identities fail closed. Historical profiles remain for regression coverage.

Reference source: Dingo-Shenanigans/ReSkate commit
`e2d12a701d38cd74ed1c5eed4f48afce53c12a2e`.

Updated native entry points and guards, overlay callbacks/model/menu offsets,
model revision and status fields, controller/HID input, camera source pointers,
spectator callbacks, and MSC session/peer layouts. The removed `drone_follow`
field is no longer read on 2.1.2. Existing bail/head rotation and collision
behavior is preserved.

`tools/audit_212.py <ReSkate.dll>` checks identity, 32 function prefixes,
header guards, global pointer references, HID call sites and spectator FOV call.
The reviewed function/data manifests are in `tools/migration212/`.
`tools/layout212/` builds a source layout probe using `-DUPSTREAM=<source>`;
its verified output is saved in `tools/migration212/layout.txt`.
The historical `native211`/`embedded211` internal names now hold this build's
2.1.2 mappings; they do not imply runtime support for the previous binary.

Gameplay validation remains required: launch/menu navigation, local camera,
hard-impact collision recovery, controller recording, throwdown FP spectating
with modifier freelook, and MSC tag updates on the server.

Validation: Release build succeeded with MSVC x64. Offline binary audit passed.
The full CTest regression suite passed 48/48 after updating runtime and updater
compatibility guards, including rejection of 2.1.1 update metadata.

Built companion SHA256:
`e8d96f647c6671aefa6c4d33ef999d5245f5915089b25a00cbaac94c6eabffcd`.
