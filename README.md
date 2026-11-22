# Filthy Casual TTR

Filthy adds seated controls, equipment profiles and a wrist flashlight to Kupo’s **Tale of Two Realities 1.1.7**. Install the release ZIP through Mod Organizer 2, enable it below TTR, then open **FILTHY → General → Features** on your Pip-Boy or Wrist-Boy. Fresh installs start with optional features unchecked.

[Download RC21](https://github.com/nikamigaming-create/filthy-casual-ttr/releases/tag/v1.8.0-beta.4-rc.21)

## Find your settings

| Device | Open FILTHY | Use a page |
| --- | --- | --- |
| Wrist-Boy | Raise your wrist. Hold right trigger and squeeze right grip to cycle past DATA to FILTHY. | Release the trigger. Right grip advances a page and left grip goes back. Use the right stick to browse or adjust, A to select and B to back out. Lower your wrist to use your hand in the world. |
| Full-size Pip-Boy | Point at FILTHY and press right trigger. You can also hold right trigger and use the grips to change category. | Use the pointer or right stick. A selects and B backs out. |

Both devices edit the same settings. FILTHY is beside Stats, Items and Data. Left-stick click still opens Wait.

## What’s in the download

| Feature | What you can do |
| --- | --- |
| Seated calibration | Sit on foot, extend both arms forward and hold both triggers for TTR’s zombie pose. Then adjust and save separate heights for normal play, cars, the Vertibird and riding. |
| Movement | Set where walking, running and forward sprinting begin. Adjust sprint speed, AP cost and the start notification. Auto-walk is optional. |
| Hands and controllers | Use finger gestures, grip sensitivity, hand smoothing and eight remapping profiles. Index finger input requires compatible hardware. |
| Equipment | Assign carried weapons and consumables to their wheels. Save three profiles per wheel and three holster layouts. Preview holstered items, lock marker positions and combine layouts into three kits. Hold left thumbrest and double-tap right for weapons. Reverse hands for consumables. |
| Weapons and aid | Switch manual or automatic reload, adjust the seated ammo pouch, use the reload tutorial, recover compatible magazines and bring supported items to your mouth. Supported drinks leave matching empty bottles. Hard impacts can break glass. |
| Wrist flashlight | The lamp sits on the glove and plugs into the underside of the Wrist-Boy. Aim with your wrist and adjust color, brightness, width and reach. It projects onto world surfaces and casts shadows through railings, foliage and around characters. The local dog-mesh setup below enables Cheyenne’s full-body shadow. |
| Interface and manual | Adjust supported dialogue and HUD placement and hints. Read Kupo’s twelve-page controls book in VR or open `docs/Kupos-Controls.pdf` in the download. |
| Optional pages | Voice Commands uses TTR’s recognizer when available. The Eyebot page requires its locally prepared content. Its in-world activation and trigger altitude controls still need headset verification. [Device guide](docs/NATIVE-SETTINGS.md) |
| Vehicles and riding | Enable Driving, Flight or Riding individually and restart. Cars support trigger acceleration, braking, reverse and coasting. Prepared rigs support physical wheel grips and arcade handling. The Vertibird uses its cyclic and collective. Bighorner and Deathclaw riding have separate mount challenges. [Vehicle setup](vehicle-vr/INSTALL.md) |

Pool, base building, troop command, physical crafting and repair, saddles, bags and mount armor are outside this release. The holster component runs in holsters-only mode. Flashlight shadows are local to the beam. The release does not enable scene-wide shadows or volumetric lighting.

## Install and update

1. Start with a working TTR 1.1.7 installation and its xNVSE and JIP LN dependencies. Check the supported DLL in the [install guide](docs/INSTALL.md).
2. Install the ZIP as one mod in MO2. When upgrading, back up personal INI edits and controller profiles first. Enable the new version and keep only one active core. Use your usual TTR launch entry.
3. For Cheyenne’s shadow, extract a copy of the ZIP and run the following from its top folder with Python 3. Use your own game and MO2 paths. Enable the generated local mod below Filthy.

   ```powershell
   py -3 tools/prepare-dog-shadow.py "D:\Games\Fallout New Vegas\Data" "D:\MO2\mods\Filthy Local Shadow Assets"
   ```

4. For physical wheels or the fitted cockpit, follow the [local vehicle import instructions](vehicle-vr/INSTALL.md). Cars require Uhmatt Drivable Cars 8b. Flight requires Vertibird Player Home 4.18, its TTW patch 4.18c, Pilot Inside and their dependencies. Local content is generated from your installed games and mods.

The flashlight was checked in the simulator with native rendering at 100% scale. Headset and upscaling checks are pending. [Changes since RC8](RELEASE-NOTES.md)

Thanks to Kupo for Tale of Two Realities and to the authors of the original vehicle mods. [Third-party credits and licenses](THIRD_PARTY.md)
