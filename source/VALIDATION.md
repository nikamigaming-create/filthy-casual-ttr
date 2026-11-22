# Dependency-only release validation

RC21’s archive omits the 14 NVSE vehicle functions. In the isolated TTR 1.1.7
MO2 profile, the vehicle module stopped at seat-script initialization until the
functions were restored. The next archive is assembled and audited by
`package.ps1`. It uses the published RC21 flashlight and settings binaries and
includes the vehicle scripts alongside its rebuilt vehicle DLL.

With the original dependencies installed in the simulator, the stock Roadster
acquired arcade road contact, accelerated and moved, then exited with B. The
stock Vertibird took off through Pilot Inside, moved with the right stick,
climbed, and landed, then restored on-foot controls. Both final eyes showed an
unobstructed stock flight view after correcting the camera frame.

The fitted flight interior and physical wheel grips still need stock-resource
attachment work. Stock Cheyenne body shadows, physical headset play and upscaler
behavior still need acceptance. Keep the candidate unpublished until these
checks pass.
