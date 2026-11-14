# Reload demonstration motion references

This is a visual-authoring note, not an alternative set of game controls.
TTR 1.1 remains authoritative for supported interaction and object ownership.
No source footage, photographs or manufacturer assets are redistributed.

## Observed human reference

- Mossberg / Jeremy Stafford, Loading and Unloading - Mossberg 500 / 590 Series
  Shotguns, https://www.youtube.com/watch?v=3_ufCUSZ27s, linked by the manufacturer:
  https://resources.mossberg.com/journal/mossberg-500-590-series-shotguns-loading-and-unloading
  Browser observations at 1:25 and 2:21: rear hand supports the grip near the body;
  the free hand approaches the receiver; the receiver turns to expose its bottom
  for tube loading. The gun is repositioned for the job rather than kept fixed
  at full aiming extension. These are observed frames/sections, not motion capture.
- RevIndustries, illustrated user instructions, Figure C:
  https://www.revindustries.com/docs/revindustries-speedloader-user-instructions.pdf
  Open cylinder and downward muzzle establish the loader's gravity-aligned path.
  Use this as a reference for gun orientation, not its proprietary loader details.
- Ruger American Rifle owner manual:
  https://ruger-docs.s3.amazonaws.com/_manuals/americanRifle.pdf
  Manual describes the action, but does not prove our hand pose.
- Ruger Firearms, official American Rifle Tech Tip:
  https://www.youtube.com/watch?v=r8OaJWV9RcQ
  Observed 0:30–0:37: left hand supports the fore-end, right hand operates the
  bolt; rifle stays close to the torso. This reference concerns hand roles only.
  TTR's off-hand trigger interaction differs from that demonstration. Do not
  label our crossed-hand bolt take as a matched human pose.

## Changes being verified

- Pump take: bring receiver toward torso and rotate underside toward the viewer
  before obtaining/inserting a shell; return to normal hold before pumping.
- Cylinder take: lower muzzle and move the gun closer before aligning loader;
  return before closing. Native geometry and loaded-round assertions still apply.
- Use eased approaches, a pause at contact and deliberate release. Measure native
  socket locations each time the gun moves. No guessed world-space insertion.
- Bolt contact currently enters TTR's native pose (free-pose weight zero). Adding
  grip input did not replace that native open-hand pose. Do not change the native
  hand rig merely to cosmetically stage a passing video.

Acceptance remains unproven until actual final-eye sequences show comfortable
reach, understandable hand roles, visible contact and accurate native operation.

## Latest review: 2026-11-11

- Single-shot grenade rifle: native open, insert, close, usable shot all passed
  (PID 36204). Reload animation 9 shares the break-action lesson, not a new lesson.
- Cylinder lower hold: native loading and closed geometry passed (PID 43548).
  Post-insertion still shows the cylinder clearly; full temporal review pending.
- Pump rotated hold: failed native insertion (PID 13712). Final-eye image also
  shows excessive muzzle rise and overlap. Rejected for the edit. A controller
  Euler roll is not a roll about the gun's actual barrel axis.
- Bolt closer hold: native cycle/shot passed (PID 29756), but the left forearm
  crosses the receiver. Rejected as human-reference acceptance.
- Outdoor recovery: native walking, gravity pull, recovered insertion and exact
  ammo accounting passed (PID 40096); follow-up shot failed. Not a complete take.

Acceptance must cover the full approach/contact/release, normal-speed motion,
both eyes, sensible wrist/forearm posture and visible action. No selected still
or successful ammo counter alone can pass a take.
