# In-world weapon tutorial

The optional guide highlights reload parts on your equipped weapon and shows
instructions in the world. It uses the installed TTR 1.1.2 handling data; it does
not replace native firing, grabbing, or reloading.

## Turn it on

Triple-tap **left Menu** quickly to show or hide the guide. With a gun held,
a single Menu tap changes manual/automatic reload mode after a short delay.
Triple-tapping does not change that mode. Holding Menu still opens the seated
settings. The tutorial starts off each game launch and writes no save data.

In automatic mode, the guide identifies the automatic controls. Switch to
manual mode to follow the physical loading instructions. Use the green outline
on the real weapon; an instruction card is not an interactive button.

## Reload mechanisms

The tutorial teaches each distinct action rather than repeating hundreds of
similar weapons. A/X below means the primary face button on the gun hand;
the recorded right-hand demonstrations use A.

| Distinct action | Representative | What the viewer must see |
| --- | --- | --- |
| Detachable magazine / pistol slide |10mm pistol |Point at magazine and well; eject; grip spare; align and seat; point at slide; trigger-assisted rack |
| Rifle magazine / charging handle |Assault carbine |Different well direction; aligned insertion; charging-handle motion |
| Bolt action |Hunting rifle |Point at bolt; trigger interaction; unlock/pull/return/lock if native mechanism requires it |
| Swing-out cylinder / loader |.44 revolver |A/X plus wrist motion; loading position; loader; close |
| Loading gate |.357 revolver |Gate opening; individual rounds; close |
| Tube loading / pump |Hunting shotgun |Loading port; separate shells; holdA/X and grip fore-end to pump |
| Break action |Caravan shotgun |Open breech; shell positions; close |
| Energy cell |Plasma or laser rifle |Actual cell orientation and insertion path; no invented slide operation |
| Lever action |Cowboy Repeater |Hold A/X with the gun hand; off-hand grip at the highlighted action; pull and return (same control family as pump) |
| Charger clip |Chinese pistol |Pull slide and lock back with A/X; grip loaded clip; align above top guide; release; cycle slide; fire |
| Single-shot launcher / break action |Grenade rifle |Native open, insert, close and fire; shares the break-action lesson (observed reload animation9) |
| Shared pouch/recovery |Pistol and rifle |Grip spare; hold trigger on pouch for ammo types; drop; normal/gravity pickup; reinsertion and return to pouch |

## Ammunition and dropped parts

Look down at the native ammo holder in front-left of your torso. Hold GRIP to
lift a spare. Hold the trigger at the holder for about one second to change
between available ammunition types.

For a dropped magazine, use the normal pickup controls: grip it nearby, or point
at it, hold GRIP, and flick back with the gravity gloves. A recovered compatible
magazine retains its remaining rounds. Align it with the empty gun's well and
insert it using native manual reload controls. To stow it instead, bring it to
the ammo holder and release GRIP.

After topping up an already chambered pump shotgun, **Ready to fire** means you
can shoot. Cycle the pump after that shot; an unnecessary extra cycle can eject
a live round. The guide reads native state and does not force a loaded chamber.

## Coverage still to complete

The current 13-chapter video demonstrates ten weapon examples plus activation,
ammunition selection and magazine recovery. It is not complete weapon coverage.
The 583 metadata entries include variants and non-reloading items; that count
is not a count of tested weapons or distinct lessons.

Missing explicit demonstrations include the Fat Man, missile launcher,
grenade launchers, heavy ammunition/power packs, Tesla cannon, railway rifle,
and BB-gun/handle-only behavior. Flamer and recharger weapons need investigation:
their installed handling entries expose no reload parts and currently receive
the generic unmapped manual guide. No-reload items must be identified explicitly.

Coverage is complete only when every distinct native operating/loading action
has an accurate guide and demonstrated representative, with variants grouped
only after their behavior is confirmed equivalent. See
[coverage inventory](TUTORIAL-COVERAGE.json) for the current gaps; Fat Man is first.

## Compatibility

This development candidate targets the exact TTR 1.1.2 build listed in
[installation instructions](INSTALL.md). It reads your installed handling data;
no original game or TTR models, textures, or data are included. A supported
metadata entry is not a guarantee that every individual weapon variant has
been tested. Native hand poses are preserved.

Representative native reload sequences and follow-up shots have been checked;
physical headset comfort and final release approval remain separate from those
checks. The current guide is installed locally and available for review.
