# Current beta validation: 1.8.0-beta.2 / TTR 1.1.5

The release is a beta for the exact TTR DLL identified in INSTALL.md. Unknown builds refuse hooks.
Core and Vehicles remain separate. Pool is not included. Original game and third-party mod files are not distributed.

Current checks cover:
- All 70 input sources, profile save/reload, combos/holds/double taps, INI overrides and missing-menu fallback.
- Mouth approach/dwell, tracking loss, head turns and one-item consumption of kebab, iguana, Buffout and whiskey.
- Recovered native magazine insertion with an empty chamber, trigger suppression until native slide cycling, repeated pickup and pouch return without duplicate rounds.
- Separate saved car/flight offsets, unchanged ground calibration, native driving, takeoff/climb/landing and exit controls.
- Bighorner actual-body hold, 30-second challenge, native fast-forward gait, idle on stick release and dismount.
- Native glass impacts on LAND and road geometry; the existing gentle-drop/sign/fade checks remain relevant.
- Portable local vehicle generation preserves the installed mesh/node geometry and transforms; generated game data stays private.

Simulator checks do not certify headset comfort or every controller family. No new physical-headset check was performed for this compatibility update.
Gentle object placement can still show native physics jitter, and glass visibility on every possible material is not certified.
Non-Quest controller profiles are available for community testing. Vehicle cockpit refinements remain beta work.

Validated 2026-11-14. Deathclaw-specific behavior was not rerun on 1.1.5.

The final near-wall bottle regression and four empty-bottle families passed native
pickup/release and rendered-physics checks on 1.1.5, including impact effects.
