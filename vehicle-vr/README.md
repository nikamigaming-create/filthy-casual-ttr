# Filthy Casual Vehicles

Optional development integration for TTR. The core Filthy Casuals mod and Pool
are independent; neither is required to drive using the default controls.

Target TTWVR.dll SHA-256:
`7EB72860D209D9B9BDD774607220EE63323367AA9FA525F6EB1B23184F30F91E`.
Unknown builds refuse hooks. Build x86 with `vehicle-vr\build.cmd`.

## Native dependencies

Download separately: [Uhmatt Drivable Cars 8b](https://www.nexusmods.com/newvegas/mods/63890),
[Vertibird Player Home 4.18 and TTW patch 4.18c](https://www.nexusmods.com/newvegas/mods/60923),
and [Pilot Inside](https://www.nexusmods.com/newvegas/mods/100171).
The supported car is MkT Roadster. This repository contains no original dependency
or game models/textures. It is not a turnkey vehicle installer.

## Controls

Car: left-stick up/down drives forward/reverse; right-stick left/right steers.
The other two stick axes do not control the car. Opposite
input brakes before changing direction; neutral slows to a stop. Either or both
hands can grip the physical rim independently by squeezing at it. A squeeze
that started elsewhere stays native, so carrying a magazine past a control
cannot steal the hand or drop the magazine. Native gun and item triggers are
available while driving; B exits. The addon restores only the combat control
that the Roadster boarding script disables, retaining its seat and other locks.
With no hands on the wheel while moving, a visual knee-steering pose blends in;
regripping or stopping blends it out. This is an animation, not knee tracking.

Vertibird: right stick moves, left stick changes heading, A climbs, X descends,
B lands, Y toggles the original HUD. Grip the visible control with either/both
hands for physical steering; release frees that hand. Thumbstick input takes priority while gripping. With a gun drawn, both
triggers and A/X stay native for firing, slides/bolts and reloading; aircraft
climb/descent on A/X resumes when the gun is holstered. Item triggers also stay native. The flight script's repeated combat lock is narrowly adapted so it no longer holsters your gun every frame; other scripts retain their normal control locks. The local cockpit places the pilot in the left seat,
with an upright control made from owned Fallout motorcycle-handle geometry.
Physical head movement stays free look. Both expression and condition input queries reach the original aircraft script,
which owns flight and landing. The duplicated center-console pedestal is removed
from the local fitted cockpit; the original panel and control handles remain.

`FC_ControlsProviderV1` advertises Driving and Flight to the optional Filthy
Casuals editor. Both modes are configurable while on foot and offline. Boarding
and exit select their saved profiles automatically. [Provider contract](../docs/CONTROLS-PROVIDERS.md).

## Locally owned classic cockpit

`classic-vertibird/prepare_local.py` extracts the user's New Vegas hull, Enclave
chair and original textures. `fit_cockpit.py` fits dependency cabin/console parts
to the native rig. Derived models/textures remain private and are never packaged.
The selected classic path needs no Fallout 4. The old `owned-vertibird` scripts
are historical import experiments, not the current selected path.

The native seated idle supplies pelvis/legs; arms and head stay on the tracked
path. The player is not hidden or shrunk. Roadster and aircraft have separate
seat/eye fits. kNVSE is required for the native seated-animation path.

## Validation and remaining limits

Current isolated simulator checks passed two-hand rim contact, physical steering,
throttle movement, braking/exit, aircraft forward motion, climb, completed landing,
pilot survival, and on-foot control restoration. Provider modes switch with those
states. Both-eye captures exist; physical headset comfort is not yet certified.

Portable import/install UX, full canopy appearance review, visible companion
seating and interactive cockpit instruments remain development work. This module
is optional and separate from the core downloadable seated-mod candidate.

The weapon tutorial's reading card stays in the cabin while travelling; part
highlights update with the completed native weapon pose. On foot the card
remains fixed in its reading zone. With Filthy Casuals enabled, weapon and
consumable selectors stay fixed in the cabin during travel. Driving sticks do
not dismiss them. Touching a preview selects it into the native right hand;
B closes a selector without also exiting the car. Default shortcut: keep the
opposite thumb rest touched and double-tap the right rest for weapons or the
left rest for consumables. Both actions also appear in each vehicle's control
mapping options. On foot, selectors retain their original world anchor.
