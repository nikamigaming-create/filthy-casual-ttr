# Dependency-only release validation

## General-only RC22

The player ZIP has SHA-256
`4B769CC55019790E3FE5941D82A73F3843278BE55E7E5B989BB6666191CC91FE`.
The manifest covers all 229 payload files. The settings DLL exposes General
as the single FILTHY section. The full-size prefab contains the settings
controls. The core, OpenXR input layer and vehicle DLL match the preceding build.
Use `tests/rc22-general-only.ps1` on the isolated dependency-only profile to
exercise the flashlight submenu on both devices.


## RC22 Eyebot altitude controls

The November 22 headset report identified reversed altitude controls. A
dependency-only simulator run on the previous ZIP reproduced the reversal.
From an initial lens height of 638.647, the left trigger moved it down to
598.082 and the right trigger moved it up to 659.719.

The corrected player ZIP has SHA-256
`F4A406CA240246C050BA4CABE38463622141FBC3DCFDC7BA8FC74B8B6612AF7F`.
In the worn Pip-Boy flight test, the deployed lens started at
638.647. The left trigger raised the lens to 707.384 game units. The lens
stayed at 707.384 with both triggers released, then the right trigger
lowered it to 651.042. A parked the recorder, and the full-size Pip-Boy
subsequently opened the Eyebot page. The settings DLL in the headset
profile matches the tested ZIP. The earlier headset-confirmed flashlight
core is unchanged. Physical confirmation of this trigger mapping is pending.

RC21’s archive omits the 14 NVSE vehicle functions. In the isolated TTR 1.1.7
MO2 profile, the vehicle module stopped at seat-script initialization until the
functions were restored. `package.ps1` assembles and audits the RC22 archive.
It includes the vehicle scripts, FILTHY tab XML, and rebuilt core and device
settings DLLs.

The earlier RC22 vehicle and Pip-Boy build passed the isolated dependency-only simulator checks.
The stock Roadster used arcade road contact and accepted steering from either
hand on its original rim. It accelerated, moved, and exited with B. The
Vertibird took off through Pilot Inside. The right cyclic and left collective
grabbed, moved, and released independently. Stick movement, climb, landing and
on-foot control restoration passed. Both final eyes showed the installed
interior and Filthy’s authored handles.

Earlier RC22 simulator coverage includes the worn Wrist-Boy handoff, controller
mapping, equipment pages, wheels, automatic reload, walk/run/sprint, book set-down,
and Bighorner riding. That ZIP passed repeated full-size Pip-Boy opening,
FILTHY navigation, Data restoration, and return to gameplay in both eyes at
native render scale. The same session passed all four bottle families for
consumption, held empty identity, hand-relative pose, release and visible physics.
The maximum replacement position error was 0.352 game units. Physical headset
play, upscaling, and stock masked character shadows still need acceptance.

Earlier vehicle and Pip-Boy ZIP SHA-256

```text
19DE97A90E6A302C7C01BE59D05F304A639B241C977CEA96D84E078B1AC04F87
```

## Combined RC22 headset candidate

The preceding combined player ZIP had SHA-256
`AFE4BECE4724B8A378B180A9FC3EB29FACA960B01269AB31F6D28FF8C724080B`.
It includes the flashlight core the player confirmed on the physical headset
(`48FB0763FD4A8D1BBBB4C0BF6CB4A5A67353FCE7862A17BD00641AF7324CC08B`).

The previous package omitted the RobCo Eyebot records and scripts. An older
simulator fixture supplied the missing files privately. The combined ZIP
provides the records, eleven native scripts, and authored recorder optics.
New Vegas supplies the robot shell through its installed game data.

The isolated dependency-only MO2 runs loaded that ZIP directly. Both Pip-Boy
screens opened FILTHY > Eyebot. The wrist deployed and parked the recorder.
The full-size page showed the parked robot status in both composed eyes.
The inventory contained one remote. The parked lens rendered a still and tracked
Sunny moving across 70 scene samples. All 83 MP4 frames encoded and restored
the live scene transforms. The subsequent headset pass identified the altitude mapping above.

The same ZIP subsequently passed full-size Pip-Boy entry, Seated Height, close
and reopen, FILTHY reentry, Data restoration, and return to gameplay. All four
bottle families consumed and exchanged into the correct native empty in hand.
The largest hand-relative replacement error was 0.181 game units (rounded up).
Every sampled moving physics interval had corresponding visible bottle motion.
