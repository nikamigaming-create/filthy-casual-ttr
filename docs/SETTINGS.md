# In-game settings

## Pip-Boy and Wrist-Boy

With the native menu module installed, open **Filthy Casuals** in the full-size
Pip-Boy. On the worn Wrist-Boy, look at the screen and click the right stick to
open the same tab. General Settings opens first. **A** toggles Manual/Automatic
Reload on the top row. Use the stick to move between rows or adjust values with
left and right. **A** opens or applies a choice, **B** goes back, and **X**
cycles the available Filthy Casuals tabs. In the full-size Pip-Boy, click the
visible row or use the on-screen controls.

General Settings contains seated height, sprint, wrist flashlight, hands and
walking, optional features, controller mapping, and lighting when available.
Features apply together when you select **Apply changes**. The controller
editor uses the same control profiles as the larger HUD. Click the right stick
to change the mode being edited and use the left stick horizontally to change
the profile slot. Save the profile to keep new bindings after a restart.

## Large settings HUD

Hold left-stick click to open Settings with the default Filthy Casuals controls.
Your remapped Settings action still works. No headset Menu button is required.

Both input methods work on the same screen:

- Point either controller at a row or button and pull that controller's trigger.
- Either stick: up/down selects a row, left/right changes its value.
- A or X selects an action or toggles the highlighted switch.
- B returns. Back is also a pointer button and a selectable stick row.
- Numeric values have visible minus/plus buttons. Selecting their label alone
  does not change them. Changes save automatically.

Settings categories: Seated height, Sprint, Wrist flashlight, Feature selection,
and Controller mapping. There are no separate grip/Y/stick-click shortcuts for
editing individual settings.

Default height means normal play on foot. Height offsets
are stored separately for on-foot play and driving.
Open Seated height while in the relevant seat to adjust only that seat.

Sprint: enabled, speed multiplier, AP drain per second, stick position needed
to start, and start notification. Zero AP drain is free sprint; 100% stick is
the forward edge. Changing speed does not change AP cost or the activation point.

Wrist flashlight: enabled, brightness, beam width, reach, colour and lamp position.
Position has separate forward/upward rows. TTR's normal flashlight toggle remains.

World Shadows and Volumetric Lighting are temporarily withdrawn after outdoor strobing. Their settings are hidden, and existing INI enable flags are ignored without overwriting your preferences. The wrist flashlight remains independent. The experimental implementation is preserved for further work.

First-launch feature selection is separate from preferences. Features remain off
unless opted in. The selector uses eight choices per page and visible Previous,
Next, Select none, Save and close, and Cancel buttons. Settings can be reopened
later to change those choices. Vehicle features require a game restart.

The menus keep their existing RobCo housing and terminal lettering. The panel
stays where it opened rather than following every movement of your head.

Caption and HUD layout is optional: Features > Captions / HUD placement.
Turn it off to use TTR's original dialogue/name placement and world panels.
This does not disable the game's subtitles. The stored key remains
`[Features] WorldPanels` in `SeatedControls.ini.features.ini`.

## Calibration tracking check

Hold your head steady briefly before choosing Calibrate. If tracking is unavailable or the sample is stale, calibration keeps the current height and asks you to try again. Reset remains available. This check does not change native body proportions.
