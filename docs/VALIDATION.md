# Validation - 1.7.0-dev.1

Checked 13 November 2026 against TTWVR.dll SHA-256
`7EB72860D209D9B9BDD774607220EE63323367AA9FA525F6EB1B23184F30F91E`.
Unknown builds are refused. Windows x86, isolated MO2 profile, real game with
scripted OpenXR controller input. No personal saves or global runtime changes.

| Area | Result |
| --- | --- |
| Controls model | Strict profile parsing, analog preservation, gestures/chords, defaults, release barriers, external-edit protection and input loss passed |
| Modes and offline editor | Three advertised modes, inactive-mode editing, mode switching, absent-provider preservation, profile v3 migration and offline import/export passed |
| Real game controls | Pointer record/apply/cancel; native Pause/Back; saved profiles; fixed recovery; raw finger input separate from remapping passed |
| Driving | Grip/steer, throttle, brake, exit and return to on-foot mode passed |
| Flight | Native takeoff, movement, climb, landing and return to on-foot mode passed; native menu input preserved |
| Recovered magazines | 10mm pistol and assault carbine: native eject, ordinary grip pickup, native reinsertion, second pickup and return to native pouch passed |
| Ammo accounting | Pistol recovered 5 rounds; carbine recovered 23 rounds. Exact inventory debit/credit on both cycles; four handoff commits, zero failures; pouch return spawned no extra magazine |
| Presentation | Populated controls, driving, flight, weapons, aid and height panels captured in game; both eyes reviewed |
| Tutorial | Triple-Menu activation and native-world part outlines/card presence passed |

The recovered-magazine bridge does not spawn drops, write clip counts, fabricate
ammo or replace save handling. Native TTR owns those operations. It validates
reference identity, count, model, compatible held weapon and native hand mode,
then hands the recovered object back to native reload initialization.

Coverage is bounded: this is a development prerelease, not exhaustive headset
acceptance. Gravity-glove retrieval has the same held-reference entry path but
was not separately exercised in this round. Empty magazines, every weapon family,
arbitrary custom bindings, physical headset comfort and long-session/save-load
recovery have not all been certified. Vehicle visuals remain development work;
the vehicle provider is source-only here and requires separate native mods and
local asset preparation. Pool is optional and excluded from this core package.

Reproducible checks: `tests/check-controls.cmd`,
`node tests/controls-offline-unit.js`,
`tests/native-worldmag-recovery-build.cmd`, and
`tests/native-worldmag-recovery-static.py` against the supported owned DLL.
Private live runners: `tests/controls-vehicles.ps1`,
`tests/controls-vehicle-ready.ps1`, `tests/native-worldmag-runtime.ps1`.
Local accepted logs: controls-vehicles-run (PID 27516), controls-ready-run
(PID 30108), final-controls-recovery (PID 23276), robco-final-photos (PID 32216).
Runtime assertions, rather than the earlier overwritten screenshot-sidecar JSON,
are the magazine-count evidence. The sidecar naming collision is corrected.
