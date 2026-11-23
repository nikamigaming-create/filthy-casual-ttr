# Filthy Casual TTR 1.8.0-beta.4-rc.23

- Vehicle recognition uses the installed Uhmatt 8b vehicle families and covers paint variants. Simulator testing confirmed the driver's torso and hands in Chet's red Jeep and black car, including a reload while seated in the Jeep.
- The pickup, Sport Coupe, Van, Firetruck, and compatible Flatbed use their original wheels for grip steering. Both hands passed independent grab-and-turn checks in the simulator. The Roadster retains physical steering. The red Jeep and the black car use the left stick.
- The full-size Pip-Boy prepares FILTHY before you open it. FILTHY is visible on the settings page and on both supported English Wrist-Boy case textures.

## Known issues

- On the full-size Pip-Boy, advancing from Data into FILTHY can briefly pause. The headset report found that entering from the other direction avoided the pause.
- The Fusion Flea and Shopping Cart can obscure the driver. The standard Motorcycle crashed on boarding in both RC22 and RC23 simulator tests.

## Filthy Casual TTR 1.8.0-beta.4-rc.22

- Installs as one MO2 mod with the original dependencies listed on the [front page](README.md#vehicle-downloads).
- FILTHY opens directly to General on both devices. Select Wrist flashlight to adjust the beam.
- Corrects the wrist flashlight endpoint across VR render resolutions.
- Includes the native scripts required by the vehicle module.
- Uses the stock Roadster’s wheel and tyre pivots for physical steering and arcade handling.
- Includes the full-size FILTHY tab and preserves the native Data menu during category switches.

## Changes since public RC8

- **Settings moved onto both devices.** FILTHY has its own fourth category on the wrist and full-size Pip-Boy. The same settings follow you between them. The wrist returns hand input to TTR when lowered.
- **Walking, running and sprinting have adjustable zones.** Set each threshold on the device.
- **The flashlight projects onto the world.** The local shadow pass handles grass, trees, saloon glass, bridge railings and signs.
- **Cars use Uhmatt Drivable Cars 8b.** Grip the stock Roadster rim to steer, use the triggers to accelerate or brake, and release them to coast.

The controls book, core wheels, remapping, seated calibration and bottle effects carry forward from RC8.

The flashlight endpoint correction was confirmed on a Quest 3 at 100% DLSS Performance.
