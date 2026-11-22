# Install Filthy Casual TTR

## Supported base

Start with Kupo’s TTR 1.1.7 running in VR through MO2, including its xNVSE and JIP LN requirements. The native adapters check this SHA-256 for the winning TTWVR.dll.

```text
119A085A5B0101F0618B8E076A421B4B89E30D72D9B6EAE874F4E5AF7BD3FB27
```

## Install or upgrade

1. Quit Fallout. Back up any personalized INIs or remapping profiles.
2. Install the Filthy Casual TTR RC21 ZIP as a new mod in MO2. Disable the older Filthy core and its test overrides, then enable RC21 below TTR. Keep the Controls Book and Glass ESPs enabled to use those features. Keep the Reload Parts ESP if an existing save depends on it.
3. Launch your usual TTR entry. Open **FILTHY → General → Features** and choose what to enable. Vehicle selections take effect after restarting. Existing feature choices persist when their settings are retained.
4. For seated play, do TTR’s seated zombie pose first. After calibration, adjust the current seat height in Filthy and save it.

## Local assets

- **Cheyenne.** Follow the [dog shadow setup](../README.md#install-and-update). The importer checks the original archive mesh and the generated result. It can be rerun against the same output.
- **Cars and flight.** Install the [original dependencies](../vehicle-vr/INSTALL.md) and run the local rig importer for physical wheel grips and the fitted cockpit.
- **Eyebot.** The [device guide](NATIVE-SETTINGS.md#optional-eyebot-page) explains how to generate and enable the Eyebot ESP and assets. The Equipment and General pages work independently.
- **Wrist appearance.** To prepare the optional English label for the physical Wrist-Boy button, install the Python requirements from `vehicle-vr/requirements-local.txt`, then run the following from your extracted ZIP. Use `xbox` in both places for the controller layout, or `pc` in both places for the PC layout. Enable the generated mod below Filthy.

  ```powershell
  python tools/build_filthy_case_paint.py --archive "D:\Games\Fallout New Vegas\Data\Fallout - Textures2.bsa" --layout xbox --local-dds "D:\MO2\mods\Filthy Wrist Label\textures\pipboy3000\xbox\english\PipBoyArm01.dds"
  ```

Kupo’s normal controls handle play outside the device. Use the [README](../README.md) for wheel gestures and category navigation.
