# Sprint and wrist-guided light

Included in the Filthy Casual TTR core for the exact TTR 1.1.7 host listed in INSTALL.md. No extra flashlight or sprint mod is required.

## Sprint

Push the left stick to its forward rim, including forward diagonals. Default speed is 2x ordinary movement; default cost is 15 AP/second. Ease below 90% to stop. After AP exhaustion, ease below 65% briefly before restarting. After death/loading, return the stick to neutral first.

Sprint is on foot only and is gated off in menus, vehicles, sneak, swimming and over-encumbrance. It changes movement output without editing the actor's saved SpeedMult.

Open Settings and select Sprint. Use either stick up/down to choose a row and left/right to change its value. A/X selects or toggles the highlighted row. You can also point with either controller and pull that hand's trigger: click a toggle or the visible -/+ buttons. B or the visible Back button returns. Changes save automatically. No grip combinations, Y shortcut or stick-click page cycling is used.

`[Sprint]`: `Enabled=1`, `StartThreshold=0.97`, `StopThreshold=0.90`, `SpeedMultiplier=2.00`, `APPerSecond=15`. A cost of 0 means free sprint. `ShowNotification=1` shows one VR message with the existing Travel Light icon when sprint starts; set 0 to disable. It does not repeat during a continuous sprint. Breathing audio is not included: no suitable native human breathing loop has been verified.

## Wrist light

With Wrist flashlight enabled in Feature Selection, hold the left trigger
near the Wrist-Boy to use TTR’s usual light action. The lamp follows the
left hand and rests on the knuckles. Its coiled cable plugs into the underside
of the Wrist-Boy, below FILTHY.

Open **FILTHY → Wrist flashlight** for brightness, beam width, reach, colour,
and position. The default beam reaches 35 metres. Turn Wrist flashlight off
in Feature Selection to use TTR’s original light.

The projector shades visible surfaces at their world depth in each eye. Empty
space stays dark, including when the lamp passes close to your face. The
knuckle lamp and its cable remain visible on the hand. Local geometry casts
into a shared shadow map. The material pass includes instanced grass and uses
native vertex-buffer UVs for static cutout foliage. Distant terrain and
object proxies are excluded. The stock beam mesh is detached while the
projector is on and restored when it turns off.

Ordinary alpha-blended materials in the flashlight beam use diffuse and material opacity for partial shadows, including bottle glass. Grass and masked leaves cast through their texture cutouts. The beam keeps the existing 512 × 512 shadow map.

## Headset check

In a dark area, aim across a near railing or sign, then down the road. Turn
your head without changing where your wrist points. The light should stay
on the same surfaces as you turn. Roll the wrist above and below the cuff
to check the plug and coil.
