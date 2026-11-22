# Cars, Flight and Riding

Open **FILTHY → General → Features** to enable Driving, Flight or Riding, then restart Fallout.

## Requirements

- Cars require [Uhmatt Drivable Cars 8b](https://www.nexusmods.com/newvegas/mods/63890) and its dependencies.
- Flight requires [Vertibird Player Home 4.18 with TTW patch 4.18c](https://www.nexusmods.com/newvegas/mods/60923), [Pilot Inside](https://www.nexusmods.com/newvegas/mods/100171) and their dependencies, including kNVSE.
- Riding is included in the Filthy download.

## Physical wheel grips and fitted cockpit

Enable Filthy below the vehicle dependencies in MO2. Extract a copy of the Filthy ZIP and run this from its top folder with Python 3.11. Substitute your paths. Omit `--cars` or `--vertibird` if you only use one integration.

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r vehicle-vr/requirements-local.txt
.\.venv\Scripts\python.exe vehicle-vr/prepare-local.py --game-data "D:\Games\Fallout New Vegas\Data" --cars "D:\MO2\mods\Uhmatt Drivable Cars 8b" --vertibird "D:\MO2\mods\Vertibird Player Home 4.18" --output "D:\MO2\mods\Filthy Casual Vehicles - Local Assets"
```

Use a new, empty output folder and enable the resulting mod below Filthy. Keep the generated files local to your installation.

## Controls

- **Cars.** Right trigger accelerates. Left trigger brakes, then reverses. Release both to coast. Grip a prepared wheel to steer or use the left stick. Drawing a gun returns the triggers to the gun. B exits. Cars with prepared wheel rigs use arcade handling. To switch back, set `ExperimentalArcadeHandling=0` under `[Driving]` in `FilthyCasualVehicles.ini` and restart.
- **Vertibird.** Grip the right cyclic for pitch and bank. Grip the left collective for lift and turning. Releasing a handle frees that hand. B lands.
- **Bighorner.** Approach its flank and use both grips to mount. Keep one hand on it during the 30-second challenge. Afterwards, the left stick moves and the right stick turns. B dismounts.
- **Deathclaw.** Sneak up from behind, mount and hold on for 60 seconds. Dismounting restores hostility.
- **Seat height.** Calibrate with TTR’s seated zombie pose first. Open Filthy while seated in the vehicle to adjust the height for that vehicle type.
