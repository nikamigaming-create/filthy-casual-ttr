# Install Filthy Casual TTR 1.8.0-beta.2

Start with TTR already working in VR through Mod Organizer 2, including its
xNVSE, JIP LN NVSE and other upstream requirements. This addon does not install
the original games or TTR. [Overview](../README.md) Â· [Controls](CONTROLS-EDITOR.md).

## Supported base

This build targets upstream **TTR 1.1.5** and accepts only TTWVR.dll SHA-256:

```text
E14567BC1C7D183130F902F9489B8CFFE6269DE06EEA0D79BF8D1D3098C595B1
```

PE timestamp: `6AF8509E`. Unknown builds refuse integration. Check the DLL from
the enabled base mod, rather than an unrelated copy outside MO2:

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath 'C:\path\to\TTWVR.dll'
```

The prior TTR 1.1.2 host needs [Filthy Casuals beta 1](https://github.com/nikamigaming-create/filthy-casual-ttr/releases/tag/v1.8.0-beta.1). Do not mix core/Vehicle binaries from different compatibility releases.

## Install or update

1. Quit the game. Back up your personal `SeatedControls.ini` when updating.
2. In MO2 choose **Install a new mod from an archive**, selecting
   `Filthy-Casual-TTR-1.8.0-beta.2.zip`.
3. Enable **Filthy Casual TTR** below the base TTR mod in the left pane, so the
   addon wins its own file conflicts. Leave base TTR and its dependencies enabled.
4. Preserve the personal INI, especially height and reload preferences.
5. Start your headset/runtime normally and use the same working TTR executable
   entry in MO2. No new launcher or global OpenXR registration is required.

The core ZIP contains no Pool or Vehicles DLL. They stay optional. The legacy
`Filthy Casual Reload Parts.esp` and original carrier are retained for existing
saves: keep the ESP if your saves already depend on it. Current native TTR owns
world-magazine creation and save records; the old addon drop hooks stay disabled.

## Files that must remain together

```text
NVSE/Plugins/SeatedControls.dll
NVSE/Plugins/SeatedControls.ini
NVSE/Plugins/SeatedControls/SeatedInput.dll
NVSE/Plugins/SeatedControls/SeatedInput.json
NVSE/Plugins/SeatedControls/Controls/
meshes/FilthyCasual/tutorial.nif
textures/FilthyCasual/Tutorial/
tools/controls-editor.html
```

The Controls directory includes the licensed Quest models and textures and
original RobCo artwork. Install the complete archive. Documentation, licenses
and a per-file SHA-256 manifest are included. No original game/TTR files, test
runtime, personal save or machine-specific launcher is included.

## First use

Hold the opposite separate thumbrest and double-tap the right thumbrest for
weapons or left for consumables. Touch a preview; it goes to the right hand.
The center cap clears the hand. B closes. A deliberate right-grip squeeze returns
holding behavior to TTR. Stimpaks use native trigger plus body stab. Ordinary food
in sticky mode can be brought to the mouth; A/X stows it without eating.

Tap left Menu to switch manual/automatic reload. Hold Menu for seated height:
A calibrates, right stick adjusts, B saves for next run, Y toggles routine hints.
Click the left stick there to open the RobCo controls editor. Fixed recovery:
physical left Menu + right stick click for one second.

Profiles are separate from game saves:
`%LOCALAPPDATA%\FilthyCasualTTR\Controls\profile-1.fcp` through `profile-8.fcp`.
The folder also contains `catalog.json` for offline editing. [Full instructions](CONTROLS-EDITOR.md).

Graphics, DLSS, Link/Air Link and texture settings are separate from this addon.
The installer does not alter them. [Validation](VALIDATION.md).

## Tested local bottle build

This local update includes `Filthy Casual Glass.esp` and original glass textures.
Enable that plugin alongside Filthy Casual TTR. It adds only a texture-set record
and depends on FalloutNV.esm; no third-party game assets are bundled.
Existing INI preferences are retained during the local upgrade. Gentle impacts
leave tracked empties intact. Hard collisions shatter them with native sound,
particles and a timed glass decal. Public 1.8.0-beta.2 does not yet include this update.
