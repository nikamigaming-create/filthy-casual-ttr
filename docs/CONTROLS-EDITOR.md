# RobCo controls editor

Filthy Casuals can remap TTR's current controller bindings while the game is
running. The panel shows the actual MIT-licensed Quest Touch Plus 3D models used
by MGS5VR, with live button/touch highlights, on an original RobCo-style terminal.
The controllers turn to expose a selected grip or trigger. No game/vendor model
is extracted from the user's game for this panel.

## Open and operate

- Hold left Menu for the existing Height/settings panel, then click the left stick.
- Fixed recovery shortcut: hold physical left Menu + right stick click for one second.
  It works independently of the selected profile, including in native menus.
- Point either controller at a row and pull its trigger, or use right stick up/down
  to select a row and A to edit. Release the opening controls before recording.
- Press a button, squeeze a grip/trigger, move a stick, or touch a separate thumb
  rest. Press two inputs within 350ms for a combo. Release them to review.
- Choose APPLY (or A) to use the proposed binding; CANCEL (or B) keeps the old one.
- X cycles Direct, Press, Tap, Hold, and Double tap. Right stick left/right changes
  the selected binding's timing in 100ms steps (200 - 2000ms).
- Left stick click toggles capture of additional capacitive touch sensors. It is
  off by default so resting fingers do not accidentally become modifiers.
- Y restores the selected binding. The explicit OFF button disables it.
- Left stick left/right chooses one of eight profile slots. SAVE persists the
  current configuration; LOAD restores that slot. RESET restores defaults for the displayed mode.
- B or CLOSE exits. Buttons must be released and sticks centered before gameplay
  resumes. The recovery shortcut remains physical even if Menu/stick bindings change.

Gold shows assigned inputs, green presses, and blue capacitive touches. Finger
animation always uses physical grip, trigger and touch data, independently of
virtual game bindings. The editor does not replace physical hands or weapon grips.

## What is remapped

The editor exposes all native input channels queried by the supported TTR build:
left/right triggers, grips, primary/secondary face buttons, stick clicks, thumb-rest
touch, left Menu, and each stick direction. Filthy Casuals' weapon wheel,
consumable wheel, reload-mode toggle, height/settings and tutorial have separate
shortcut overrides. Unchanged bindings pass through to TTR unchanged.

TTR shares native bindings between contexts. The editor lists those uses together:
for example, remapping Trigger also remaps its native fire, mechanism, scope and
held-item uses. It does **not** pretend that Fire and Operate Slide are independent
native actions. Likewise, native Wait/Console use the same stick-click channel.
Spatial requirements - reach the holster, insert a magazine, swing, stab, flick,
release over the shoulder - remain TTR's normal interactions. Quest Home is owned
by the headset and cannot be remapped by a game.

A warning identifies shared source assignments. APPLY deliberately keeps both;
use OFF or another binding on the other row if a swap is desired. Nothing is
silently stolen from another binding. Automatic native context selection remains
TTR's responsibility. Native inactive actions cannot be reactivated by a remap.

## Profiles and installation

Profiles are stored outside game saves and MO2 mod folders:
`%LOCALAPPDATA%\FilthyCasualTTR\Controls\profile-1.fcp` through `profile-8.fcp`.
`active.txt` selects the next-launch slot. Saves use an atomic replacement and
one `.bak` backup. An occupied slot must first be loaded before overwriting it;
external edits are detected and rejected until reloaded. Version 1 profiles migrate
to version 3 when saved, preserving addon shortcut meanings and the former global mapping in discovered modes.

The private simulator uses its own `SEATED_TEST_DIR/controls-profiles` directory.
It cannot overwrite personal play profiles. Install the complete addon archive,
including `NVSE/Plugins/SeatedControls/Controls/`; it contains the model textures,
mesh data, generated terminal artwork and their license/provenance files.
Keep the personal SeatedControls.ini when updating. No registry/runtime change,
Meta Link setting change, replacement TTR DLL, or save-format change is required.

## Modes and installed add-ons

Use **< MODE / MODE >** to configure any installed provider while standing on foot.
You do not have to enter a car or aircraft. **AUTO** follows the actual gameplay
mode. Closing the editor always returns routing to the actual gameplay mode.
Each of eight profile slots contains separate on-foot, driving and flight banks.
A mode transition clears outputs and waits for released buttons / centered sticks.
Driving and flight are advertised by the optional Vehicles DLL; they are not
pretended to be part of the core mod. Unused destination channels are omitted.
Installing another provider grows the catalog without changing the editor.

The car provider exposes left-stick forward/reverse, right-stick steering, exit
and both physical wheel grips. The flight provider exposes movement, heading,
climb, descend, land, HUD and aircraft weapon channels. Both advertise the addon
wheel/reload/settings/tutorial shortcuts, which remain available while mounted.
All source buttons, stick directions and touch sensors can be assigned to any
exposed destination, including a two-input combo.

## Settings coverage

The left Menu input and the addon Height/settings action are remappable. The
physical recovery chord remains fixed; the headset's system Home is unavailable.
This is a binding editor, not a general INI editor. Height, reload preference and
routine popup hints have their existing in-game controls. Selector geometry,
pouch placement offsets, finger-animation enablement and the OpenXR runtime path
still use configuration files; some require restarting the game. Never imply that
every INI setting can currently be changed live from this panel.

## Edit offline, without the headset

1. Run the game once with your installed addons. It exports `catalog.json` into
   `%LOCALAPPDATA%\FilthyCasualTTR\Controls` even while you are on foot.
2. Open [the desktop editor](../tools/controls-editor.html) locally in a browser.
   Load that catalog and your `profile-N.fcp`. All rows are populated.
3. Choose any mode, input, modifier, gesture and delay; download the profile.
4. Quit the game, back up the old slot and copy the downloaded file into Controls.
   Load that slot from the in-game editor. Profiles belonging to absent providers
   are preserved. Nothing is uploaded or written into game saves.

For addon authors: [provider API](CONTROLS-PROVIDERS.md).

## Acceptance boundary

The editor is a local development candidate. Tests exercise real XR action queries,
profile save/load, default native Pause/Back, recovery, release barriers and physical
finger separation. Both-eye captures verify the 3D panel. Physical headset ergonomics
and every native interaction after arbitrary user-created profiles remain unproven.
Shared-source conflicts are intentionally visible, not silently resolved.

## Typed INI bindings and desktop access

Double-click **Configure Controls.cmd** (beside the README). The desktop editor
has a prominent YOUR MENU BUTTON section: type an input, a combo, or a hold.
Load your installed `NVSE/Plugins/SeatedControls.ini`, download the updated copy,
then back up and replace the installed file while the game is closed. Without
loading an INI it downloads a snippet to merge, not a replacement settings file.

For example, replace Menu with right-stick click and remove its old Favorites
assignment (this intentionally changes that native shortcut):

```ini
[Bindings]
MenuButton=RightStickClick
RightStickClick=Off
OpenControls=RightGrip+LeftGrip,Hold,1000
OpenControlsOnStart=0
```

`MenuButton` retains the existing tap/hold Menu behavior. `OpenControls` toggles
the binding editor directly, independent of Menu. Set `OpenControlsOnStart=1`
to open it on the next focused input session. The original recovery chord stays.
A combo's component inputs can still perform native actions before the editor
opens; choose or reassign controls to suit your play style.

Any exposed destination can be specified under `[Bindings]` for all modes, or
under `[Bindings.onfoot]`, `[Bindings.vehicles.driving]` or
`[Bindings.vehicles.flight]` for that mode. Mode-specific values win. These
explicit INI overrides win over saved profiles at startup and when loading a
profile. In-game edits work for the current session. Remove an override or use
`Default` to return ownership to the profile. `Off` disables the destination.
No INI means existing profiles and controls remain unchanged.

Binding syntax: `Input`, or `Input+Modifier,Hold,1000`. Gestures: Direct, Press,
Tap, Hold, DoubleTap. Delay: 200–2000 ms. Names ignore spaces, case and underscores.
Invalid bindings reject the whole INI override transaction and retain the saved
profile; the editor reports the error. Restart after editing the INI.

Destination keys:
```
LeftTrigger LeftGrip LeftPrimary LeftSecondary LeftStickClick
RightTrigger RightGrip RightPrimary RightSecondary RightStickClick MenuButton
MoveLeft MoveRight MoveBackward MoveForward TurnLeft TurnRight Crouch Jump
LeftThumbRest RightThumbRest WeaponWheel ConsumableWheel ReloadMode
HeightSettings WeaponTutorial
```
Input names:
```
LeftTrigger LeftGrip X Y LeftStickClick RightTrigger RightGrip A B
RightStickClick LeftMenu LeftStickLeft LeftStickRight LeftStickDown LeftStickUp
RightStickLeft RightStickRight RightStickDown RightStickUp
LeftThumbRest RightThumbRest LeftStickTouch RightStickTouch
XTouch YTouch ATouch BTouch LeftTriggerTouch RightTriggerTouch
```
Primary/secondary aliases are accepted. An input must actually be supplied by
the runtime: this does not create missing sensors or expose the headset Home
button. Known profiles now include Touch/Plus/Pro, Index, Vive, WMR, Pico 4 and Steam Frame.
See [controller profiles and input names](CONTROLLERS.md) for community-test status. Native
actions sharing a channel remain grouped. Provider labels in the editor explain
what each channel does in driving and flight.

Additional input tokens and hardware names are in `inputs.json` and the desktop editor's controller picker. [Full controller coverage](CONTROLLERS.md).
