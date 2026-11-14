# Optional Filthy Casual Vehicles 1.8.0 beta

Install the core only if you want selectors, seated settings and the remapper.
Vehicles is separate; Pool is not enabled or included.

## Required downloads (not bundled)

- Working TTR 1.1.2, exact DLL SHA-256 `7EB72860D209D9B9BDD774607220EE63323367AA9FA525F6EB1B23184F30F91E`.
- Cars: [Uhmatt Drivable Cars 8b](https://www.nexusmods.com/newvegas/mods/63890).
- Flight: [Vertibird Player Home 4.18 + TTW patch 4.18c](https://www.nexusmods.com/newvegas/mods/60923) and [Pilot Inside](https://www.nexusmods.com/newvegas/mods/100171).
- kNVSE and the original mods' requirements. Python 3.11 for local preparation.

Install the Vehicles ZIP in MO2 below the original dependency mods. Before driving/flying,
extract another copy of this ZIP to a work folder and prepare your private mesh override:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r vehicle-vr/requirements-local.txt
.\.venv\Scripts\python.exe vehicle-vr/prepare-local.py --game-data "D:\Games\Fallout New Vegas\Data" --cars "D:\MO2\mods\Uhmatt Drivable Cars 8b" --vertibird "D:\MO2\mods\Vertibird Player Home 4.18" --output "D:\MO2\mods\Filthy Casual Vehicles - Local Assets"
```

Use your actual folders. Omit `--cars` or `--vertibird` if you do not use that integration.
Choose a new empty output folder; existing mods are never overwritten. Enable the generated
Local Assets mod below Vehicles. The helper reads your owned files locally and produces the
wheel attachment and fitted classic cockpit. No Fallout 4 installation is needed.
Never distribute the generated Local Assets folder; it contains dependency/game-derived data.

## Play

- Car: left stick forward/reverse; right stick left/right. Grip either side of the wheel independently. B exits.
- Flight: right stick moves; left stick turns. A climbs, X descends, B lands. With a gun drawn, A/X and triggers stay with the gun.
- Free hands can select items, use native weapons and reload. The core's selectors travel with the cabin.
- Hold Menu for settings. Ground/car/flight/creature heights save independently.
- Bighorner: both grips at its flank to mount, keep one hand gripping the body through the 30-second challenge, then left stick moves/right stick turns. B dismounts.
- Deathclaw: sneak behind it while undetected, mount and hold on for 60 seconds. Dismount restores hostility.
- ChallengeSeconds is configurable per species in FilthyCasualVehicles.ini; zero unlocks steering after a valid mount.

Home rooms/facilities come from Vertibird Player Home. Visible companion seating and interactive cockpit instruments are not promised by this beta.
