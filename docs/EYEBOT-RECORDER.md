# RobCo Eyebot

The recorder is included in the Filthy Casual TTR ZIP. MO2 loads its RobCo Eyebot.esp plugin. The game supplies the robot shell and collision from Fallout: New Vegas.

## Wrist-Boy and Pip-Boy controls

Open **FILTHY** on the Wrist-Boy or full Pip-Boy and select **Eyebot**.
Use the right stick to move through actions and **A** to select.

- **Deploy / Recall** sends the recorder out or ends a recording.
- While positioning, the **left stick** moves the Eyebot and the **right stick**
  aims the lens. Squeeze the left trigger to rise or the right trigger to descend.
  Release both to hold altitude. The green feed fills the width of the wrist screen and
  updates about once a second, with a longer interval on slow draws. Press
  **A** or **B** to park. **Fly / Aim** lets you move it again.
- **Take a Snap** captures one green still. From the parked page, select **Fly / Aim** to resume piloting or **Stow** to put the robot away.
- Select **Record** to start, then **B** to put the wrist page away.
- Open the page again and select **Deploy / Recall**, then **Save MP4**.
- **Clips** shows saved takes with green stills. **Trash** asks for confirmation
  and moves that take to the local clip trash. **B** cancels a pending deletion. Press **B** again to return to the recorder.
- Activate the deployed robot directly to open its own command menu. The
  **RobCo Eyebot Remote** under Pip-Boy Items > Misc is an additional entry.

Color videos are saved under the current user's Documents in
`My Games\FalloutNV\Eyebot Clips`. Saved clips keep their original color. The
thumbnail shown on the wrist is green. Trashed clips can be recovered from
the `.eyebot-trash` subdirectory.

## Runtime

During a take, the recorder samples visible actor and prop poses at up to
24 Hz into a bounded buffer. The on-arm pilot feed, explicit stills, and saved replays use independent camera draws. On Save, each pose is drawn
from the parked lens, the game's live transforms are restored, and a worker
encodes the frames to H.264. The MP4 is published after encoding completes.
For TTR 1.1.7, the player's head is restored for the saved third-person view
when the lens has enough clearance. The reveal is scoped to each export draw
so normal first-person rendering keeps its original masking.

The live wrist page uses TTR 1.1.7's original curved `pipboyscreen:0`
mesh and UVs. The Eyebot updates the screen material when its content changes.
The stock screen returns on close.

Saved replays are 512 by 512 and have no audio. The pilot feed and snapshots
remain at 256 by 256. Export can abort if a required actor or prop is no
longer loaded.

## Developer build

Run `eyebot/build.cmd` to build the device settings DLL, which also hosts the
recorder. The player packaging recipe runs `eyebot/build_content.py` to create
the Filthy camera mounts, display, textures, and plugin records. The activator
references the installed antique Eyebot model. JIP attaches the camera optics
when the robot is deployed.

## Verification

TTR 1.1.7 was checked in the isolated OpenXR simulator in the Prospector
Saloon. Both sticks moved the real game object from the live Wrist-Boy page.
The page showed a lens still, saved a color MP4, browsed its thumbnail and
moved the clip to recoverable trash. The Eyebot's native Activate command
menu was also exercised. Physical headset testing is still pending.
