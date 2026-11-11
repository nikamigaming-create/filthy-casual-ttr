# Install Filthy Casual TTR

[Back to the controls and overview](../README.md)

Revision 1.5 beta adds physical manual-reload parts while keeping the existing
thumbrest selectors, Menu tap/hold controls and seated height. The phone video is
`outputs/Filthy-Casual-Physical-Parts-Mobile.mp4`. See
[physical parts, tested coverage and limits](PHYSICAL_PARTS.md).
## Before installing

Use a working Tale of Two Wastelands VR setup in Mod Organizer 2. Verify that your
usual MO2 executable already launches TTWVR in VR. The base setup must include
its required xNVSE and JIP LN NVSE dependencies.

The addon accepts only the following **TTWVR.dll SHA-256**:

```text
C9D047D4199C0458281D341AC9DC438A7C928999E19CDAD6F327F8A439A7F9EB
```

The associated PE timestamp is `6AF014A8`. The DLL hash is the compatibility
check; matching a filename or archive label is insufficient. Integration with
other TTWVR builds is refused.

To check your DLL, locate the `TTWVR.dll` supplied by the enabled base TTWVR mod
in MO2. In PowerShell, replace the example path with that file's actual path:

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath 'C:\path\to\your\TTWVR.dll'
```

Keep your current working base installation. This addon does not include TTWVR,
xNVSE, JIP LN NVSE, or game content.

## Install the ZIP

1. Close the game. Open the MO2 instance and profile you normally use for TTWVR.
2. Choose **Install a new mod from an archive** and select
   `Filthy-Casual-TTR-1.5.0-beta.zip` (or the supplied local-test ZIP).
3. Name the new mod **Filthy Casual TTR** and complete installation. The
   archive's data root contains `NVSE`; no enclosing mod folder needs to be selected.
4. Enable the new mod in MO2's left pane. Place it below the enabled base TTWVR
   mod so the addon has higher file priority. This is left-pane mod priority,
   not the ESP load order. Also enable **Filthy Casual Reload Parts.esp** in the
   right pane; it requires only FalloutNV.esm. Keep your existing personal INI
   when updating, especially seated height and reload preferences.
5. Start your headset/runtime as you normally would. Launch the same TTWVR
   executable entry through MO2 that worked before installing the addon.

The addon loads its companion OpenXR layer for this game process. Use your
existing working OpenXR runtime. No new launcher, environment-variable setup,
or system-wide OpenXR registration is part of installation.

The runtime files installed under the mod's data root are:

```text
Filthy Casual Reload Parts.esp
meshes/FilthyCasual/reload-carrier.nif
textures/FilthyCasual/reload-carrier.dds
NVSE/
  Plugins/
    SeatedControls.dll
    SeatedControls.ini
    SeatedControls/
      SeatedInput.dll
      SeatedInput.json
```

Documentation, original SVG artwork, license notices, and a package manifest are
also included. The test runtime and machine-specific development launchers are
excluded.

DLSS, anti-aliasing, render resolution, and reprojection are separate local
graphics preferences. This archive contains no TTWVR graphics INI, FalloutPrefs,
or SteamVR settings, and does not change those settings during normal play.

## First session

Double-tap the right thumbrest for weapons or the left thumbrest for consumables
while keeping the opposite thumb on its thumbrest. No stick click or wheel hold.
Keep the opposite thumbrest touched throughout the two taps. Releasing that
thumbrest cancels the sequence; resting both thumbs alone opens nothing. Tap left
Menu to change reload mode; hold it for one second to open seated settings.
A long hold never also changes reload. Left Menu replaces its native binding
during gameplay and passes through in native menus. Y Pause, face buttons,
triggers, grips and stick clicks retain native gameplay input without combo delay.
No addon wrist cuff. `[Controls] DoubleTapMs` sets the 450 ms gap between taps;
`HoldMs` adjusts only the Menu hold (1000 ms by default).
Touch a preview with either hand; the native item goes to the right hand.
Touch the large center bottle cap to clear your hand without consuming anything.
Press B to dismiss the selector without changing your selection.
Item use follows TTR's controls; there is no addon Y-consume override. Ordinary
held-food consumption is not yet verified. Stimpaks retain the native
right-trigger-and-body-stab action. Push the left stick fully to run.

For seated height, sit in your normal position and hold left Menu for one second.
A calibrates to a default target of 165 cm; right stick
up/down adjusts by 5 cm; X resets the offset; B saves and closes. A fresh package
starts with a zero height offset.

Selector popups default off. Inside the height panel, press Y to toggle them.
All height messages stay visible regardless of this setting.

Native gun actions remain available outside our panels. Tap left Menu
to switch automatic/manual reload. The mode message is shown and the choice is
saved. Manual mode places TTR's native pouch in seated reach. `[Weapons]
SeatedAmmoPouch=0` preserves native placement; the centimetre offsets are in
`SeatedControls.ini`. Squeeze the right grip deliberately to take over native
holding/release for a selected gun or consumable.

See [the full control reference](../README.md#reach-touch-equip) for Stimpak use,
other aid, page changes, holstering, and stowing.

## Update or remove

Before updating, back up your existing `NVSE/Plugins/SeatedControls.ini` if you
want to preserve its settings. Close the game, install the new archive in MO2,
and replace the previous addon mod. Keep only one copy enabled. Compare your
saved settings with the new defaults before restoring them.

If MO2 has placed a generated `SeatedControls.ini` in **Overwrite** or a separate
profile output mod, that copy may take priority over the packaged defaults.
Check those locations when a reset or update appears not to take effect.

If starting directly from MO2 chooses a different runtime than your working
launcher, set `[OpenXR] RuntimeJson` in `SeatedControls.ini` to that runtime's
absolute JSON path. This applies only to the game process; it does not change
Windows' registered runtime. An explicit launcher's `XR_RUNTIME_JSON` wins.

Earlier local builds were named **TTWVR - Seated Controls**. Update that existing
mod in place or disable it before enabling Filthy Casual TTR; never enable both.
The internal DLL and INI filenames remain `SeatedControls` for compatibility.

To disable the addon, close the game and untick **Filthy Casual TTR** in
MO2. Keep the base TTWVR mod enabled. If you reset a previous runtime height
offset when installing, restore your preferred adjustment separately.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| TTWVR does not launch without the addon | Restore the working base setup first; this package depends on it. |
| Addon controls do not appear | Confirm the addon is enabled in the active MO2 profile, all four runtime files are present, and you launched through that MO2 instance. |
| Compatibility is refused | Hash the active base `TTWVR.dll` and compare it with the supported SHA-256 above. |
| Touch does nothing immediately after opening | Move your hand away from the preview, then touch it again. Resting overlaps are deliberately ignored. |
| Touching with the left hand equips the right | This is expected: either hand selects; native item handoff always targets the right hand. |
| Ordinary aid does not consume on trigger | The addon adds no consume override. Ordinary held-food consumption is not yet verified; Stimpaks use the native trigger-and-body-stab action. |
| Seated height is too high | Check for a second height adjustment in your runtime, then use X in the addon height panel to reset and recalibrate. |

If reporting a problem, include the addon version, your TTWVR DLL hash, runtime,
and the action that triggered it. Describe whether you observed it in a simulator
or a physical headset.
