# Native emote aiming investigation — 25 October 2026

## Requested behaviour

Add upper-body camera aiming during native emotes such as pointing. Scope: local
first person on foot, right-stick look enabled, Camera only (no character lock).
Keep existing Native head movement independent: it can run during ordinary
walking; upper-body aiming must follow the native emote lifecycle only. The user
confirmed that an emote takes over normal head look in third person as well.
An opt-in prototype is now implemented locally. It has not been installed, committed or published. See the implementation note below.

## Binary and source baseline

- Workspace: `release-1.36.2-source`, `main`, released mod 1.36.3; no production source edits in this investigation.
- Game: `C:/Games/ReSkate/Skate.exe`, SHA256 `fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.
- Preferred EXE image base: `0x140000000`. All addresses below are RVAs, not absolute pointers.
- Reviewed ReSkate DLL SHA256: `96112d915c181b696b5cb8ed17b62a74d09ca2900c4383a4500c1fa88bf76e43`.
- Reference source: `build/upstream212`, revision `55a7342dc6e7e7be724a17cfc5733b7e265de8e1`.
- Exact reviewed instruction prefixes: `tools/emote_aim/findings.json`.

## Verified offline findings

### A native gesture signal, separate from physics look-at

The native parameter descriptors at `0x6121c80` describe these five parameters:
WorldPosition (input vector), MaxDistance (input float), ReturnValue (output bool),
PlayingGesture (output uint32), GesturingPlayerWorldTransform (output matrix).
Native record `0x7f331e0` points to implementation `0x8356b0` and wrapper
`0x5968ea0`. Decode record pointers as full 64-bit VAs, then subtract the image base.

The implementation iterates players, **skips local players** (`player+0x45 != 0`),
gets the skater entity, finds component type `0x72804e0`, calls `0xf94c40`, checks
distance, and calls `0xf90410` to write PlayingGesture. Therefore this is strong
proof of the remote/native-query semantics, **not yet proof of the local signal**.

- `0xf94c40`: `cmp dword ptr [rcx+0x550],0; setne al; ret`.
- `0xf90410`: `mov eax,dword ptr [rcx+0x550]; ret`.
- Component vtable `0x6171ae0`; slot zero points to `0xf906f0`, which returns type `0x72804e0`.
- Construction at `0xf8832e` installs that vtable; `0xf883a2` clears the uint32 at `+0x550`.
- Allocation size is `0x5a0` (construction path near `0xf88302`).

**Important:** this is a different component from the physics/animation component
cached at `entity+0x628` (vtable `0x6170410`). Do NOT read `cached_component+0x550`
and assume it is a gesture state. Resolve the new component from the entity's
owned factory collection (`entity+0x70`, count byte +8, slots +0x20 at stride0x20)
and verify its type/backlink. The existing `read_native_component` template is
suitable once this new type contract is validated.

The field is a uint32 gesture identifier/nonzero signal; it is **not proven to be**
the enum `ReplicatedSkaterGesturePhase` (None/Into/Loop/Exit). Those strings exist
in the EXE, but their local field and end-of-emote semantics are unmapped.

### A script-native camera transform getter

Native parameter `CameraTransform` at `0x6132d10` is an output matrix.
Native record `0x7f52d10` has zero inputs/one output and points to:

- Native function `0x85eac0`.
- Wrapper **`0x59aa230`**, which tail-jumps directly to `0x85eac0`.
  Do not confuse it with `0x596a230`, an unrelated address.

The native moves output RCX to RDX, clears R8/R9, obtains the global camera service
at `0x75cbc48`, and tail-calls its vtable slot `+0x60`. This is a candidate for
feeding the displayed FP camera direction into the native authored emote graph.
**Not yet verified:** that graph actually consumes this getter, which camera
variant the service returns, and the complete return-value ABI. Do not install a
hook merely because the parameter is named CameraTransform.

Other CameraTransform parameter groups at `0x612f850` / `0x612f930` take SessionId
and Time; implementations `0x85b490` / `0x85b610` are replay/session operations.
Those are false leads for emote aiming.

### Related gesture-component paths (semantics still incomplete)

- `0xf9cf00`: copies a matrix into component+0x500/+0x520, stores float +0x4f4,
  sets byte +0x4f0. Matrix input setter, **not proven to be local emote aim**.
- `0xfa17d0` consumes that matrix and a distinct controller branch.
- `0xfa2910` uses the complementary branch and calls `0xf8e520`.
- `0xf974f0` calls `0xfa2910`, `0xf8bd70` (component+0x558) and `0xfa2260`.
- `0xfa2260` publishes property values via the native setter `0x4654e10`.
- `0xf8e520` includes pose-transfer/retargeting work. Do not label it an emote-aim
  function without proving the relevant subpath.

## False leads and limits of the live captures

Existing Native head movement only changes stick-input passthrough in
`src/native_head_look.h` / `src/stick_runtime.cpp`. Enabling it is not a verified
solution for emote aiming. The saved game settings observed during investigation
had `stick_native_head_look=0`; do not assume every captured frame tested it enabled.

The physics look-at object `core+0x3e0` has size0x60 and a physics-context backlink
at +0. `0x474db80` publishes its float fields to animation inputs, and `0x4755580`
/ `0x47476a0` compute its outputs. Those outputs stayed zero/unchanged in captured
idle/point samples. This is insufficient to drive or detect the requested emote.
`OB_LookAtX/Y` strings exist, but their names alone do not prove emote semantics.

Do NOT infer emote state from physics bit26 or a changing float/low byte. Observed
correlations have no verified producer/consumer contract. Do NOT use only LB/D-pad
presses or a timer: wheel selection, holds, interruption and exits must work.

## Captures and process visibility

All reads used OpenProcess with `0x1010` (VM_READ + limited query), ReadProcessMemory
and module enumeration. No process writes, injections or input automation occurred.
The sandboxed process list hid the running game. The unsandboxed read-only list
worked; the user confirmed subsequent process-ID changes were manual restarts.
**Never reuse captured addresses or PIDs after a restart.**

The owner was verified through local-player flags, entity/player backlinks,
component/core/rig backlinks, the supported vtables and (in later checks) membership
in the local player manager. User performed both camera-mode point tests and a
second labelled idle/held-point comparison. No further user tests are requested;
continue offline analysis per the user's instruction.

Raw data stays local in the ignored `build/` directory; do not publish memory dumps:

- `emote-live-trace.jsonl`: 1,768 samples over90s; contains both native FP off/on.
- `emote-state-idle.json`: 10 labelled idle snapshots.
- `emote-state-point.json`: 300 snapshots during the user-held point test.
- `emote-live-owner.json`: obsolete runtime pointers; must re-resolve if ever used.
- `emote_*.py`, `emote-*.txt`: exploratory scripts/disassemblies.
- `gamequick.py`: fast offline PE/Capstone reader, no game interaction.

Earlier raw snapshots covered physics controllers, not the newly identified gesture
component. Consequently they cannot yet validate its +0x550 local lifecycle.

## Efficient next steps

1. Use the offline native parameter/record chain above; do not restart broad string
   searches or request the same user captures again.
2. Trace the gesture identifier's producer and the authored local emote direction
   consumer. Preserve the distinction between local and remote paths.
3. Prove whether the authored emote uses native camera getter `0x85eac0`. If so,
   consider a narrow, owned hook returning the final FP camera orientation only
   under positively verified local emote state. Preserve the original position,
   return ABI and all unrelated camera consumers. Otherwise hook the actual aim
   producer; do not rotate arbitrary bones as a substitute.
4. Only then implement an independent option alongside Native head movement,
   with saved settings/presets and fresh owner/focus/gameplay checks. Suspend on
   bail/recovery, boarding, spectator view, third person, device loss and stale state.
5. Meaningful checks: emote entry/hold/exit/cancel, wheel-selected emotes, camera
   changes, pointing up/down/sideways, Native head movement on/off, and existing
   multiplayer pose transmission. Build/test locally; do not push or publish until
   explicitly requested.

## EXE inspection caveats

- MSVC unwind entries can split one function into multiple fragments. Follow
  UNW_FLAG_CHAININFO rather than treating every .pdata BeginAddress as a new function.
  Example: fragment0xfa18c2 chains to actual entry0xfa17d0.
- Disassembling one huge code range across embedded jump tables can lose alignment.
  Verify candidate instructions from reviewed entries/call targets or raw operand
  references; do not conclude a field has no writes from one linear sweep.
- Native record fields at +0x20/+0x28 are 64-bit function/wrapper VAs; parameter
  descriptor pointer is +0x38. Decode rather than transcribing similar hex addresses.
- A PE reader must bound reads by section SizeOfRawData. Reading virtual .bss bytes
  from the following on-disk section fabricates data. Current reviewed instruction
  and native-record addresses are within actual raw bytes.

## Experimental implementation (local, 25 October 2026)

Default-off `stick_native_emote_aim` is exposed alongside Native head movement.
It reuses StickSettings command, settings and preset handling.
`src/emote_aim_runtime.cpp` chains the reviewed getter at0x85eac0, preserves its
return register and output position/padding, and substitutes only orientation
from a fresh displayed FP frame. It resolves the separate gesture component from
the owned collection and requires a nonzero ID. Current owner, normal ground
state, focus, overlay, FP ownership, stick mode, frame age under100ms and engine
thread are checked. Failed guards leave native output untouched. No skeleton or
gesture state is written; head-input passthrough remains unchanged.

This is a hypothesis test: the local identifier lifecycle and authored emote's
use of this getter remain unverified. Five-second counters identify getter calls,
nonzero native gesture samples and actual replacements. No gesture samples means
investigate local state; overrides without a visible effect mean another camera
source may drive the emote. Do not remove gates to force a visible result.

Tests cover option/command/preset persistence, menu routing, frame expiry,
thread/owner replacement, orientation-only copying, foreign/duplicate/missing
components and collection replacement. In-game response remains untested.

Test1 DLL SHA256: `e0bb06e68121d66e002bd39221b0b0c7299b9262f757f89c7e27fd44bd0ee66f`. Release build succeeded; offline EXE audit passed.
The final regression suite passed49/49. No game installation or Git push was performed.

## User test feedback, 25 October 2026

Pointing follows the first-person view when stationary. Running sometimes returns pointing to forward, then stopping restores view-relative pointing. This confirms a visible stationary result, not complete locomotion coverage. No explicit speed gate was found in the adapter; native running animation camera consumption remains unverified. Preserve the verified owner, gesture, camera and movement gates.
