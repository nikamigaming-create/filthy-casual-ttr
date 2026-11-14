# Controller profiles — community test candidate

The core remapper now accepts 70 stable source identifiers. Open the in-game
editor, select a channel and press a physical input; combinations and gestures
are supported. Profile 1–8 saves and mode banks still apply. Extra controls are
input sources, not extra destination actions. TTR contexts sharing one channel
remain grouped. Steam/system buttons are intentionally excluded.

| Runtime profile | Model | Validation |
|---|---|---|
| Touch / Quest | Licensed Touch model | Quest regression |
| Touch Plus | Licensed ringless model | Simulator; physical final check pending |
| Touch Pro | Licensed Pro model | Community test |
| Valve Index | Licensed Index model | Community test |
| HTC Vive | Licensed wand model | Community test |
| Windows Mixed Reality | Licensed WMR model | Community test |
| Pico 4 | Licensed Pico 4 model | Community test |
| Steam Frame | Original labelled 3D layout diagram | Community test |
| Khronos Simple | Generic model | Limited fallback inputs |

Controller identity comes from the active OpenXR interaction profile per hand,
not the headset. A runtime presenting Touch compatibility is reported as Touch;
that does not establish its actual hardware model. Unknown profiles keep native
game input and display no misleading Quest model. No promise covers every
controller, vendor extension or physical device.

## Finding button names

After a connected run, open `%LOCALAPPDATA%\FilthyCasualTTR\Controls\inputs.json`.
It lists stable INI tokens, runtime-readable names, paths, supported inputs, and
the two reported controllers. `catalog.json` lists destination actions/modes.
The desktop **Configure Controls.cmd** can load both files, or select a known
controller offline. Its **Button names for your INI** list is copyable.

Example Steam Frame input names: `LeftShoulder`, `RightShoulder`, `LeftDpadUp`,
`LeftDpadDown`, `LeftDpadLeft`, `LeftDpadRight`, `RightX`, `RightY`, `RightMenu`.
`LeftMenu` uses Frame's View button. Touch X/Y retain stable compatibility IDs;
runtime-readable names explain their physical equivalents. D-pad aliases refer
to the same physical buttons, not separate buttons.

## Starter INIs

`profiles/controllers/` provides opt-in snippets. Merge their keys into the
regular `NVSE/Plugins/SeatedControls.ini`; do not replace your full settings file.
They open the editor at startup and provide accessible wheel/editor shortcuts.
They are suggestions, not a guarantee of conflict-free native bindings. Native
actions using a combo's component buttons may still run before the chord fires.
Review the editor's conflicts and change the destinations you want to replace.
Only simple-controller devices lack enough distinct inputs for full TTR play.

No personal profiles, saves or Link configuration are replaced. Optional vendor
extensions are requested only when the current runtime advertises them. Existing
native controller-specific binding suggestions take priority; the adapter fills
missing native profiles by translating TTR's own Touch actions.

## Assets and evidence

Models come from the MIT WebXR Input Profiles assets v1.0.20, with source URLs,
hashes and license in controller-models-provenance.json. Steam Frame is an original
layout diagram based on Valve's published button arrangement, not a Valve mesh.
Controller profiles are checked against Khronos definitions and Valve's current
Frame integration (shoulder paths, rather than obsolete bumper names).

- https://github.com/immersive-web/webxr-input-profiles
- https://github.com/KhronosGroup/OpenXR-Docs/blob/main/specification/registry/xr.xml
- https://github.com/ValveSoftware/Unity/blob/main/com.valvesoftware.openxr.utils/Documentation~/index.md

The montage demonstrates the profile UI and simulator-driven inputs. It does
not certify physical tracking, controller ergonomics or every TTR interaction.
