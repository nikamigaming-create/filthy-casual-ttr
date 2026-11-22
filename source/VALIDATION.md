# Dependency-only release validation

RC21’s archive omits the 14 NVSE vehicle functions. In the isolated TTR 1.1.7
MO2 profile, the vehicle module stopped at seat-script initialization until the
functions were restored. `package.ps1` assembles and audits the RC22 archive.
It includes the vehicle scripts, FILTHY tab XML, and rebuilt core and device
settings DLLs.

The final RC22 archive passed the isolated dependency-only simulator checks.
The stock Roadster used arcade road contact and accepted steering from either
hand on its original rim. It accelerated, moved, and exited with B. The
Vertibird took off through Pilot Inside. The right cyclic and left collective
grabbed, moved, and released independently. Stick movement, climb, landing and
on-foot control restoration passed. Both final eyes showed the installed
interior and Filthy’s authored handles.

Earlier RC22 simulator coverage includes the worn Wrist-Boy handoff, controller
mapping, equipment pages, wheels, automatic reload, walk/run/sprint, book set-down,
and Bighorner riding. The current ZIP passed repeated full-size Pip-Boy opening,
FILTHY navigation, Data restoration, and return to gameplay in both eyes at
native render scale. The same session passed all four bottle families for
consumption, held empty identity, hand-relative pose, release and visible physics.
The maximum replacement position error was 0.352 game units. Physical headset
play, upscaling, and stock masked character shadows still need acceptance.

Tested RC22 ZIP SHA-256

```text
19DE97A90E6A302C7C01BE59D05F304A639B241C977CEA96D84E078B1AC04F87
```
