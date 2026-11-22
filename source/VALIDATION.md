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

Other car models use native handling. Stock masked character shadows, physical
headset play and upscaler behavior still need acceptance.
