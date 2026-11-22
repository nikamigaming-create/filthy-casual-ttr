# Dependency-only release validation

RC21’s archive omits the 14 NVSE vehicle functions. In the isolated TTR 1.1.7
MO2 profile, the vehicle module stopped at seat-script initialization until the
functions were restored. The next archive is assembled and audited by
`package.ps1`. It uses the published RC21 flashlight and settings binaries and
includes the vehicle scripts alongside its rebuilt vehicle DLL.

The final RC22 archive passed the isolated dependency-only simulator checks.
The stock Roadster used arcade road contact and accepted steering from either
hand on its original rim. It accelerated, moved, and exited with B. The
Vertibird took off through Pilot Inside. The right cyclic and left collective
grabbed, moved, and released independently. Stick movement, climb, landing and
on-foot control restoration passed. Both final eyes showed the installed
interior and Filthy’s authored handles.

The same ZIP passed worn Wrist-Boy handoff, controller mapping, the worn
equipment pages, thumbrest wheels, automatic reload, walk/run/sprint, book
set-down, and Bighorner riding in the simulator. The full-size Pip-Boy takes
lost the live compositor, and the empty-bottle tests failed pose or handoff
assertions. Those checks remain open along with stock masked character shadows,
physical headset play, and upscaling. Other car models use native handling.

Tested RC22 ZIP SHA-256

```text
FA27A0977E21AB62D8BD4F776EF67373C14B0AAD7CE45341C7F6C8B70823396C
```
