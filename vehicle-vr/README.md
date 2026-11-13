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

Car: grip the physical wheel with either/both hands and rotate it; right trigger
throttle, left trigger brake, X reverse, B exit. Steering uses the original car's
physics. Vehicle camera, wheel and hands share its moving chassis frame.

Vertibird: left stick moves, right stick changes heading, A climbs, X descends,
B lands, Y toggles the original HUD, triggers operate aircraft weapons and stick
clicks choose weapons. Physical head movement stays free look. The original
aircraft script owns flight and landing.

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
