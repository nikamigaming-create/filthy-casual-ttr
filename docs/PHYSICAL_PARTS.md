# Physical reload parts - 1.5.1 beta.2

Manual reload parts become native world objects. Drop a spare, eject a magazine,
or use TTR's Save Ammo action and release it. Pick it up with the original grip
control or the original gravity gloves. Move your hand and release to throw.
Bring a recovered compatible magazine/cell to its gun's empty well, keep gripping,
align it and insert using TTR's original manual-reload movement. The exact rounds
remaining in that part are retained; an empty magazine stays empty.

To return a recovered part, bring it onto the floating ammo/magazine holder in
front-left of your torso, then **release grip**. This is TTR's native ammo-pouch
marker, not a separate bag. Default seated placement is 40 cm forward, 18 cm
left and 45 cm below the eyes. Override `PouchForwardCm`, `PouchSideCm` and
`PouchBelowEyesCm` in `[Weapons]` if needed; explicit existing values take priority.
X Take remains available. Gravity gloves use the original gesture: point at the
part, hold grip, then flick back. No extra pickup or throw button is introduced.

The material fix restores texture coordinates discarded after GPU upload from
an exactly matching installed source mesh. Geometry, normals, vertex colors and
topology must match; authored UV wrapping and runtime texture choices are retained.
World copies clear only the first-person shader bit and normalize texture paths.
Older cached parts repair into a new content-addressed file when loaded; original
cache files remain intact. Source game/TTR files are read locally, never modified
or bundled. A source mismatch leaves the release on TTR's native path.

Automatic reload and returning a spare to the pouch retain TTR's behavior.
This addon does not introduce another throw button or change native grip poses.

## Installation and owned assets

Install the Filthy Casual TTR ZIP through MO2 and enable
**Filthy Casual Reload Parts.esp** in its right pane. Preserve your existing INI
when updating. The ESP, tiny carrier mesh and texture are original addon assets.
The addon derives each part's model from your installed game/mod files into
`meshes/FilthyCasualLocal/Reload`. Keep that private cache with your MO2 profile;
never redistribute it. No original game or TTR assets are included in the ZIP.

Supported target: FalloutNV 1.4.0.525/x86, TTWVR SHA256
`C9D047D4199C0458281D341AC9DC438A7C928999E19CDAD6F327F8A439A7F9EB`.
Unknown builds are refused. JIP LN and the enabled carrier ESP are required.

Set `[Weapons] PhysicalReloadParts=0` and restart to disable physical drops and
recovered-part gun/pouch handoffs.
Existing appearance metadata is still restored while the addon remains installed.

## Verified coverage

| Representative weapon | Native pickup / throw | Exact ammo recovery |
|---|---|---|
| 10mm pistol | Passed | Passed |
| Assault Carbine | Passed | Passed, including X Take |
| Plasma Defender | Passed | Passed, including X Take |
| Laser Pistol | Passed | Passed |
| .357 Magnum Revolver, loose rounds | Passed | Passed |
| Single Shotgun, shell | Passed | Passed |

Loaded and empty ejection passed. Empty pickup/throw created no ammunition.
A loaded pistol retains its chambered round while its 11-round magazine becomes
a real object. Native Grip+A Save Ammo, extraction and release passed for the
pistol and rifle with exact 11-round and 23-round inventory transfers. Pouch return and automatic
reload passed; neither created an addon world-part transaction.

Normal save/load and leaving, unloading and returning to the original room
passed. The latter check exposed a real bug: JIP's per-reference model assignment
is cleared when the game destroys an unloaded reference. The addon now reapplies
that assignment to the returning object while preserving its native saved pose.
Pending release placement is distinguished from an already placed object, and
native early pickup takes ownership of its current position.

The fitted box collider follows the actual part dimensions with dynamic native
Havok physics. Local NIF round-trip, gravity, contact and settling checks passed.
The game owns ammo stacks, reference physics and save serialization; the addon
stores its model association and source weapon/ammo form references in JIP
metadata. JIP performs normal load-order remapping; the addon does not rewrite saves. User saves were not edited.

These are representative native paths, not a claim that every modded weapon was
individually tested. Dependency/cache failure guards were reviewed; their runtime
negative cases and physical-headset comfort remain unproven.

## Earlier physical-parts video

`Filthy-Casual-Physical-Parts-Mobile.mp4`: 42.1 seconds, 1080x1920, H.264, 30fps,
small download with fast-start playback. Assault Carbine and Plasma Defender
show physical drops, native pickup and throwing. Both-eye source recordings were
reviewed throughout at 1fps and more densely around interactions; both had zero
repeated capture frames. The phone edit uses the full right eye, original speed,
readable comic captions and gameplay-only wipes. It is silent.

Inputs were choreographed through the XR test runtime; gameplay and physics are
real. Wipes mark editorial cuts, including the rifle pickup after its approach
corrections. This is not an uninterrupted first-attempt or perfect human-grasp
claim. Full stereo masters, timing metadata and edit provenance remain private.
No QA/runtime labels appear in the promotional frames.

## Boundaries

Direct reinsertion accepts a tracked part from the same weapon base and selected
ammo, within the current capacity, held in TTR's reload hand with an empty well.
Different weapons/ammo remain ordinary world pickups; they can be returned to
inventory. Source identity is stored on newly created parts. Parts created by the
older 1.5.0 beta retain appearance/pickup/Take but lack reinsertion identity.
Loose rounds/shells can be recovered into the pouch; direct world-part handoff
uses the native installed-magazine/cell path. Native cocking and chamber rules
remain in effect. An empty magazine can coexist with a retained chambered round.

Contact eligibility is refreshed from a coherent main-thread scene publication,
bound to reference/hand/weapon, consumed once, and rejected after50ms. Under a
severe stall, handoff waits for a fresh contact. Native reload alignment and final
insertion still decide whether the magazine is seated.
Firing's spent-shell system is unchanged. Collision is a fitted box rather than
a concave replica. Pickups retain TTR's usual contact offset and hand pose.
Missing private models retain the normal donor appearance; keep your local cache.

Public v1.4.5 is unchanged; this is the local 1.5.1 beta candidate.

## Building the original carrier

The shipped carrier is generated solely from our code. For a source package:

```powershell
python -m pip install -r tools/requirements-reload-carrier.txt
python tools/build_reload_carrier.py
.\build.cmd
.\package.ps1
```

Generation writes the three original assets under `build/reload-parts/mod` and
updates `reload_carrier_data.hpp`. No installed game or downloaded mod is read.
## Recovered-magazine verification (1.5.1)

Native runtime checks passed for:
- Partial10mm magazine:5rounds plus the existing chambered round, native
  reinsertion, second ejection and pouch return. Exact total ammunition retained.
- Empty10mm magazine:source identity survives normal save/load; native insertion
  adds no ammunition, retaining only the game's pre-existing chambered round.
- Assault Carbine:23-round recovered magazine plus chamber; native insertion,
  firing, and pouch return.
- Plasma Defender:32-round cell, original gravity-glove pickup, native insertion,
  firing with its normal two-cell use, and pouch return.

The final packaged binary passed partial-pistol insertion/pouch and all three
metadata maps returned to size0 after consumption. Independent native-contract
and source reviews checked ownership, empty-count safety, disabled/menu guards,
pose publication and metadata cleanup. Neither user save files nor global XR
settings were edited. These are isolated native-runtime checks, not a new
physical-headset comfort certification.

New phone demo: `Filthy-Casual-Magazine-Recovery-Mobile.mp4`,1080x1920/30fps,
silent H.264 with fast-start. Reviewed successful rifle and plasma sequences,
normal speed, complete right eye, gameplay-only wipes and readable captions.
Both-eye masters and source frame metadata remain private. Selected spans have
zero repeated source frames. Fingers/body partly obscure the exact seating and
pouch socket; the recording shows normal approach, reload and empty-hand return.
