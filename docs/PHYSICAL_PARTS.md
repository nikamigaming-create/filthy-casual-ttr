# Physical reload parts - 1.5 beta

Manual reload parts become native world objects. Drop a spare, eject a magazine,
or use TTR's Save Ammo action and release it. Pick it up with the original grip
control, move your hand, and release to throw. X Take returns the ammunition to
inventory for use through the original pouch and reload path.

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

Set `[Weapons] PhysicalReloadParts=0` and restart to disable new physical drops.
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
stores only its model association in JIP metadata. User saves were not edited.

These are representative native paths, not a claim that every modded weapon was
individually tested. Dependency/cache failure guards were reviewed; their runtime
negative cases and physical-headset comfort remain unproven.

## Video

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

Recovered world magazines return their rounds through native Take. Directly
reinserting that world reference as a TTR-held magazine is not implemented.
Firing's spent-shell system is unchanged. Collision is a fitted box rather than
a concave replica. Pickups retain TTR's usual contact offset and hand pose.
Missing private models retain the normal donor appearance; keep your local cache.

Public v1.4.5 is unchanged; this is the local 1.5 beta candidate.

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