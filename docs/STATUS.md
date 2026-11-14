# Current 1.8.0-beta.1 release candidate

Fresh core/input/vehicle builds pass. Focused mouth, 70-input remapping, typed INI,
and 30/60-second creature challenge unit checks pass. Current native checks pass
repeated recovered-mag pickup/insertion/pouch return, no automatic chambering,
native slide cycling, four ingestible families, separate car/flight seat saving,
ground restoration, driving, takeoff/climb/landing, Bighorner body hold/run/idle/exit.
Core SHA-256: 1F2C930882B28DAA9C08015B7BBB60DFACEF58F7EF255BEC50C2B0204988509C.
See [current beta coverage and limits](CURRENT-BETA.md). Earlier entries below are historical.

# Food/glass installation mismatch corrected — 2026-11-13

- The normal play profile still had core 86C87DB0 and input 89F72CF9, rather than
  the matched candidate containing the ingestible/mouth and LAND projection fixes.
- Fresh x86 core/input build and mouth unit checks pass. Native isolated session
  PID15460 used the exact six-file candidate and a copy of the personal INI, including
  HeightOffsetCm=66.5. Kebab, iguana, Buffout and whiskey each consumed exactly once.
- Actual native glass geometry on desert LAND (4 vertices/2 triangles) and road
  mesh (9/7); both-eye screenshots show shards and their disappearance after the
  native lifetime. Puff and positional sound dispatch succeed in the native log.
- Frozen candidate workflow now keeps tested DLLs, layer manifest, glass ESP and
  textures together. Installation requires candidate hashes and visual review,
  backs up existing files, verifies destination hashes and preserves personal INI.
  Procedure: docs/LOCAL-CORE-UPDATE.md. Candidate: outputs/food-glass-candidate-20261113.
- Mouth interaction remains sticky-mode-only: deliberate approach and 650ms dwell.
  Squeezing returns ownership to native handling. Stimpak keeps native stabbing.
- This run does not resolve earlier gentle-placement jitter or certify every
  surface (the previous cinderblock fixture failed), nor replace a headset check.

# Creature ride durations — 2026-11-13

- Bighorner default is 30 seconds; Deathclaw default is 60 seconds.
- Each species has ChallengeSeconds in FilthyCasualVehicles.ini. Zero starts
  with steering unlocked after a valid mount. Sneak/behind/undetected checks
  remain mandatory for Deathclaws; dismount still resets and resumes hostility.
- Focused C++ tests pass: defaults/custom/zero, malformed/nonfinite/negative
  configuration, pause, missed body-hold ejection and reset. x86 vehicle build passes.
- Installed normal development addon with backup and matching SHA-256 recorded
  in evidence/creature-timers-installed.json. No game was running during installation.
- Prior native runs verified old 60/120-second challenges, mounting gates,
  body hold, steering, dismount hostility and restoring movement after loading
  a mid-ride save. These are not a runtime rerun of the new duration configuration.
- The latest Deathclaw showcase did not achieve the requested native death ending;
  that video is not final. Physical headset comfort/haptics remain unverified.
Installed normal MO2 core/input and controller assets; INI hash unchanged. Rollback: evidence/controller-profiles-installed.json. Mobile montage: outputs/controllers/Filthy-Casuals-Controller-Montage-Mobile.mp4 (52.9 s, 1080x1350, 2.06 MB).

# Controller profiles and typed INI access � 2026-11-13

- Adds typed INI overrides, configurable direct editor access, startup fallback,
  desktop Configure Controls.cmd, and settings-preserving INI download.
- Expands to 70 stable input sources; old source IDs and profile versions retained.
- Raw profiles: Touch, Plus, Pro, Index, Vive, WMR, Pico 4, Steam Frame, Simple.
  Vendor extensions are requested only if advertised. Existing native suggestions
  are merged, not discarded; missing native profiles translate host Touch actions.
- Device identity and model chosen per-hand from OpenXR. inputs.json exports names,
  stable tokens and paths. Models are MIT WebXR assets; Frame is an original diagram.
- Per-profile simulator session passed all eight featured families: trigger/grip,
  live detection/export, editor display, Index force and seven Frame extra buttons.
  Quest Pause/Back still passed after switching profiles. Session PID3128 closed.
- Source/path and binary mesh validation: nine profiles, 70 tokens, 18 model files.
- Final montage refresh passed again (PID39452 closed), improves model lighting/view angle. Physical
  non-Quest controller compatibility remains for community testing, not certified.
- No user INI/profile/save or global Link setting changes. Public release remains
  1.7.0-dev.1 pending the user's final headset test; local package is 1.8.0-controllers.1.

# Bottle release render/physics handoff - 2026-11-13

The physical-headset report was reproduced: a consumed bottle's empty replacement
lost native hand ownership and its Havok body fell, while the rendered root stayed
at the release point. The old handoff test checked only grab ownership. Its earlier
"throw" claim was insufficient and is superseded here.

- Replaced selector positioning via MoveTo plus three separate SetPos calls with
  one native JIP SetPosEx. These newly dropped/spawned references already belong to
  the player's cell. No new physics, velocity overrides or per-frame pose pinning.
- Ordinary native ground pickup was the matched control and did not freeze.
  With the placement correction, the replacement's visible root, reference and
  Havok body advance together after native grip release.
- Preserves the full bottle's native hand-relative position and rotation when
  replacing it with the matching empty. No new grip pose or Pip-Boy remapping.
- All four families pass consumed count, empty identity, pose preservation,
  squeeze takeover, release and more than eight distinct rendered flight poses:
  Whiskey, Scotch, Nuka-Cola and Sunset Sarsaparilla (PID10632).
- The motion gate also compares consecutive physics/render movement. It rejects
  the original captured failure (1,438 ms continuous visual stall while physics
  moved); all four corrected families have zero such stalled intervals.
  Evidence: evidence/bottle-release-regression.json.
- Full-to-empty position errors were 0.21-0.44 game units (under 0.7 cm); maximum
  rotation matrix error was 0.000124. Existing native model shape differences remain.
- Final-eye stereo capture: work/vehicle-controls/bottle-release-proof.mp4 (90 s),
  with frame metadata. Sequence contact sheets and detailed release frames reviewed;
  visible bottles leave the hand and descend, and the Sunset impact puff appears.
  This is simulator evidence, not physical-headset acceptance or a showcase film.
- Focused native-impact decision tests pass. No new audio test was made in this
  correction; existing glass sound/decal/fade implementation is unchanged.
- Installed normal MO2 core DLL with matching hash and rollback manifest:
  evidence/bottle-placement-installed.json. User INI hash unchanged. Game closed.
  Headset throwing acceptance remains pending; public release remains 1.7.0-dev.1.

---

# Car split sticks and moving selectors - 2026-11-13

- Car propulsion now uses left-stick Y; steering uses right-stick X. Right Y
  and left X are unused for driving. Flight stick mapping is unchanged.
- Weapon and consumable previews are anchored in the current vehicle frame,
  transported after the same completed native pose as hands and camera.
  Driving axes no longer dismiss the wheel or get drained by item selection.
- Default thumb-rest shortcuts remain available mounted. Remapped Filthy
  Casuals actions are no longer restricted to on-foot mode, and both vehicle
  control catalogs expose the wheel/settings/tutorial actions.
- B closes the selector without also triggering vehicle exit. Native right-hand
  equip/hold and ordinary gun/item interactions remain the handoff path.

Native split-axis, both wheel shortcuts, moving touch selection, gun fire, drink handoff and B close/exit checks pass (PID35056). Mounted previews capped at 30 game units for seated reach; on-foot distance unchanged. Current validation: work/vehicle-controls/car-axes-selectors.log.
Install/rollback: evidence/car-axes-selectors-installed.json.
Personal settings/profiles preserved; no public release change. Headset check
of this latest correction remains pending.

---

# Vehicle hands, sticks and bottle handoff - 2026-11-13

This local correction supersedes the pending vehicle-control notes below.
Target remains TTR 7EB72860; public 1.7.0-dev.1 is unchanged.

- Corrected four-byte-early native hand transform offsets. The earlier writes
  overlapped native input, causing Pause and frozen/unstable hand contacts.
  Exact consumer guards plus bounded-write sentinel tests now cover the fields.
- Both execute and condition queries now carry the dependency's stick inputs.
  Right stick controls travel/steering in cars and translation in flight.
- The flight dependency's repeated default combat-disable call no longer
  holsters your weapon each frame. Its other seat/control locks are retained;
  other scripts and nondefault control-disable calls are untouched.
- A vehicle grip now claims only a new squeeze started at its control. Holding
  a native magazine/item and passing a control cannot steal that squeeze.
- With a weapon drawn, both triggers and A/X pass through for normal firing,
  offhand bolts/slides and reload handling. Physical control grips are separate.
- Removed only the added duplicated center-console pedestal from the owned
  local cockpit model. Original panel, cabin and two control handles remain.
- Empty drinks now use native reference/Havok placement before pickup, retaining
  the original hand-relative pose. Four bottle families pass identity/counts,
  pose preservation, squeeze takeover and native release/throw. No held pinning.
- Car simulator checks pass real travel, steering, both/independent wheel grips,
  no reach-induced Pause, firing while driving, hands-free steering, reverse and
  B exit. The earlier reverse assertion queried a ref as a quest; its corrected
  native-reference check passes without another gameplay change.
- Flight simulator checks pass travel/climb, independent control grips, no
  reach-induced Pause, rifle bolt cycling/chambering/firing, landing, survival
  and restored on-foot controls. PID37396 also passed native pistol magazine
  pickup/insertion and firing during 15,224 units of actual flight, with no Pause.

- Tutorial highlights now publish after the completed native hand/weapon pose,
  rather than using the previous MainLoop transform. Its reading card is cabin
  local while mounted; on foot it keeps the original stable world zone. No
  smoothing or newest-XR overlay pose is applied to these native scene nodes.

The world-anchored vehicle selector limitation recorded here is superseded by
the split-stick/moving-selector correction above.
Tutorial moving-flight and on-foot publication/holster checks pass. Installed
locally with matching source/staged DLL hashes and rollback manifest.
Physical-headset acceptance of this correction is still pending. Test runners
close their owned games. Personal saves, input profiles and graphics unchanged.
Installation hashes/rollback: evidence/vehicle-hand-controls-installed.json.

---

# Local work since public 1.7.0-dev.1 - 2026-11-13

Public release below remains unchanged. Do not describe the vehicle candidate
or glass appearance as physically accepted without the evidence noted here.

- Created and reloaded FC_Vehicles_Stocked in the isolated QA save, then copied
  its matching FOS/NVSE pair to the Vehicle Test profile. Fourteen supported
  bottled drinks, Stimpaks, pistol/carbine and ammunition are present alongside
  the fuelled Roadster and landed Vertibird. Existing user saves untouched.
  Manifest: evidence/vehicle-test-stock.json.
- Sticky consumption now hands the matching native empty bottle to the same
  right hand. Four drink families passed consumption/count/base identity and
  native squeeze/throw in work/vehicle-controls/bottle-combined.log (PID40924).
- Bottle impact fix installed locally: core A143540A, original glass ESP/DDS,
  in Seated Custom and Filthy Casuals Vehicle Test. Personal INI/input preserved.
  Actual native grip releases filmed: gentle drop intact, hard floor throw and
  three Stop-sign impacts shatter. Positional glass sound, particle puff and
  both-eye decal visibility verified; engine-owned 10-second timed fade recorded.
  Video: outputs/Filthy-Casual-Bottles-With-Sound.mp4 (41s, stereo game audio).
  Rollback/install hashes: evidence/bottle-impact-installed.json.
  No public release change or physical-headset acceptance in this turn.
- Bottle breakage requires native Havok contact and relative collision velocity
  change. Unit negatives and native gentle/hard-drop/lifetime checks pass.
  See docs/bottle-impacts.md. Final appearance/installation is recorded in the
  bottle evidence manifest once complete; do not substitute counters for pixels.
- Vehicle candidate includes left-stick driving, independent grip attachment,
  knee-steering pose and flight handle/left-seat fit. A current car reach fixture
  opens a native pause screen and freezes native palms; physical wheel/knee
  acceptance is still unresolved. This is distinct from bottle impact testing.
- Native car mount disables fighting. A diagnostic allowing all player controls
  proved native firing while mounted; the narrower combat-only fix is built
  but its integrated driving/fire proof is still pending the reach issue.
- Earlier flight candidate passed right/both/independent release and landing
  checks (PID36456). This does not establish acceptance of later car changes.
- QA wrapper currently copies the vehicle candidate into the shared Vehicles
  Development mod. Do not claim normal vehicle setup is isolated from that copy.
  Core install state is recorded separately; preserve personal INI and settings.

All test runners close their owned game. No personal-save edits, global XR/Link
changes, public publication or game-asset distribution were performed here.

---

# Installed and published: 1.7.0-dev.1 - 2026-11-13

Public main and prerelease: commit 65ffdc9c4f6e5b66fd4067d4553744bde75bfa39.
https://github.com/nikamigaming-create/filthy-casual-ttr/releases/tag/v1.7.0-dev.1
64-entry archive, verified per-file manifest, clean source reference; 24,981,675 bytes.
SHA256 44ca581bfc950b693000aa90dbabd77bf04988287355a90ed59e94b1671c836e.
Public checkout builds core and optional vehicle source; controls, offline and
native-mag unit/static checks passed. Runtime coverage in VALIDATION.md.

Installed tested core/input, full RobCo assets and tutorial assets into normal
Filthy Casual TTR. Personal height65cm, manual reload and hidden routine popups
preserved; [Reload] RecoverWorldMagazines=1 added. Backup:
D:/TTWVR/Backups/RobCo-Modes-Recovery-20261113-100603.

Created profile TTWVR - Filthy Casuals Vehicle Test with local settings/saves,
normal personal mods plus car/flight dependencies. Pool and all QA stages off.
Current core wins over the old Vertibird development core DLL. Native plugin
masters/order and saved fixture plugin availability verified. Personal saves
untouched. Play Vehicles Test.cmd uses the proven MO2 game route and clears QA
runtime/layer environment, targeting Meta Link's per-process x86 runtime.
Load FC_Vehicles_Ready: fuelled Roadster and parked native landed Vertibird.

All owned game sessions closed. User physical-headset check is next; no claim of
exhaustive custom-binding or all-weapon acceptance. No Link/graphics changes.
Local evidence: evidence/robco-modes-installed.json, evidence/robco-release.json.
Public scope header is relocated from local-runtime to root (same implementation).

---

# RobCo modes, offline editing and native magazine recovery - 2026-11-13

Current candidate 1.7.0-dev.1 supersedes the earlier controls candidate below.
Combined simulator tests passed; see VALIDATION.md. Pool remains disabled.
Personal height/manual reload settings preserved; vehicle test uses an isolated save.

- SeatedControls.dll: 380115CDECCA7021E35EDDFF5930DAF0AA8771D1B4ACCE220E3AAE97D6C83C7A
- SeatedInput.dll: D8D69BC67DC02E273A5872F4970A65CB53A894C8AEAC1BDA69101D5776767024

Publication/install status is recorded separately after final packaging.

---

# RobCo runtime controls editor installed locally � 2026-11-13

User requested all current TTR bindings displayed/remappable in game, with the
same real 3D Quest controllers as MGS5VR, RobCo styling, and saved profiles.
Completed local candidate: 21 native input channels + 5 independent addon shortcuts.
Shared native consumers remain grouped, explicitly described in the panel; no
unverified semantic native hooks or guessed context splits were added.

Normal MO2 Filthy Casual TTR now contains the accepted pair:
- SeatedControls BD08C85AF3BD823B0B76909AC9584D0007FE2B4B5FF081235371EEB9871175FF
- SeatedInput 02D6515C698F43627B38FB13E2EF62B7443E019E2CAF30D211B9457DEF4D4F2C
- Upstream TTR 7EB72860D209D9B9BDD774607220EE63323367AA9FA525F6EB1B23184F30F91E
- Backup D:/TTWVR/Backups/RobCo-controls-20261113-084931
- Installation hashes and unchanged personal INI: evidence/controls-installed.json

Open: hold left Menu -> click left stick in Height/settings. Fixed recovery:
physical left Menu + right stick click for 1 second. Either controller ray can
select rows; A/B/right-stick fallback stays fixed inside the editor. Eight profile
slots use atomic writes + backups outside game saves; occupied slots and external
edits are protected. V1 profiles explicitly migrate to V2 without shifting addons.
Raw physical fingers remain separate from virtual native bindings. Input loss,
closed editor, profile changes and inactive native destinations have release gates.

MIT WebXR Touch Plus models and MIT MGS5VR renderer adaptation ship with notices.
Original generated RobCo artwork and full prompt are recorded in assets/controls.
Own deferred D3D11 commands preserve the host graphics context on execution.

Validation: focused C++ tests passed strict profile parsing, analog preservation,
gestures/chords, release barriers, double-save protection and runtime loss.
Combined final simulator PID14016 passed native Pause/Back, existing Height entry,
actual XR pointer hit/record/cancel, model resource rendering, record/apply X to
right trigger, real native queried down/up values, save/load/defaults, fixed
recovery, raw finger curls while native trigger suppressed, and focus recovery.
Both-eye final captures reviewed; readable RobCo panel and actual controller
geometry visible. No physical-headset comfort claim, full temporal motion capture,
or exhaustive gameplay certification for arbitrary custom bindings. No normal
save-format, native engine adapter offsets or personal graphics/runtime changes.

Archive: outputs/Filthy-Casual-TTR-Controls-RobCo-candidate.zip, 54 allowlisted
entries, all manifest hashes verified, own/appropriately licensed content only;
no TTR DLL, QA runtime or personal profile bundled. Public release unchanged.
Game closed normally after the combined test. See docs/CONTROLS-EDITOR.md.

# Solo Pool VATS installed; frontal preview ready - 2026-11-13

Accepted PID9044, Pool DLL 43A16BB1AF6E98F922B25BE307D9DA9264D3E11CC0ED48B33507BA127F4C451E. Normal MO2 now has this
local candidate with the unchanged Filthy Casual core/Meta/input settings.
Backup: D:\TTWVR\Backups\Pool-VATS-20261113-072315.

Activate table -> Solo Trick Shots; hold right trigger450ms; left-hand ray +
left trigger selects a ball, X cycles; A explicitly executes. Release/B cancels.
Normal practice and Sunny remain unassisted. Actual balls receive green brackets,
selected target gold box/number, and native-world predicted paths. Original VATS
SOUN records play from the owned master; no audio assets are distributed.

Root transform initialization and bounds publication fixed invisible annotations.
Own model is warmed with trigger released before arming; planner uses fresh tracked
inputs and visible drawn cue identity, independently of physical strike samples.
Physical/manual cue-contact safeguards remain intact.

Passed: normal-mode rejection, mode callback, three openings, X and actual native
left-ray selection, cancel/held-A/release/no-double-shot, native physics outcome
for all16balls, prediction error 0.010440 table units. Both-eye
frontal highlights/paths reviewed. User requested cue-end view, not side framing.
Video: outputs/pool-vats/Pool-VATS-Front-Mobile.mp4, continuous13.4sec at actual
speed. Capture491frames with4 isolated repeats; no interpolation. Silent recording.
No claim of physical-headset comfort, every possible layout, or complete Sunny match.
Owned test closed normally. Public release remains unchanged.

Portable candidate: outputs/Filthy-Casual-Pool-0.3.0-beta-vats-candidate.zip;
406 exact allowlisted files. Own VATS geometry/textures included; all retail and
upstream dependencies remain locally imported, not redistributed.

Follow-up hardening: reject partially bound visual state if an externally replaced
NIF omits required card/marker nodes; shipped authored node set verified complete.
Pool remains first; darts later; resizing cancelled.

# Equipped cue installed; Pool VATS next - 2026-11-13

Manual pool candidate DD837A5B passed PID19024: Sunny opening, wheel-selected
WEAP4346, bridge squeeze leaves equipmentPause=0, actual forward-stroke contact
at7.96s (0.358m/s), exactly one reply, B exit retaining selection. Both-eye review
accepted gameplay, not a complete match or perfected hand-pose showcase. Rear
hand is outside framing; bridge fingers are somewhat raised. Session closed.

The decisive input bug was bridge-owned squeeze also reaching ball pickup
preparation. updateEquipment now masks that hand from candidate, direct and ray
pickup intent for the entire owned press. Existing held/pulled objects and return
recovery remain untouched. No collision tolerance increase or physics replacement.

Installed Pool/Core/Input pair into normal MO2 with unchanged personal INI.
Backup D:/TTWVR/Backups/Pool-cue-20261113-062857. Evidence pool-cue-installed.json.
Source-only equipment candidate ZIP validated381 allowlisted files, no retail or
upstream assets. Existing public release unchanged. Mobile same-match video:
https://drive.google.com/file/d/1R6G5CWYx4XKF1zclsZYM2-O1qqNuFmWv/view

User now requests Pool VATS trickshot prediction PLUS optional assisted shot.
Keep working baseline installed while adding hold-right-trigger preview, explicit
A confirmation and release/B cancel. Own16-ball prediction copy; no live state
mutation until confirmed shot enters existing vrShot/rules. Pool remains first;
darts after, multiplayer later, resizing cancelled.

# Equipped cue / native contact integration - 2026-11-13 (WIP, not installed)

Priority: finish single-player pool against Sunny, compile/install the accepted
build, and deliver an actual same-match mobile video. User cancelled resizing.
Darts against Sunny is queued AFTER pool; multiplayer follows finished local
games. Do not divert this work to those features or publish an unaccepted build.

Normal MO2 still uses the room/rack build below. Candidate remains isolated.
No saves, personal settings, graphics or public releases were changed.

Candidate: legacy own MISC cues convert one-for-one to WEAP4346; normal weapon
selection/draw and pool entry; Casual wheels stay enabled; native menus and
owned B/grip releases have separate handling. Right-hand rear cradle and optional
left sliding bridge preserve native wrist/arm IK. A nearby bridge press is
reserved before native melee support can steal it; held-away presses stay native.

Root issue proven: native contacts see stock weapon geometry before our late
pool-grip transform. Exact7EB7 call160F3A->162550 now temporarily presents the
same real cue root in RAW grip coordinates, using completed corrected-grip->wrist
calibration. Original native solver publishes points and frame, then root WORLD
is restored in finally. Final visible cue follows corrected native wrist/IK.
No raw-only hand override or global contact reset was introduced.

Native table collision correctly rejects strokes through the cushion. Owned
Havok surface queries proved this; recording now raises rear cue and uses a
reachable70cm two-hand separation. Previous long reaches were bot staging faults.

Equipped shot still NOT accepted. With correct address/bridge, native avoidance
repelled the cue before actual visual-tip contact. Latest candidate96AFECD4 uses
existing per-object primary-weapon flag+1FD for ONLY the active unheld cue ball,
ONLY during original right-hand solver161280->1564F0. Actual pool tip sweep owns
that strike. Both hands, other balls, actors and table/world remain native. Byte
restores in finally. Current visible cue/root and same-native-pass guards apply.
No collision radius or hit tolerance was expanded. Static second review passed.

Native dispatcher is shared with Vehicles; Vehicles rebuilt but not reaccepted.
Exact new shared-owner pair must be accounted for before updating a profile that
uses Vehicles. Normal vehicles are currently disabled. Pool/Core/Input candidate
binaries are not installed in normal MO2 yet.

Evidence: pose-boundary.json proves corrected native grip imposed the old clamp;
native-collision-geometry.json records exact native cue points/radii; bounded
sim-session/pool-contact.csv records actual visible strokes (failed tests remain
failed). Candidate96AFECD4 Sunny-film PID33300 recorded Sunny's real first shot,
but bot failed wheel touch after viewing sideways. Film is incomplete, not accepted.
Normalize the bot's head-facing before opening its wheel; do not call it a pool
physics failure. tests/pool-equipped-match-film.ps1 is the pending combined run.

Next: real equipped player stroke in SAME Sunny match; both-eye video review;
native grip/holster and clean exit; package source-only recipe and install exact
accepted build with backups and preserved settings. No synthetic hits/scores.

# Pool room startup and wall rack installed - 2026-11-13

Installed the verified physical-equipment build into normal MO2 Filthy Casual Pool;
TTWVR - Seated Custom now also enables Filthy Casual Pool Equipment.esp.
Latest TTR remains 7EB72860. PhysicalReloadParts remains off; upstream owns ammo.

Root cause: the six original saloon tabletop decorations overlapped our equipment,
and the inherited ClearTable routine applied PushObject strength1200. ClearTable
now applies no impulses. Exact untouched original references are retired only
when native ownership confirms unheld/unpulled; moved/taken items are preserved.
Initialization waits for the saloon/collision and stops after successful setup.
Four spare cues stand in an open-front wood wall rack beside the jukebox. Fixed
furniture references are authored in the real room; player-moved cues are not
repeatedly repositioned. Existing IDs and save files remain intact.

Runtime acceptance PID42800: six duplicates retired, 24 pickup objects present,
30 stability samples, four cues with zero rest drift, two practice starts/exits,
16 balls on cloth, native gravity pickup/hold/release passed. Final visual PID46412:
15 rack board meshes present and both-eye rack/table captures reviewed; session
closed. This is simulator acceptance, not a new physical-headset test.

Portable source-only package validated380 files, excludes owned NIF/DDS/equipment
ESP and completed upstream scripts. Actual packaged importer ran against the local
owned game and pool dependency. All22 generated NIFs match tested contents after
normalizing randomized PyFFI string-table ordering; all26 installed scripts match
build after newline normalization. Normal profile retained except Equipment ESP.
No public release changed. Backup: D:/TTWVR/Backups/Pool-room-20261113-032514.
Evidence: evidence/pool-room-installed.json; work/pool-equipment/room-rack-acceptance.json;
evidence/pool-room-final-rack_{left,right}.png and pool-room-final-table_{left,right}.png.

# Pool gameplay capture completed - 2026-11-12 (PID28140)

Latest installed TTR remains7EB72860, native worldmag owns ammunition, and
PhysicalReloadParts=0 in normal play. Normal pool remains the released compatibility
build; the physical-equipment candidate is isolated. No saves or public releases changed.

Private recording bot now uses Sunny's authored practice-stroke/delivery/follow-through
timing through ordinary XR controller poses. Setup pickups are omitted from the video.
Native hand pose is calibrated from wrist/index/pinky anatomical landmarks, not the
NPC wrist bind alone. Corrected two recording-script faults: table collision left raw
controller input below the native floor, requiring measured upward dead-zone recovery;
and an absolute tracking-origin reach ceiling incorrectly rejected valid leaned-head
follow-through. No object/bone/ownership/hit writes were added for choreography.

PID28140 completed the same native Sunny match with Sunny's shot and the player's reply.
Bridge plane/channel, cue address, native one-shot counter, and visible render-leaf
contact passed:1269 samples, nearest tip-to-ball1.75563 game units. Both-eye captures
completed; game closed and normal TTWVR - Seated Custom profile restored. Parent
chronological frame review confirms corrected level bridge, stroke and ball movement.
Prior PID16580 upright bridge and failed41228/48224 takes are not final video evidence.
This is simulator gameplay evidence, not new physical-headset acceptance or a completed
full-match claim. Mobile edit uses samePID28140 sources at original speed; private Drive
handoff completed after final export review:27.5s/825frames, SHA9B5E092C,
Drive file1maqGrKxZFniQfiBq-3omM4T0DQmCfJJD; metadata size1437442 verified.
Evidence: evidence/pool-gameplay-mobile-handoff-28140.json. Public publication unchanged.

# Latest TTR installed - 2026-11-12 (7EB72860)

Installed Downloads/TTWVR (5).zip into normal MO2 after exact-build port and
combined runtime checks. Upstream still labels this 1.1.2; identify it by DLL
SHA256 7EB72860D209D9B9BDD774607220EE63323367AA9FA525F6EB1B23184F30F91E,
PE6AF60DF2. Three upstream binaries changed; personal TTWVR.ini retained.
Native worldmag now owns dropped ammunition and save restoration. Our legacy
physical-magazine install AND tick/restoration paths are disabled on this host;
normal SeatedControls.ini explicitly has PhysicalReloadParts=0. No save edits.

- Core F48A413E85CFEA49C0230269E9A16EE0EF60CA4C89D5BE069581BC977DC2E57D.
- Meta 180A81A97BB967FEC6ADE2AB6585E109A975EE6C1E4276A2645A4191F7556A44.
- Normal pool BF330A49816C6D6BB1372842EE46E7D2846A2A5DF5F1A4D3EAA3EFB504650847.
  This is a private compatibility rebuild of released pool source, preserving
  existing player-cue controls without Equipment ESP. Canonical pool source
  remains the separate physical-equipment development candidate.
- PID44440 passed weapon/aid/stim native handoffs, automatic reload/fire, height,
  dialogue side/name anchoring, and scope519 restores/519 attempts/0 errors.
  Physical ammo diagnostics active:false/nativeOwned:true. PID42344 passed
  released pool entry/exit and tracked-camera preservation. Both sessions closed.
- PID27928 crashed due to two missed small native thunk addresses in plugin.cpp;
  D094 to DA03 and A641 to ADC1 corrected. Latest combined run passes.
- Static guards verify new host; old host refuses. New native settings offsets
  differ, not a simple global RVA shift. Preserve semantic mapping evidence.
- Installer and evidence: work/ttr-latest-7eb72860; installed manifest
  evidence/ttr-latest-7eb72860-installed.json. Rollback:
  D:/TTWVR/Backups/TTR-before-7EB72860-20261112.
- Normal height/graphics/Link/preferences preserved except requested ammo toggle.
  No public release/push for this update; no physical-headset acceptance claim.

Physical pool showcase remains active. Final-world cue publication fixed false
 early contact: PID612 published tip matches final-eye pixels (about1px), and
correctly rejects a stroke with the visible cue short of the ball. Final guarded
observer/equipment code now targets7EB; physical player cue requires Equipment ESP.
A new native direct rear-shaft pickup helper is prepared; no fake object grab.
The full latest-host match capture PID4720 crashed during Sunny's lean; dump is
being investigated. Do not accept its frozen footage. A prior new-host Sunny
turn completed in PID42452 before the test helper failed on retired glove data.
Accepted equipment/Sunny sections exist in a51.8s mobile draft; corrected player
shot/full new showcase not delivered yet. Production uses released pool behavior,
not this unfinished physical-equipment candidate.

Status-document recovery: a failed default-Windows-encoding write truncated this
file; exact earlier text restored from recorded document transforms and preserved
baseline. Evidence: work/ttr-latest-7eb72860/docs-status-recovery-provenance.json.
Historical text below is preserved; all future writes must specify UTF-8 and
use an atomic replacement.

# Pool physical equipment and new showcase — active 2026-11-12

Development overrides only; normal MO2 profile and user saves preserved. No
new public release. Exact staged pool DLL SHA256:
1E0841B415C2BC1170BF9B1CC7B42F9CCF59046450B23B0DE31A8ED25ABB68CD.

- Real owned-asset MISC cues, rack, chalk and sixteen balls use native pickup,
  gravity pull, held ownership and release. Cues/balls have authored dynamic
  collision; original open rack Havok remains unchanged.
- EquipmentMotion now verifies actual native motion before returning success.
  Ball table ownership is keyframed4; pickup/free world is dynamic sphere2.
- Native pre-eligibility collision-ray observation releases only the exact own
  ball before TTR rejects a keyframed body. Table writes stop first. Failed
  motion changes remain paused and retry; no forced native grab is called.
- Player cue contact uses the true forward capsule center and measured radius,
  with focused no-early-contact, lateral-miss and one-shot tests passing.
- PID41832 passes rack/chalk native gravity pickup, same-reference hold/drop,
  cue-ball pickup, dynamic motion, physical placement and rules adoption.
  Short forward-hand flick fixes the former test's behind-body catch failure.
  Bootstrap now stages on clear floor: Sunny+Y100, not inside side table+X100.
- New cue, rack, chalk and Sunny footage has both-eye sampled visual review.
  Player contact capture is being synchronized to native pose/QPC; previous
  player footage is not accepted as exact visible-contact proof. A complete
  reviewed mobile showcase has not yet been delivered.
- Avoid Pool-Palm bone feedback for this bridge: clamped skeleton bones caused
  impossible controller positions. Use bounded native controller/grip poses.
- One load crash PID44444 has a native heap-allocation fault; source of earlier
  corruption is unproven. Do not claim stability acceptance from build success.

# Vertibird moving-hand contact correction — 2026-11-12

User correctly identified hand jitter in the delivered flight footage. This was
reproduced, not dismissed as encoding/tracking noise. Raw controller-to-world
positions stayed stable in cabin space; native contact corrections shifted by
70.63 units and the rendered wrist by49.18 during fixed-input forward flight.
Native151570 computed hand velocity from previous WORLD point117F900+10, so
vehicle travel entered controller-motion/collision history.

New vehicle-vr/contact_frame.hpp mirrors the exact C565A6C2 native snap-turn
history transport, using currentCarrier*inverse(previousCarrier) before the
original15B1A0 contact routine (guarded caller3BCED1). It carries solved/raw
points, orientation, damping vector, the40-slot recovery ring and optional
held-pose history; manager correction vectors rotate and correction matrices
conjugate. Cached native yaw is synchronized to prevent duplicate yaw transport.
The original contact solver still runs. New mount/root identity or rejected
transport uses the native contact-only reset14F6B0 before reseeding. No actor,
inventory, save or unequip transaction is added. Final scope is flying only;
car behavior retains its prior path. Shared-page hook writes now finish before
protection restores unwind in reverse.

Validation: x86 build and frame math tests pass, including wrapped ring indices,
translation+rotation, preserved real hand travel, invalid ring/scale rejection
and a negative stale-history fixture. Same live sequence before PID45824 and
after PID50352: fixed head/controllers while stationary, climbing, forward,
reverse and turning, then opposing head/hand motion. All10 after phase/hand
gates pass; forward wrist span0.0267units max vs49.18 before; raw/target span
0.0144units, no erroneous contact correction.3147carries,0misses,1mount reseed.
The known-bad baseline fails the same gate. See tools/check_vehicle_hand_parity.py
and private work/vertibird/classic/vertibird-hand-*-result.json.

Both-eye capture46.43s/1393frames has0black frames/1repeat. Both-eye contact
sheets and full forward-motion frame analysis reviewed; left-eye right-hand
motionp95 dropped25.60 to0.073pixels/frame in the measured ROI while console
stayed stable. This is simulator motion evidence, not physical-headset comfort
certification. Optional held-item history is transported per native semantics
but armed flight has not been independently accepted. General cross-thread
conversion snapshot race remains a separate unproven concern, not the measured
cause fixed here. Do not extend this flight-only result to all vehicle behavior.

Final installed development DLL SHA256:
E614B87207154A1B6B3DB1FB1B4AC6C658EC37442B532AABC889A3CAE649ACAD.
Final build differs from captured candidate only by excluding the car branch;
review confirmed the captured flight path is unchanged. No public release.
New short mobile proof: outputs/vehicle-hand-fix/
Filthy-Casual-Vertibird-Steady-Hands-Mobile.mp4,43.47seconds/1304frames,
720x1280 H264/AAC. Private mobile proof link:
https://drive.google.com/file/d/1qzAv7pr_jqglqmsAq1kqZtx1nhWucE26/view?usp=drivesdk
The prior full-tour Drive video
predates this fix and should not be called current moving-hand proof.

# Vehicle synchronization / cockpit integration — active 2026-11-12

Development overrides only. Normal mod lists, user saves and global OpenXR
runtime unchanged. Exact supported host remains C565A6C2 / PE6AF4F66C; newer
4C45D6E5 is NOT installed. No current vehicle candidate is a public release.

Native mounted position/rotation conversion, source-publication seat refresh,
physical boarding-yaw calibration, root invalidation and TLS setup isolation
are implemented. Frame math/ABI reviews and x86 build passed. New seated sampler
uses the actual retained SitChairRelaxIdleC third-person sequence, bounded game
time, native interpolation/quaternion conversion and exact-build per-node world
cache release. Only pelvis and legs are changed, after native parent publication;
head/arms remain tracked. No player hiding, scaling or fading.

Flight PID1148 (continuous-native-seat.log) and PID23648 (full-lining-motion.log)
passed live seated animation, both knees forward, feet near floor, climb,
translation, free look/turn, native landing, survival and on-foot control restore.
The first idle can finish normally while the retained native lower-body sequence
continues. Code now requests once per actor/root identity and handles a rebuilt
root; that rebuild path has build/review evidence but no dedicated live reset
acceptance yet. Early cleanup matches this request and preserves unrelated idles.

Car PID45572 (supported-seat-car.log) passed steering, native acceleration,
20/20 both-hand samples, braking, exit, survival and seated publication. Both-eye
footwell captures now show the body ON the bench instead of intersecting it.
Original bench surface at driver XY is Z57.53; native pelvis offset36.85 and the
verified chair contact offset6.99 produce rootZ27.67. Eye stays independently97.
Original chassis supports sit aroundZ27–33; exact sole contact is partly occluded.
2553 matching seated samples/applies, zero immediate or local rewrite error.

Six instrumented callback buckets (including seated sample/apply) averaged a
combined0.331ms per publication over2669 samples, including bounded QA IO in
outer callbacks. This excludes conversion hooks, native render/body work and
unscoped diagnostic observer work; NOT a total frame-time or90Hz headset claim.
Private performance snapshot: work/vehicles/supported-seat-performance.json.

The final local cockpit geometry now uses 23 shapes /38,033 triangles, zero
collapsed/inverted fitted faces. Restored all20 original console housing faces
behind all184 controls; removed superseded bounded interior window returns while
retaining the exterior-derived center/cross ribs. Fourteen small cut-post loops
are capped using original UVs. No player hiding/scaling/fading. Original Fallout
materials remain. Build evidence: work/vertibird/classic/final-canopy-build.log.

Current complete recordings: car PID45572; before360 and native boarding
PID48920; resumed cockpit lookaround/flight/landing/after360 PID42160. The first
long runner hit its erroneous300second watchdog clamp after boarding; clamp is
now900 with explicit owned-process protection, and only the unfinished takes
were re-recorded. No obsolete FO4 footage is used. Both circles include all8
completed viewpoints. Recorders assert the encoded movie covers Finished.

Source media: car47.53s/3repeats; before96.57s/0; boarding56.1s/9;
flight89.7s/1; after96.83s/0. Black frames occur only at native boarding and landing
fades. Source contact sheets cover both eyes every4seconds; decode and cadence
checks cover every frame. This is simulator footage; physical headset comfort,
full normal-speed human temporal acceptance and dedicated rapid-reset behavior
remain unproven. Do not label the entire candidate perfect or headset-certified.

Final requested mobile edit: outputs/vehicle-showcase/
Filthy-Casual-Classic-Road-and-Rotor-Mobile.mp4,374.97seconds,720x1280,30fps,
full uncropped left eye, Fallout-inspired captions/wipes and original score.
Source hashes/build matching and completion checks are recorded in
showcase-source-validation.json. Full master decode passed11,249frames. Mobile download transcode also fully
decoded11,249frames,61,253,312bytes,720x1280/30fps/H264+AAC. Uploaded in place
to the existing private Drive preview after original109.5MiB exceeded connector
100MiB limit. Download SHA256 cce5d20d625da2ea77e0a013e7976a6b7eef50eab05965b3dd2d521c282ae4a3.
Drive: https://drive.google.com/file/d/1FTnvummpgmab7rZoqPLG6TXOsDkzfpTy/view?usp=drivesdk
No public release was created.
All owned game processes closed; MO2 selected profile restored to Seated Custom;
personal SeatedControls.ini SHA256 remains D17B65AD...02F278B. New vehicle
candidate remains private development; no public release or normal-profile
installation has been claimed. Derived game assets remain private.

# Classic Vertibird canopy direction — 2026-11-12

User selected classic Fallout 3/New Vegas appearance and requested wider forward
windows / partly transparent roof while retaining the aircraft identity.
The new FC-27 procedural silhouette is rejected; do not install or publish it.
FO4 remains a historical private prototype, NOT the chosen dependency path.

New source-only tooling: vehicle-vr/classic-vertibird. Local preparation extracts
classic FNV hull and matching textures, reads installed VPH interior/console,
records source hashes, and fails on missing texture references. 14 shapes /
24,971 triangles, zero missing textures. Generated game-derived files are private
under ignored work/vertibird/classic. No game files enter public deliverables.

Actual Blender model study: canopy-exterior.png / classic-canopy-proposal.blend.
Widened forward and overhead panels follow the source roof surface, retain the
classic silhouette and source UV weathering, solid center/cross ribs and fitted
rubber seals. This is a MODEL PROPOSAL, not installed or runtime-certified.
The stock VPH cockpit has very narrow front slots; original inspection eye was
above them. Lower eye test confirms visibility limitation, not an acceptable
final cockpit. New canopy needs a fitted inner liner, panel/seat alignment,
NIF alpha/depth validation, both-eye flight verification, and animation retention.
Do not merely install the static exterior: native animated aircraft rig must stay.

No game launched or normal profile/INI changed in this design pass. Vehicle
hand retention, proper seated driver legs and visible companion seats remain
unfinished. No new vehicle release published; existing Drive video is the older
FO4 private prototype, not evidence of the selected classic canopy.

# Authentic Vertibird and working Roadster — 2026-11-12

Private replacement video verified on Drive: https://drive.google.com/file/d/1FTnvummpgmab7rZoqPLG6TXOsDkzfpTy/view?usp=drivesdk
81 seconds,720x1280 H264/AAC,20181994 bytes; all2430 frames decoded,
zero black gameplay frames. Current video shows the actual FO4 cockpit and driving.
Normal Seated Custom profile restored after MO2 finished exiting; INI hash D17B65AD unchanged.

User rejected the generic cockpit and missing driving demonstration. Replaced
private development cockpit with the ACTUAL locally owned Fallout 4 VR Vertibird
geometry/materials. Source-bound conversion: seven shapes, 38,582 vertices,
40,171 triangles, original UVs, reconstructed BC5 normal Z, D3D9 mip chains.
Source scripts in vehicle-vr/owned-vertibird; proprietary inputs/outputs remain
ignored in work/vertibird/fo4 and local MO2 development override. No game assets
are in public deliverables. Model seat calibration now uses FCPilotSeatRest.

Flight PID44608 passed native climb, forward flight, landing, survival, control
restoration; actual stereo capture 2171 frames /72.37s /zero repeated frames.
Reviewed full 2-second contact sheet and full-size cockpit/side/flight captures.
Rear headrest occludes much of rear view. Hands dip during forward acceleration,
then recover; not a final all-motion acceptance. Visible passenger seating,
rotor animation/parked exterior continuity and live instruments remain unfinished.

Roadster: measured 51cm native arms vs ~85cm shoulder-to-wheel distance. Local
seat moved 30cm forward and25cm down. Also fixed rigid geometry retaining its
SKINNED shader flag and included BOTH original steering-wheel pieces. PID6304
passed two-hand rim contact, physical steering, real throttle/movement, braking,
B/native exit and survival. Full-size captures now show a complete rim and hands
in contact while turning at rest. Grip retention during acceleration still needs
work; the driving assertion proves steering/throttle/exit, not continuous hand
attachment on bumps. Read vehicle-vr/README.md for controls and limitations.

Combined mobile edit outputs/vehicle-showcase/Filthy-Casual-Road-and-Rotor-Mobile.mp4
is rendered and reviewed; private preview only. Source current Vertibird capture
metadata corrected from helper's wrong development-core path to actual override.
New captures use explicit CaptureAddonPath. Normal play/release binaries unchanged.
All test games closed; no save writes or global XR runtime changes.


Status maintenance note: a Windows encoding write failure truncated this document.
Historical text below was reconstructed from Git and this project's recorded edits;
raw edits, successful/failed operation audit and unmatched historical patch contexts
are preserved in work/vehicles/status-recovery-*. No source code or saves were lost.
Current vehicle results above come from the recorded runs, not reconstructed notes.

# Vertibird night showcase - 2026-11-11

Drive (metadata verified): https://drive.google.com/file/d/1FTnvummpgmab7rZoqPLG6TXOsDkzfpTy/view?usp=drivesdk
Local outputs/vehicle-showcase/Filthy-Casual-Vertibird-Mobile.mp4: 51 seconds,
720x1280 H.264/AAC, 9,707,855 bytes. Original retro graphics/music; actual final-eye
footage from PID22060. All 1530 frames decode; zero black gameplay frames;
audio peak .260. Stereo source 2167 frames, one repeat. Edit presents left eye.
Video shows cockpit/free look/climb/flight/gesture/native landing. No QA text.
This is a private development showcase, not physical-headset/release acceptance.

Original cockpit: vehicle-vr/build_cockpit.py and export_cockpit.py. Blender/NIF/DDS
under assets/vertibird-cockpit. Widened canopy improves actual forward view.
import-vertibird-local.py attaches our mesh to the locally owned aircraft skeleton;
never redistribute that dependency-derived skeleton. Native flight script 0045B6
GetAngle is scoped to player/active flight: physical head is free look, right stick
controls heading. Native climb, flight, completed landing, pilot survival and
on-foot control restoration passed PID50008 and PID22060; games closed normally.
Native movement-disabled flag blocks TTR hand publication; scoped movement-enable
restores it. New freeGrip vehicle compatibility built from public baseline plus
this tiny change in work/vehicles/stable-core; unrelated ammo WIP excluded.

Remaining: hands dip below view during forward flight; rear viewing still blocked
by pilot headrest despite lean. Visible companions in new seats not implemented;
original companion transport retained. Cockpit controls/instruments modeled but
not all interactive. Do not claim perfect or fully playable cockpit.
Roadster steering tests failed. Arbitrary seat adjustment reverted. Latest fix:
source wheel skinned to Bip01 Spine with nonidentity inverse bind; old importer
incorrectly attached under Scene Root. New FCWheelCarrier preserves source inverse
bind under matching bone, and interaction uses actual wheel world/carrier basis.
Latest test PID22164 in work/vehicles/wheel-bind-showcase.txt still failed hand contact; no car drive pass. The bind-frame correction is retained as source-correct but insufficient.
Pose probe PID23900 confirms input translation arrives; stopping SyncPos or enabling
all controls did not uniquely fix attachment.

All vehicle work remains isolated. Normal play profile, saves, personal height
settings, global XR runtime and public releases unchanged.

# Vehicles and physical-reload follow-up WIP - 2026-11-11

Normal Seated Custom profile and public releases remain on prior tested 1.1.2.
All new vehicle/ammo code is isolated; do not publish as a finished release.

Vertibird Player Home 4.18, TTW patch 4.18c, Pilot Inside: installed separately.
Audit: work/vertibird/installed-dependencies.json (hashes/masters/overrides).
Pilot Inside adds an interior activation seat, not an in-flight cockpit.
vehicle-vr/plugin.cpp adds scoped native-command inputs for the piloted actor.
Quest: left stick movement, right stick yaw, A climb, X descend, B native landing,
Y HUD; triggers original weapons; stick clicks weapon selectors.
Field flight PID32248 passed forward movement, ascent, completed landing,
pilot survival and on-foot control restoration. Initial distant-location run
ended in aircraft destruction; not a landing pass. No headset acceptance yet.

User explicitly requests real cockpit while flying. import-vertibird-local.py
locally imports cabin and console onto the animated fuselage and uses authored
pilot-chair coordinates for PlayerPos. No dependency assets may be distributed.
MO2 priority corrected in run-vertibird-qa.ps1. PID49012 showed the cockpit and
passed native flight/landing, but windshield is obscured by overlapping surfaces.
Current bounded probe isolates BodyMain and VertibirdHull01:2 culling via
sim-session/vehicle-visual-mode.txt, and captures opposite head directions.

Car entry/exit works; hands-on-wheel gate failed. TTR stops publishing hand bones
when original car disables controls. Enable-look-only probe PID7108 also failed.
Need isolate native pose gate; no frozen-hand or successful-drive claim.
Local Roadster wheel import is development only. No new video delivered yet.

Physical ammo WIP: recovery now enters native pouch-held initialization, including
loose grenade rounds; pouch no longer restores stale saved placement. Grenade
gravity pickup/pouch return and ground pickup/insertion/fire passed. Partial
10mm recovery/reinsert passed. Second fixture load/manual toggle crashed in
nvse_1_4.dll +78056, c0000005; diagnose before installing new core.
Unfired ejected rounds and new pool case/rack/chalk remain pending.
No personal saves or global Link settings changed.

# TTR 1.1.2 updates published - 2026-11-11

User explicitly authorized publishing all current updates. Source main:
filthy-casual-ttr 506f6cd8e69ec576271e49c056594ce0ab804aba;
ttr-meta-link 618247e (full commit in evidence/github-current-release.json).
Public releases: v1.6.0-dev.2, pool-v0.2.1-beta, Meta v1.1.2-beta.
All three are prereleases targeting exact TTR1.1.2 C565A6C2... .
All uploaded ZIP SHA256 digests/sizes and tag/main commits verified via GitHub API.
Archives audited: explicit manifests, clean source commits, exact installed/tested
DLL hashes, licenses and validation docs; no upstream game/TTR/Meta/pool files,
locally generated upstream scripts, personal settings, saves or private runtime.
Pool remains standalone with original Pool Tables Of The Mojave v4 plus TTR.
Public compile checks and unit/Meta guard checks passed. Runtime coverage remains
as recorded below; no additional physical headset test or complete tutorial claim.
Index: https://github.com/nikamigaming-create/filthy-casual-ttr/blob/main/docs/ALL-MODS.md

# TTR 1.1.2 installed - 2026-11-11

User corrected requested version to 1.1.2. Installed from Downloads/TTWVR (3).zip.
Exact DLL SHA256 C565A6C296B597F89FCF84D7F6001D47D1635792F0E8CACAD92C6A88F6823E73,
PE timestamp 6AF4F66C. Four upstream files changed; 763 other files unchanged.
Ported canonical exact-build adapters (47 moved addresses), rebuilt SeatedControls,
SeatedInput, LocalMetaLink, Pool and isolated simulator. Unknown builds remain refused.
Static mapping and private source-before backup: work/ttr112 (not public assets).
Offline mapped-image guards passed; no upstream code executed by that guard test.
Control/tutorial-data/model/ejection and cue unit suites passed.
Isolated MO2 game PID50604 passed combined core, physical magazine, dialogue,
pool and scope checks. 520 scope restoration attempts, 520 successful, zero failures.
Game closed normally; QA lists restored; MO2 selected profile Seated Custom.
Inspected sampled final-eye pairs. Full temporal visual review and physical Meta Link
headset acceptance for this new version remain unproven; no perfect-visual claim.
Installed exact tested production DLLs + upstream hosts/meta; all hashes verified.
Personal INIs preserved, seated INI D17B65ADDA67EE2385B779841741F0AB51592CE7425086A9A304A8FC102F278B.
No personal save writes or global Link/runtime changes. Existing normal startup:
D:/TTWVR/Play TTWVR - Seated.cmd (MO2 Seated Custom -> Tale of Two Wastelands).
Rollback files: D:/TTWVR/Backups/TTR-before-1.1.2-20261111.
Evidence: evidence/ttr112-installed-compatibility.json, ttr112-runtime-check.txt,
ttr112-guard-check.txt. GitHub prereleases still target TTR1.1; this upgrade is local.

# Current mods published - 2026-11-11

User explicitly requested all current mod work pushed and GitHub links.
Public source main verified: filthy-casual-ttr 908e4bc072483cdecbad5f393ff7cef1a6c9ab28;
ttr-meta-link 8ede20784055 (full SHA available on remote).
Released TTR 1.6.0-dev.1, standalone Pool 0.2.0-beta, and Meta Link 1.1.0-beta.
All target exact TTR 1.1. Uploaded asset sizes and SHA256 digests verified.
Index: https://github.com/nikamigaming-create/filthy-casual-ttr/blob/main/docs/ALL-MODS.md
Seated package uses exact installed/captured core DF772108... and input52481E...;
public-source build verified separately. Pool/Meta public code sections match
local compatibility builds. Package hashes are in evidence/github-current-release.json.
Tutorial coverage remains incomplete (Fat Man and other special/heavy lessons).
These are current-work prereleases, not new full headset acceptance claims.
Private QA runtime, saves, dependency binaries and personal graphics presets were
excluded. No local game/runtime/settings changed for publication.

# Coverage correction - 2026-11-11

User stopped the automatic goal, then clarified the concern is coverage, not a
continuous walk-through. Do not treat normal-speed watch-through as the blocker.
The Fat Man and other special/heavy weapons are absent from the delivered edit.
Current evidence/tutorial-coverage-current.json records the actual 583 metadata
entries, their part signatures, thirteen selected chapters, and twelve open
coverage categories. Metadata count does not prove runtime coverage.
Fat Man is the first missing lesson; flamer/recharger currently fall back to
unmapped in manual tutorial mode. No new game launched for this coverage audit.
The existing Drive video is a partial guide, not all-weapon coverage.

# Current tutorial delivery and acceptance - 2026-11-11

Current mobile video: outputs/Filthy-Casual-TTR-Reload-Guide-Phone.mp4,
720x1280, 30fps, 618.667 seconds, 73754654 bytes. Caption revision 2.
https://drive.google.com/file/d/1unOavC-IglkdIYe9DlqJIFu8h1l24Zo_/view?usp=drivesdk
Same Drive file updated and metadata read back. Full phone decode passed.
Both fullHD and phone exports now contain corrected captions, selected ammo
name (10mm Hollow Point), and clearer chest-height charger take PID42088.
No export or game remains running at the current audit.
Side-view trial PID50212 passed native reload/fire but worsened card occlusion;
rejected for delivery. Existing Drive video and selected source remain unchanged.
Angled trial PID28180 also passed reload/fire; independent comparison found no
instructional improvement over baseline. Both trials closed; Drive unchanged.
Reviewer considers baseline sufficient for the beginner sequence with ordinary
native wrist overlap, not a fully exposed view of final seating contact.

All 13 selected source hashes match the edit. Current package has 44 entries,
24 original cards, and 42 installed files match; personal INI unchanged.
Installed TTR hash matches supported 1.1 build. test-tutorial.cmd now checks
all four executable exit codes immediately; all four suites passed afterward.
Current package and capture evidence replace stale records from earlier takes.

Goal ACTIVE after resume; first fresh audit finds the same continuous-playback review gap. The final-eye review is sampled chronological/dense action review,
not proven whole-video normal-speed observation. Charger contact remains partly
occluded by the native wrist, and lever grip retains its native loose pose.
User final-say reply remains pending. Do not infer full visual acceptance or
physical headset approval. See evidence/tutorial-completion-audit.json for
requirement-by-requirement results. Prior entries below are historical.

# Mobile tutorial delivered; candidate installed - 2026-11-11

Mobile output: outputs/Filthy-Casual-TTR-Reload-Guide-Mobile.mp4,618.633s,
1080x1920,H264,30fps,silent,158916378bytes.13chapters cover activation,10weapon
examples,ammo selection andmagazine recovery/stow. Export55042 exited0 and
full-file ffmpeg decode passed. No game or encoder remains intentionally open.

Candidate installed and hashes verified as below; personal settings preserved.
User is receiving the download link now. Goal remains ACTIVE: complete
normal-speed visual acceptance/physical check not proven. Reviewed sampled
both-eye sequences and actual phone edit frames; do not claim perfect natural
hand animation. Native lever pose remains loose and its explicit cut is documented.
No public publication this continuation. See evidence/tutorial-review-current.json
and tutorial-install-current.json for current hashes and exact evidence.

# Tutorial installed; mobile export in progress - 2026-11-11

User clarified the deliverable is the representative WEAPON TUTORIAL, not the
pool/all-features montage. Goal remains ACTIVE, not final acceptance.

- Candidate core DF7721089E0915D099B90C2E824E52C19977B00F400C013D20A8084834E5605C
  installed into normal Filthy Casual TTR.42files hash-verified; personal INI
  D17B65AD... unchanged. Backup work/tutorial-dev/normal-before-tutorial.zip.
  Evidence tutorial-install-current.json. No global runtime or saves changed.
- Package Filthy-Casual-TTR-1.6.0-dev.1.zip verified44entries,24original cards,
  no original game/TTR/QA runtime content. Input layer .text matches tested normal.
- Pump33722/PID32368 PASSED native cycle, pickup/insertion, pump-ready card before
  shot, usable shot and capture. Reviewer confirms LOAD->READY->PUMP-after-shot,
  both eyes. This fixes the earlier misleading post-top-up cycle instruction.
- Lever23314/PID50916 PASSED native cycle and shot. Raw capture has a100ms pause
  before pull. Explicit edit keeps frames[0,405),[480,802), cutting stationary
  setup13.5->16s. Complete pull/return/fire retained at normal speed. Metadata
  retains original frames/repeats; kept spans pass cadence. Reviewer confirms
  visible cut and usable native-control illustration, but loose native grasp.
- Mobile export session55042 currently active. Poll before any replacement.
  13chapters: activation,10mechanism examples,ammo types,magazine recovery.
  Phone-size actual pistol/bolt/pump edit frames inspected: legible large captions,
  correct pump-ready instruction, visible world/hands. All sources1.1 captures.
- All game test runners33722,23314 are closed; no game intentionally left running.
  Normal-speed complete visual review and physical-headset acceptance unproven.
  Do not mark goal complete or publish as perfect. User requests finished mobile
  link immediately after export/check; keep video free of QA/disclaimer labels.

# Pool match mobile preview reviewed — 2026-11-11

Goal ACTIVE. Latest user request: quick mobile match against Sunny; judge the
video ourselves. A real same-match excerpt is rendered, but NOT approved as a
finished human-motion showcase. Do not claim the equipment fixes are complete.

Current output: outputs/Filthy-Casual-Pool-vs-Sunny-Mobile.mp4 (~25 seconds,
1080x1920, H.264, 30fps, silent). Source outputs/pool-match/sunny-match-stereo.mp4,
PID10208, TTR1.1 hash E428C4BD1209D58D21F75108808C86049E26C76E58A164AD864A61B99696BDAB.
Sunny makes shot1, native player turn starts, measured physical player stroke
makes shot2, B exits, game closes. No synthetic hit or independent practice
footage spliced into this match. Repositioning omitted with a wipe.

Review: source sampled at 1fps for Sunny and 2fps around player stroke; full-size
address eye and portrait frame inspected. Sunny is fully in frame, captions are
readable, cue/ball action visible. Rear hand remains below the lower frame during
much of the player stroke; bridge uses a closed grip, not a detailed open bridge.
Therefore acceptable as a working preview, NOT final polished promo approval.
Full continuous normal-speed visual review remains unproven: local browser
playback timed out and the temporary native player had no targetable window.

The capture solver now keeps a reachable native bridge palm and fits the rear
palm/92-unit cue through it to the cue ball, matching the earlier successful
practice algorithm. The old fixed bridge/rear targets exceeded native IK reach.
PID10208 passed at 9mm cue-line residual and 44cm hand separation. Higher-camera
PID32112 also passed actual stroke/clean exit; its framed source is retained but
rear hand still clips. Final pitch65 attempt PID11076 stopped before recording
with native mailbox timeout; no passing result claimed. No game left running.
All capture sessions72293,71427,69521,40733,35524 are terminal. Review servers5263,
68961 stopped. Temporary playback PID37740 launched with autoexit; verify cleanup.

Pending pool production work (no implementation this continuation): FC_PoolInput
blocks native grips/triggers, ACTI cue/balls lack Havok, saloon props inherit
Trudy ownership, active balls are overwritten by DrawBalls. Need native held/table/
world handoff plus player-owned usable cues/rack/balls/chalk. See POOL-PICKUP-PLAN.md.

Tutorial remains pending: grenade rifle reload9 classifier fixed; unit/build
passed previously. Grenade PID36204 native reload/shot passed; cylinder43548
reload passed; bolt29756 crossed wrist rejected; pump50112 insertion failed.
Outdoor recovery40096 glove/reinsert/accounting passed but later firing failed;
not pouch-return proof. Old6:52 tutorial remains draft. Normal install unchanged.

Personal INI D17B65ADDA67EE2385B779841741F0AB51592CE7425086A9A304A8FC102F278B;
normal core18E7333433AB6F8B02D814F59349BDD08284A944D21B854745F1738EAD0F2352
protected. No saves, global runtime, or publication changed.

# Mobile edit rendered; prompt and placement corrections - 2026-11-11

Goal ACTIVE. Previous conversational scope clarification was no progress; this
turn changed source, produced the full first mobile edit, and found visual/native
failures that determine the remaining work. Do not mark ready or install this
candidate into the normal profile yet.

Deliverable now exists:
- outputs/Filthy-Casual-TTR-Reload-Guide-Mobile.mp4,1080x1920,30fps,
  412.300s,93,699,771bytes. Actual native-speed outdoor final-eye footage,
  activation plus8 core representatives, original comic captions/wipes. Silent.
- work/tutorial-video/edit-manifest.json preserves source hashes/PIDs/events.
  It says pending review. No QA/runtime wording or disclaimer in the video.
- Event-boundary both-eye review sheets in work/tutorial-video/review.
  Selected sheets and full-size stills were inspected, NOT complete normal-speed
  temporal review. Phone text layout is readable. Core edit remains a draft.

Native evidence/corrections:
- Earlier handle61583 ended because LEVER failed, not hunting rifle. Authoritative
  bolt-native-cycle PID40068 is complete and has a usable follow-up shot.
- New reload_tutorial::settle prevents Ready while the action controls are held.
  Hidden/automatic/weapon changes bypass/reset this presentation hold. No native
  reload state writes. All4 unit suites +build passed. Live PID34376 explicitly
  passed cycle-action while trigger held, chambered round, and usable next shot.
  Its bolt-world-card recording is complete but card placement is not accepted.
- Tutorial card was following gun; pump footage showed it looming/offscreen.
  New anchor fixes world position/facing until weapon change/zone exit. PID20124
  passed fixed world coordinates through near-face gun motion and head lean.
  Both-eye images rejected the lateral placement as too far RIGHT. Source now
  changes side offset from+0.25m to-0.40m; needs build and final-eye check.
  Card height uses head-bone+0.15m; distance0.72m;zone0.8m. Outlines remain native
  part-relative. Diagnostics include tutorialCardWorld and tutorialCardAnchor.
- Card-placement fixture originally failed due to missing parent-scope helpers,
  then wrongly compared tracking coordinates while head lean changed their
  conversion. Correct fixture compares world coordinates; stills remain essential.
- New Start-Take provenance records exact installed core/native DLL hashes.

Exceptions still unfinished:
- Cowboy Repeater: bolt-style offhand-trigger pull did not chamber; A+wrist
  pitch also did not. Existing lever-guide wording is NOT accepted. No lever
  take included in core edit. Need real native control/motion proof.
- Chinese pistol: A does NOT eject a detachable magazine (installed remains0).
  Creator's slide+trigger+A lock, then pouch grip gives native mode1 with10round
  charger clip. Generic magazine tutorial is wrong for this family. Alignment
  and native refill still unfinished. No charger footage in core edit.
- Latest charger test now identifies held duplicate node by occurrence1, validates
  measured orientation, approaches above clip along its native Y axis then feeds
  down. tests/tutorial-loader-pose.py accepts optional node/heldOccurrence.
  Do not confuse screenshot sidecar JSON with addon snapshots: probe writes
  *-state.json now. Older same-name state files were overwritten by Save-Eyes.
- Current process handle15749: card-placement PASSED, bolt-world-card PASSED and
  PID34376 closed; charger probe next/running. Poll before starting another game.
- Native single-shot launcher/shared ammo type and recovery remain coverage work.

Safety/state:
- Normal core18E7333433AB6F8B02D814F59349BDD08284A944D21B854745F1738EAD0F2352
  and personalINI D17B65ADDA67EE2385B779841741F0AB51592CE7425086A9A304A8FC102F278B
  reverified unchanged. TTR1.1 E428C4BD... hash unchanged. No publication/save writes.
- Ghidrust MCP DLL string read was rejected by automatic review over an unverified
  connector destination. No bypass/retry; continued local native controller tests.
- Package ZIP predates these source fixes. Rebuild/repackage/verify AFTER accepted
  runtime/visual work. Normal tutorial installation remains pending.

# Concise tutorial scope and native magazine cycle - 2026-11-11

Latest user asks to teach unique actions, not all 583 entries. Confirmed and
updated docs/WEAPON-TUTORIAL.md: point, name control, perform motion, show result;
repeat only for a different mechanism. Local metadata:294 entries lack all five
reload-part fields. Others share capabilities; fields alone do not prove controls.

New progress:
- Pointing gap26cm avoids native approach grab blending. PID39824 final-eye
  stills show extended index/curled fingers, both forearms and both eyes. No rig
  changes needed. Full27s take reviewed at1fps in both eyes, NOT normal-speed
  acceptance. 806frames/26.934s,30fps,1repeat,exit0. pointing-second-pass-stereo.mp4.
- Original card generator now1024x384,40x15cm, fitted56px heading/48px body with
  bounds checks. PID36468 screenshots clearly larger/readable.
- New tests/tutorial-pistol-reload.ps1: actual native A ejection, pouch grip,
  alignment, insertion, release. Outdoor stage, ammo supplied BEFORE recording.
- PID48416 exposed guide bug: native pouch pickup enters installedMode2 directly,
  pouch.mode stays0. reload_tutorial.hpp now maps2 to InsertMagazine; test updated.
  NO native reload behavior altered.
- PID36468 passed empty well4 -> actual pouch pickup/insertion2 -> seated0,
  guide fetch-ammo -> insert-magazine -> ready. Real controller actions only
  during reload. All4 test-tutorial.cmd tests and build.cmd pass.
- outputs/tutorial-development/pistol-reload-prototype-stereo.mp4 plus timeline
  and recorder metadata. Both-eye held/finished stills reviewed. Development
  proof, NOT final public video. Chamber was already loaded, so no cocking proof.

Next: native slide/chamber false->true, remaining unique mechanisms, complete
motion/video review, packaging. Canonical build is tutorial DEVELOPMENT only.
Normal stable addon remains18E73334; personalINI D17B65AD unchanged. All sessions
closed latestPID36468, QA lists restored, normal profile selected. Goal ACTIVE.
No publication, user save writes or global XR changes.

# Optional weapon tutorial development — 2026-11-11

User refined scope: concise lessons for DISTINCT mechanisms, not583 demos.
Use free index finger to point at each part, pause, then do the native action.
Full mechanism matrix and beginner/mobile/video gates: docs/WEAPON-TUTORIAL.md.
Goal remains ACTIVE; complete tutorial/video is not finished or ready to release.

Normal installation is now stable compatibility-only core SHA256
18E7333433AB6F8B02D814F59349BDD08284A944D21B854745F1738EAD0F2352.
Includes32cm dialogue-name clearance and corrected inline assembly ejection
state121F13Ch(old1.0) ->17EE0C4h(1.1), both branches. Exact old/new mapping
already in work/ttr11/mapping.json. Real assembly executed against isolated
allocated memory in tests/ttr11-eject-state-unit.cpp: committed4, native5,
old-field sentinel unchanged and flag cleared. Compatibility-only source/binary
in work/ttr11/src. Backup work/tutorial-dev/compat-before-eject-fix.dll and
compat-*-before sources. User input DLL/settings unchanged. Normal profile has
NO tutorial. Canonical source/build is the tutorial DEVELOPMENT candidate.

Implemented in canonical source:
- hold_controls.hpp: triple Menu =>Tutorial action4;450ms deferred single Menu;
  double cancels; hold Settings; focus/menu cancels; wheel cancels queued reload.
  tests/hold-controls-unit.cpp passes including old wheel behavior.
- tutorial_data.hpp: bounded read-only TVPK handling.txt reader. Reads installed
  data directly; never bundles/extracts it. All truncations, malformed values,
  overlaps, duplicates tested. Actual1.1:583 entries,202 mag/cells,139 handles,
  91 ports,21 loaders. Unknown data refuses tutorial, native controls unchanged.
- reload_tutorial.hpp basic state machine and tutorial_adapter.hpp read-only
  chamberECCBA0 (identified native log/branch), root/state and measured anchors.
  Native GetEditorID resolves runtime base form through installedJohnnyGuitar.
- plugin.cpp: optional off-by-default triple Menu toggle; metadata/state lookup;
  read-only diagnostics; cleanup/hiding on lifecycle and menus.
- tutorial_visual.hpp + tools/build_tutorial_assets.py: ORIGINAL36 3D wire
  highlight edges and10 instruction cards. No collision/save records. WorldPin
  keeps geometry OUTSIDE native weapon tree so it cannot affect pickup bounds.
  IMPORTANT scene::update(pin.node) AFTER child moves rebuilds aggregate bounds;
  without it all geometry was culled despite37 valid elements.
- tests/run-tutorial-qa.ps1: copies candidate+original assets to ONLY
  'TTWVR - Tutorial Development', backs/restores QA profile lists and enables
  original carrierESP for test. Copies personalINI into probe; never edits normal.
- test-tutorial.cmd runs controls, parser, state model, real assembly branch tests.
  All passed. Build.cmd passed; no new warnings after local variable rename.

Runtime evidence (all bounded sessions closed):
PID28932: triple on/no reload toggle, actual pistol parts, ejection->fetch guide.
Stopped on wrong fixture EditorID WeapNVM4Carbine; corrected toWeapNVAssaultCarbine.
PID38776: seven representative weapons mapped to actual parts; on/off preserves
reload mode; native pistol ejection->fetch. Objects existed but first images
were invisible; these do NOT pass visual acceptance. evidence/tutorial-native-foundation.json.
PID13672: aggregate-bounds correction made actual green 3D highlights/cards
visible in BOTH eyes. Outdoor daylight stage: loadSC_QA_scope_autosave, GameHour12,
playerX-72500,Y2000; native gravity settles player. No user save writes. Card was
partly offscreen, then moved inward14u/farther10u/above15u relative gun/view.
PID25800: inward card visible; slow two-part pointing prototype captured in
outputs/tutorial-development/pointing-first-pass-stereo.mp4 (11.4MB) +timestamps.
Pointing changed no pickup/eject state. BUT actual stills show a reach/claw,
not clean index pointing; native approach blend at final pose was~0.47.
DO NOT ACCEPT/PUBLISH this prototype. Only stills reviewed so far, not full video.

Next concrete work:
1. tests/tutorial-point-pose.py now places finger26cm from part (was13cm) to stay
   out of native grab-pose blend. This change has NOT run yet. Next smoke writes
   pointing-second-pass and v2 stills. Helper solves actual finger bone direction,
   uses native index-off/thumbrest-on/grip1, slow1500ms curves and2s pauses.
   If free-hand middle/ring/pinky remain open far away, inspect queryGesture:
   freeGrip only supplied for modal masks; native pose may leave free offhand
   open while other hand holds gun. Preserve all native occupancy/approach gates;
   do not force a pointing pose through an actual held-item interaction.
2. Card text needs larger type/height for mobile readability. Current1024x256,
   native40x10cm; consider1024x384 and40x15cm with measured text bounds.
3. Basic guide currently generic. Family-specific prompts, cylinder/gate/pump/
   lever/bolt/charger distinctions and actual reload sequences still needed.
   Chamber true was observed; false->cock transition NOT verified. No proof yet
   of full fetch->insert->cock cycle WITH tutorial on. Data alone is not proof.
4. Native guide follows MainLoop world poses; attachment under motion needs full
   final-eye temporal review. Initial stills only; no exact frame-parity claim.
5. Film full lessons outdoors with natural pointing/action transitions. Review
   complete footage normal speed/both eyes, then mobile edit with requested motif.
6. Update package.ps1(oldSHA/version!), asset allowlist/docs; retain no owned data.
   Nothing published this turn. Current public release unchanged.

Game closed afterPID25800. QA lists and normal profile selection restored.
PersonalINI hash stillD17B65ADDA67EE2385B779841741F0AB51592CE7425086A9A304A8FC102F278B.

# Dialogue name clearance installed — 2026-11-11

User requested existing name floating clearly above the head, no obstruction or
second name, with dialogue choices kept beside the person. Retained native name
pixel split and native side placement. Raised head-joint offset from22cm to32cm
because the close final-eye capture showed the previous label touching the hair.
No dialogue-choice position change in this adjustment.

Installed SeatedControls.dll SHA256:
1A81CEC82A095DA08E65F98B5AF068873B8F521E380361CFC207E7BDF5066DFD.
Rollback: work/dialogue-name-clearance/SeatedControls.before.dll.
Personal INI remains D17B65ADDA67EE2385B779841741F0AB51592CE7425086A9A304A8FC102F278B.
Input DLL remains compatibility baseline8721A83D; source unchanged this pass.

Bounded PID37584 checked three distances/both sides and exposed hair contact.
Final PID45196 checked60/180unit distances, both sides, head movement, native
name crop, one name layer at the actor anchor and removal on conversation exit.
Both-eye stills reviewed: evidence/dialogue-clearance-*_sbs.png; state matrix
in evidence/dialogue-clearance-matrix.json. Clear air gap in inspected views.
This is simulator proof, not fresh physical-headset acceptance. Tests close the
game; QA profile selection restored. No saves, graphics or control bindings changed.
Old dialogue-name-layout fixture no longer presses A into barter after greeting.
Tutorial/video goal is still active and not completed by this dialogue repair.

# Recovered magazines /1.5.1 local candidate —2026-11-10

User requires normal world pickup, gravity gloves, throwing, reinsertion and
ammo-pouch return. Implemented guarded native full-initializer entry28A422→28C491;
no direct clip writes. Exact source weapon/ammo identity in JIP reference maps.
Empty uses installed state2 path (not spare max1). Worldpart remains ordinary
until gun contact; pouch waits for intentional gameplay grip release.
Scene contact qualified together on MainLoop, ref/hand/weapon bound, consumed
once with50ms deadline. Reading live scene inside native reload was a real phase
mismatch; fixed by same-publication contact snapshot. No forced insertion flags.

Matrix PID11752 passed empty save/load+insert+pouch, rifle23+1 and plasma32
including native gravity-glove flick. Earlier pistol5+1 passed. Public candidate
PID28448 rifle insertion+fire passed, later second gravity pickup failed and is
excluded from video. PID28476 plasma gravity pickup→insert→fire→pouch passed.
Independent reviews caught explicit inactive/menu guard and consumed-ref map
cleanup; both fixed. FINAL public binary PID27664 passed partialpistol5+1,
repeat eject/pouch, exact300inventory, and all3RefMap sizes0. Source unchanged
after that test except docs. User saves untouched, only isolated fixtures used.

Video reviewed both eyes; hand/body obscure exact seating/pouch socket. Keep
27–34 insertion continuous, exclude rifleafter40 and initial native popups.
Rifle master has3shutdown repeatedframes only at53.74–53.8; selected0–40 spans
have0repeats. Plasma master has0repeats. Mobile edit normal speed, fullright eye,
gamma1.35, no generated gameplay or QA labels. Original metadata retained.

Public1.4.5 remains unchanged.1.5.1 release and video are local for review.
Install/cleanup and exact hashes recorded in outputs/magazine-recovery/delivery.json.
Rollback baseline1.5.0 goes to work/reinsert-rollback, separate from olderrollback.

# Physical reload parts local beta - 2026-11-10

1.5 beta feature is implemented and tested against the exact supported TTR build.
Original carrier + native world ammo references, fitted dynamic box collision,
private locally-derived appearance cache, native pickup/throw/Take, save/load and
cell-return association restoration. No custom throw or global ammo hook.

Coverage:10mm pistol,Assault Carbine,Plasma Defender,Laser Pistol,.357 Revolver,
Single Shotgun. Fitted matrix earlier failures were fixture contact errors;
carbine/plasma passed corrected native-contact run PID28988. Pistol/laser/revolver/
shotgun had passed their fitted runs. Loaded/empty ejection and normal save/load
passed; empty pickup/throw preserved ammo PID44860. Cell return passed PID27288
with final restoration guard. Native Grip+A extraction/release passed pistol
PID41272(11rounds) and carbinePID33724(23rounds), exact single debit. Native pouch
return, automatic reload and X Take passed final-film PID33252.

Independent native review caught pending-placement, early-held and reference
recreation edges; all fixed. No new grab hook was needed. Native controls and
contact offsets remain original. Source diagnostics are read-only, test mailbox
only. Physical-headset comfort and runtime missing-dependency/cache negatives
are not certified. See docs/PHYSICAL_PARTS.md for limits.

Portrait video:outputs/Filthy-Casual-Physical-Parts-Mobile.mp4,42.1seconds,
1080x1920/30fps,7.3MB approximately. Reviewed rifle/plasma both-eye masters,
zero repeated source frames. Phone edit has normal-speed footage, original comic
cards, gameplay-only wipes and intact captions. No QA/runtime labels in frames.
Silent. Sampled temporal review; not a claim of perfect human grasp.

Public v1.4.5 remains unchanged; new beta is local for review. Package and install
hashes are recorded in outputs/physical-parts/final-delivery.json. Tests touched
isolated fixture saves only; personal INI and normal profile must remain restored.

# Published with user approval — 2026-11-09

User explicitly approved publication. Reviewed source commit
`ecd580b7aa00127e106f9b7bc3cc756ebdc6c3b8` pushed to public main; tag v1.4.5
resolves to the same commit. Release is PUBLIC, marked beta/prerelease:
https://github.com/nikamigaming-create/filthy-casual-ttr/releases/tag/v1.4.5

Assets: Filthy-Casual-TTR-1.4.5.zip, independent optional
Filthy-Casual-Pool-0.1.0-beta.zip, Filthy-Casuals-Mobile.mp4, SHA256SUMS.txt.
Anonymous HTTP checks returned200 for the page and all four download URLs;
uploaded SHA256 digests match the reviewed local files. No messages sent to
third parties. No game launched, user saves/settings unchanged.

This supersedes the draft/publication-pending notes below.

# Standalone pool + seated compatibility, release draft — 2026-11-09

Latest user scope: Pool is standalone and optional. It requires working TTR and
Pool Tables Of The Mojave v4, NOT Filthy Casual TTR or our Meta Link helper.
When combined, Filthy Casual TTR >=1.4.5 is required to avoid legacy double input.
No original mod files may be redistributed. The public pool ZIP excludes Start,
Tick and PlanCandidate; Prepare-Pool.cmd derives them locally from four verified
upstream files. Installer outputs exactly match tested scripts. Wrong versions
are rejected before any output; original files remain unchanged.

Native standalone PID34316: SeatedControls.dll/SeatedInput.dll absent, dedicated
FilthyCasualPoolInput.dll present. Physical stroke, tracking-loss rejection,
B exit and native camera passed. Combined PID40092: same pool checks; preserved
height0.65 and reload mode; both wheels with all five fingers on both hands;
no accidental grab/draw; equip, reload toggle/restore and settings open/close.
The earlier combined test failures were baseline timing/scope errors in the
harness; corrected post-load baseline run passed. No product patch to reload
or height was needed. Games closed; personal INI byte hash unchanged.

Installed verified binaries:
- Core EBBA70BD6E492042D195D9259580ED444608043556D33CDB9AC482BE94FB01B6
- Input 786A06552DB2B7E953FD3905B13EE1429D85C4DFC10C839FE228A3AB0E70DCA6
- Pool 863638DFFF129BBA7783E7E813AC4F3DB6F7438AA96DE673EF99930879CC7D1A
- Pool input 1595ACA366F07149982BA2852C452335D528C53E467CF2555FF01359CE16E80D
Obsolete shared SeatedInput removed from the pool mod; temporary test/probe mods
removed. Normal MO2 profile remains selected. Older local-test ZIP aliases now
point to current curated release packages, not upstream-containing staging ZIPs.

Public checkout D:/code/filthy-casual-ttr-public is clean at LOCAL commit
ecd580b7aa00127e106f9b7bc3cc756ebdc6c3b8. Public source builds, focused layer,
thumbrest controls, sticky-food and cue tests passed. Source is NOT pushed yet.
GitHub draft v1.4.5 has both ZIPs, mobile MP4 and checksums; uploaded digests
match local files. Draft URL:
https://github.com/nikamigaming-create/filthy-casual-ttr/releases/tag/v1.4.5

Awaiting the user's requested final publication approval. On approval: fetch and
verify no intervening remote changes; push the prepared source commit; set draft
target to that exact commit and publish as a prerelease. Do not silently publish
with the old main source. Video shows both addons, not pool-only features.
Physical headset comfort/full-match limitations remain in docs/VALIDATION.md.

# Current local build and mobile showcase — 2026-11-09

Supersedes the rejected animation and earlier installed-build notes below.
Final player-film pickup PID48376 also passed one physical hit, tracking-loss
rejection and native B exit using the normal installed files; game closed. The
higher camera and relaxed follow-through keep the table visible in the phone edit.

Normal MO2 mods Filthy Casual TTR and Filthy Casual Pool now contain the exact
binaries exercised in the final combined native session. The temporary Stroke
Test override was removed after every staged pool file matched its installed
copy. Personal SeatedControls.ini restored byte-for-byte; height 65 cm and
personal reload setting preserved. Normal Seated Custom profile remains selected.
Game closed; user saves untouched; no public publication.

Installed SHA256:
- Core: EBBA70BD6E492042D195D9259580ED444608043556D33CDB9AC482BE94FB01B6
- Input: A56208A8616B0BEE1EE2AB1217B5CBF5ACD8BD762CB51AC62DA28E083E12425E
- Pool: A65568741EB2C7D065F49F47A50BC485325BFC469B6D4A6FDE4717B1B27B7644

Verified locally:
- Revised original 7.2-second pool setup/address/stroke/recovery, correct rear
  grip and lowered bridge thumb; measured native Sunny scale 0.95; no IK reach
  clamp and no transition jump above the 4-degree/10ms bound across 11 cases.
- Actual native hand/cue-tip crossing starts the ball simulation, rather than
  a timer. Final Sunny full motion captured in PID48664: 450 frames/15 seconds,
  zero repeated frames. Native repeat challenge and player physical stroke,
  tracking-loss/reacquisition rejection and B exit passed in PID40920.
- PID48664 food regression: A/X safely return the held instance; mouth gesture
  consumes exactly one sticky-held food and applies normal healing. Squeeze back
  to native holding disables mouth consumption. Shared native slot aliases can
  legitimately contain equal NULL context pointers; release both aliases using
  the native release/swap path before activating the held reference once.
- Saved height verified across fresh process launches; current wheels, cap clear,
  reload switching and native-hold handoff captured. Input samples preserved for
  hand animation. No new save hooks, inventory recovery or unrelated Link changes.

One earlier native worker crash (PID40504, 00AAA60A) occurred during a dialogue
transition before pool idle/contact. Root cause is not established. The owned
handoff now requires native dialogue target NULL and menu clear continuously
for 1.5 seconds, with an 8-second timeout; subsequent first/repeat runs passed.
Do not present this as proof that all native crashes are eliminated.

Local archives: outputs/Filthy-Casual-TTR-local-test.zip and
outputs/Filthy-Casual-Pool-local-test.zip. Portrait video:
outputs/Filthy-Casuals-Mobile.mp4; source/cut/cadence manifest alongside the edit
under outputs/human-showcase/mobile-edit. Current footage is real final-eye
capture with choreographed XR input; gestures reuse the accepted unchanged take.

Remaining acceptance boundaries: physical-headset reach/comfort, a complete
match through final eight-ball, and save/restart during an active match are not
certified by these runs. Public licensing/asset audit and user approval remain
required before publishing the optional pool package. This is a local playtest.

# Sunny pool animation rejected - 2026-11-09

User reviewed the stroke video and rejected its technique/realism. This
supersedes the visual acceptance wording below: mechanics pass, animation
quality FAIL. Do not describe Sunny as polished or fully dialed in.
Installed local build is unchanged; no new game was launched for this review.

Source inspection: author_sunny_pose.py holds the torso/legs in a fixed solved
stance, uses a sine-cycle rear-hand pull, and applies generic finger curls.
It lacks a properly authored setup, address, aiming pause, stroke, follow-through
and recovery sequence. Bone contact alone does not establish pool technique.

Specific external motion sources located (not acquired or quality-approved):
- WondAR Studios, Billiards shot: 6-second FBX; publisher describes Xsens body
  capture plus Manus gloves. Fab listing dd6416d8-44c9-4d9e-b072-e7c51db400a2.
  Preview: https://sketchfab.com/models/f46aa66fb3a340da9aebc8d86e19cab4/embed
- Motion Cast #30 Vol.2 Everyday Life Outdoor: publisher lists ten casual
  billiards clips, FBX, within its larger collection. Unity asset389830.
  Current listing $25 before tax; no purchase authorized or made.

Next animation revision should use an inspected/licensed body-and-hand motion,
retarget locally to the owned FNV skeleton, constrain bridge contact to the
actual cloth/rail, keep cue/grip aligned, synchronize physical impact to the
animation contact event, and blend into/out of the stance. Review visible feet,
head gaze along the cue, elbows, bridge fingers, rear grip and follow-through
from close front/side views and both submitted eyes. Do not generate another
large pose bank before one reference stroke passes visual review.

# Public beta releases and showcase - 2026-11-08

Published the two independently reviewed, asset-free source snapshots:
- https://github.com/nikamigaming-create/filthy-casual-ttr/releases/tag/v1.4.4
- https://github.com/nikamigaming-create/ttr-meta-link/releases/tag/v1.0.0

Clean release checkouts: D:/code/filthy-casual-ttr-public (71c9ec2),
D:/code/ttr-meta-link (c089f0c). Both public beta release assets were verified
against GitHub SHA-256 digests. ZIP manifests enumerate every bundled file.
The DLLs are byte-identical to the installed tested builds. Portable public
builds and startup/control unit checks passed. No personal settings, game or
Meta DLLs, saves, private runtime, or machine-specific launchers are bundled.
Inactive environment-gated diagnostics remain in the tested addon source.

Corrected Quest 3 infographic is in both README files and release assets.
Independent release review found no blocking issue; its missing-image package
finding and singular Menu wording were fixed before publication.

The new 1080p comic showcase is LOCAL FOR USER REVIEW, not published.
Actual final-eye gameplay with choreographed XR input; no speed changes or
interpolated frames. Curved hand trajectories use cubic Bezier offsets with
minimum-jerk timing; existing quaternion interpolation handles orientation.
Two initial takes: 2165 + 1747 frames, zero repeated source frames, 30 fps.
QA PID34052 closed; personal height65.4cm, manual reload and quiet hints restored.
Visual review requested a side-on pointing pickup shot and more precise ammo/
clear-hand captions. No claim of independent five-finger tracking or physical
comfort approval. Ordinary held-food use remains unverified.

# Development checkpoint — 2026-11-08

Canonical checkout: `D:\code\ttwvr-seated-controls`.

## Independent free hands and opposite-stick touch 1.4.2 - 2026-11-08

User physically rejected 1.4.1's global held-item veto and thumb/fist appearance.
The plugin now gates only gameplay/transition readiness; each native pose call
checks that hand's weapon/support policy, held/transition slots and Stimpak hand.
The shared weapon object pointer no longer suppresses a free opposite hand.
Native authored weapon wrapping/profile paths remain native. Exact-base helper
RVA1DB4C0 and18A630 contracts were independently reviewed and instruction guarded.

Index touch plus grip closes the free-hand index without a trigger pull; lifting
the index preserves pointing. All four thumb touch sources are aggregated. No
contact extends the thumb; grip blends toward thumbs-up. A first full-base thumb
abduction looked exaggerated and the user rejected it. Final shape limits the
metacarpal to12degrees, shares extension with the MCP joint, and derives mirrored
radial direction from the installed hand's own index/pinky bases. Softer thumb
curl keeps the thumb over the fingers. Original hand roots, bone lengths, mesh,
scale and native three-finger grip animation are unchanged.

User's new controls require opposite thumbstick TOUCH: mask17 left rest + right
stick touch for consumables; mask10 right rest + left stick touch for weapons.
One second, no click, either arrival order. Losing the modifier cancels a timed
hold and requires a fresh wheel-thumb contact. Both rests cancel. Menu tap reload
and hold settings remain unchanged. Unit tests cover timing/order/cancel/rearm.

PID10408 isolated native tests passed contact aggregation, fist/thumbs-up without
trigger pull, both selector combos, free-left articulation with right pistol,
unchanged occupied-hand pose counts, native firing, and the same checks with
right-hand Buffout. Candidate thumbs-up silhouette was rejected; PID27788 then
verified final open/relaxed/fist/thumbs-up/point poses after the shape correction.
Final stereo natural-thumbsup_sbs.png shows the thumb closer to the palm without
the prior stretched web. Both sessions closed. No save commands. Personal INI
restored; height37.2cm, automatic reload retained. Physical acceptance pending.

Final build/installed SHA256:
- SeatedControls.dll:106EF7D86727FD81ADBC8ED11CB0BE48F1663190BB345BF741D38D98B765033C
- SeatedInput.dll:4EAC033A3781B2AA91CAAFE52F75E2A7A367EFEEFBD8B9F5CC33B5966651C537
Local package:outputs/Filthy-Casual-TTR-1.4.2.zip. No publication.

## Active 1.2.2 revision — 2026-11-08 (local, not published)

Brand is **Filthy Casual TTR**. Installed folder and launcher paths now use
`D:\TTWVR\MO2\mods\Filthy Casual TTR`. The obsolete `TTWVR - QA Runtime`
config-only mod was archived under ignored `work/archived-qa-runtime-20261108`
and removed from all MO2 mod lists at the user's request. Root NVSE already has
runtime tests off. All 16 normal mods remain enabled; Local Graphics stays a
separate mod. Showcase screenshot filters to Filthy Casual only, then the filter
is cleared. Normal profile is `TTWVR - Seated Custom`. Height remains 37.2 cm.

Ordinary aid now retains its grip latch through native menus; Y explicitly uses
one held food/ordinary aid through native stow and inventory-use. No save hooks,
new inventory fabrication, or lifecycle recovery. The initial targeted aid-menu
run passed 17 assertions, including native pause retention, one Y use/effect,
intentional food release, Stimpak trigger/body use, and full-stick run. Actual
NPC dialogue was not reproduced; its headset check remains pending.

User then requested optional native holding and manual reload. A fresh right
grip squeeze now relinquishes the convenience hold for weapons and aid; guns
also cancel menu redraw intent. Current focused native-grip run passed short-A
reload, long-A live manual/automatic toggles once per hold, no accidental short
reload on long hold, native gun release/holster without redraw, food release,
and Stimpak grip takeover/release. Native Stimpak hand became -1. The first run
caught boolean bridge replay being suppressed; fixed, then the focused rerun
passed. Both runs closed the game.

Short A is replayed on release; long A (1.2 seconds) changes the actual cached
native bManual setting, shows a deliberate status message even with hints off,
and persists AutomaticReload in addon INI. The startup preference is applied
once after native initialization. Native hooks initialize regardless of mode.
`reload_adapter.hpp` holds exact-hash live-setting and reversible torso-pouch
offset adapters. Current default pouch offsets: forward .25 m, centered,
below-eye .45 m. Runtime placement flag passed, but first screenshot was partly
blocked by the left arm and did not clearly show native spare ammo: final visual
reach check is still in progress. Do not call pouch readiness proven yet.

New base comic guide rendered at `outputs/Filthy-Casual-TTR-Guide.mp4`, 219.6 s,
1920x1080/30fps, silent, actual full right eye with simulated controllers labelled.
Original generated comic artwork is attributed in docs/assets/ART-PROVENANCE.md.
Source footage: 3300 frames/110.049 s (one repeat), 2121/70.747 s (four repeats),
969/32.349 s (zero repeats). Edit excludes the end-of-first-take shelf bump and
the unsuccessful jab in the food take; the third take shows the successful
seated native jab. Exact puncture occurs below view, not an arm-contact closeup.
Source1fps and assembled2fps contact sheets plus full-size stereo stills were
reviewed by root and independent reviewers. Normal-speed full-video review and
physical acceptance are unproven. Review caught title-lettering padding and an
incorrect actual-game footer on the illustrated end card; source fixes are in
place, not yet rerendered. New live-toggle/grip/pouch chapter is still pending.

Do not publish. Finish new pouch visibility/reach, append new controls, rerender,
update final hashes/status, commit, regenerate the clean-source ZIP, and preserve
the user's final review before any public release. Old sections below are history.

## Folder-launch failure — 2026-11-08 09:22 PST

The user's folder launcher produced a native Steam Error dialog containing
`Application load error 5:0000065434`. Game PID 13292 was blocked before addon
initialization; the addon log still belonged to the previous QA run. Steam,
Explorer, MO2, and the game all had non-elevated tokens. After dismissing the
failed launch, closing MO2, and gracefully restarting Steam, the exact unchanged
`D:\TTWVR\Play TTWVR - Seated.cmd` launched successfully as PID 14212. Steam tracked
app 22380, addon 1.2.1 reached script initialization READY, and its OpenXR input
bridge connected to SteamVR. Renderer isolation was not requested. The agent
issued no load command. Gameplay subsequently loaded during verification; a
graceful close request did not exit the game. The watchdog was canceled before
its deadline to preserve the active session, which was left running for the user.
This supports a transient Steam-client launch failure;
it does not establish the internal cause of the stale client state. No addon,
graphics, game executable, or MO2 executable-entry changes were needed.

## Revision 1.2.1 — final headset test candidate

Built and installed SeatedControls.dll SHA-256:
`747ED7E1B8B1FD84DB37FFD2C7D951F236EB463CE4804D539973CE27033CE880`.
SeatedInput.dll SHA-256:
`6676DEBACABAF42091A08BD15AEEB287F6BAA14E312EEEF96F22C17A4B18D68D`.

The selected weapon is remembered in memory and native draw is requested again
after native holstering, only while the same weapon is equipped and gameplay,
dialogue, death, and combat-control guards permit it. Cap selection and retouch
holstering cancel this intent. No save hooks, inventory recovery, or native
adapter addresses were added. A reloads the right-hand weapon with panels closed.

The final installed build passed all 10 assertions in tests/weapon-resume.ps1:
draw, A reload from 5 to 12 rounds, actual pause-menu open/close, same-weapon
return, combat-disable inhibition/resume, and persistent explicit cap clearing.
Actual NPC dialogue was not reproduced by the fixture; that physical test remains
pending. The bounded runner closed the game. No full legacy suite was rerun.

A direct MO2 launch without a launcher runtime environment was also verified:
the configured process-local SteamVR runtime applied, input bridge connected,
and SteamVR created the swapchain. That check reached the menu, not a performance
benchmark. It supersedes the pending startup verification in the historical entry.

Graphics are local and separate. The installed TTWVR.ini override was moved from
the addon into `TTWVR - Local Graphics`, enabled above the addon in both local
profiles. Package inputs exclude it, FalloutPrefs, and SteamVR settings. Native
rendering remains UNVERIFIED: even Upscaler=Off and DLSS/bEnabled=0 still started
the helper. The normal launcher does not enable SC_TEST_NO_UPSCALER. The simulator
uses that explicit test-only override. Local MSAA is 2, per-game SteamVR scale is
67 with global 1.5 (about 100% effective), Meta ASW reports off, and SteamVR reports
motion smoothing unsupported/off. No native-frame-rate or headset clarity claim.

Production height remains 37.2 cm and popups remain off. Package defaults remain
height zero and no runtime path. Version 1.2.1 is the current package candidate;
earlier ZIPs and guide footage are historical. No Git remote or public repository
has been configured. Publication awaits the user's final headset test and approval.

## Direct-MO2 launch repair — 2026-11-08 07:56 PST

User reported MO2 crashing and no VR from the ordinary Run button. Windows
recorded MO2 2.5.2 failing in Qt6Core+3061F5 at 07:49; it did not recur during
the subsequent normal-window launch. The Qt crash's root cause is not proven.
That normal Run launch started the game but did not connect to SteamVR. Both
registered OpenXR runtimes pointed to Virtual Desktop, while the working CMD
launcher explicitly selected SteamVR. No global runtime registration changed.

At the user's request, SteamVR/game were restarted through the existing working
launcher. Game PID 23148 connected the addon input bridge, SteamVR created a
2944x2208 swapchain, and the game subsequently loaded gameplay and logged real
center-cap interaction. The game was left running for the user, intentionally.

An optional `[OpenXR] RuntimeJson` was added to addon configuration. It sets the
game-process XR_RUNTIME_JSON before XR initialization only when no explicit
environment override exists. Blank remains the portable default. The user's
installed INI specifies SteamVR's steamxr_win32.json, preserving their 37.2 cm
height and quiet popups. Build passed; installed hotfix DLL SHA-256 is
`97B8F9F6CBF4FBD64E08E00F56876063C3FE575CBD79AC29ECAAF3764B9C81BA`.
The mapped prior DLL was preserved outside plugin discovery in work/ and the
new file installed for the next launch, without interrupting the user's session.
The next direct-MO2 launch still needs runtime verification. The existing 1.2
ZIP below remains the previously tested build; this launch hotfix is local only.
No MO2 executable entries were rewritten, and no saves were changed.

## Supported build

TTWVR.dll SHA-256:
`C9D047D4199C0458281D341AC9DC438A7C928999E19CDAD6F327F8A439A7F9EB`.
PE32 timestamp `6AF014A8`. Unknown hashes are refused. Current adapter addresses
and their validation boundary are recorded in `adapter-evidence.txt`.

Revision 1.1 selection-only SeatedControls.dll SHA-256:
`128723EFB7F2B8019E0B492DF5CE80D1BF13511D10748ED2B01C3841AB5A90B3`.
This was the previously installed/package binary.

Revision 1.2 SeatedControls.dll SHA-256:
`294DD126BD2CE934430DD0DD9852A4F7876663C83C0C00FF39C90784E8B54475`.

## Revision 1.2 — center cap, quiet messages, local graphics setting

The user tested 1.1 in the physical headset and reported that interaction seemed
okay. Their supplied 222-second recording was reviewed with a coarse contact
sheet spanning the recording; the original file was preserved. This is not a
claim of complete frame-by-frame acceptance or acceptance of the new 1.2 cap.

- Both selectors now have a 12 cm center cap on every page, including empty
  inventory. It uses the owned game's Caps001 model, turned face-forward and
  scaled from rendered mesh bounds. No game asset or currency is copied/spent.
- Touching the cap closes the selector, holsters weapons, clears live-Stim, and
  returns grabbed aid via the existing native full-stow path. No save changes.
- `[UI] ShowSelectorPopups=0` suppresses addon hints and selection/dismissal
  notices in both MessageExAlt and the XR panel. Y in the height panel toggles
  and immediately persists this preference. All height messages remain enabled.
- The user's active MO2-generated TTWVR.ini was set to DLSS Performance. Only
  its `Upscaler Level` changed to Quality, backed up in ignored work/. Resolution
  remains 100%, sharpening 75%, SteamVR scale 150%. This local graphics change
  is not shipped in the addon. Headset clarity/frame timing still need checking;
  the isolated simulator deliberately disables upscaling.
- The user's 37.2 cm height offset is preserved; package defaults stay zero.

Focused validation: build.cmd succeeded. The first center-cap run passed 25
checks covering weapon/aid/Stimpak clearing, counts, both touch hands, paging,
quiet messages, the Y toggle, and height messages. It exposed a null-array
empty-inventory bug, then closed the game. After fixing that path, empty-cap.ps1
passed 10 checks for populated and empty wheels, paging, and cap dismissal.
Final popup-panel.ps1 passed five checks after shortening a clipped label.
No broad legacy acceptance suite was rerun. Every bounded QA session exited.

Stereo final-eye captures in evidence/cap-* show face-forward caps with spacing
from the outer previews and quiet selectors. The final height-panel captures
show the full Y toggle/status text in three readable lines, ON and OFF. The
existing guide video remains a labelled revision-1.1 demonstration; it does not
show the new controls. New cap reach and DLSS Quality need physical confirmation.
The 1.2 ZIP and installed DLLs match the current build. Nothing is published.

## Current behavior

- Left stick opens aid; right stick opens weapons. Release leaves the wheel open.
  Either hand can touch; selection goes to the native right hand and closes it.
- World-fixed, compact previews use mesh touch tests. A/X page, B closes, and
  moving away dismisses the selector.
- Center bottle cap selects nothing through native holster/stow. Selector
  notifications default off; Y in the height panel toggles them. Height messages
  always remain visible.
- Weapons use staged native holster/equip/draw, retained grip, and the verified
  automatic-reload default. Retouching the drawn weapon holsters it.
- Ordinary aid transfers one actual inventory instance through the engine drop
  path into TTWVR's native grab. Instance extra data is passed through; the held
  object retains authored scale. There is no custom consume action.
- Stimpaks use native live-Stim selection and native trigger/body-stab use.
- Explicit item replacement uses native full two-slot grab cleanup and returns
  grabbed items to inventory. This can also stow a left-hand grabbed object.
- Height calibration uses OpenXR reference-space offsets. Opening its panel does
  not stow held aid. The shipped offset is zero.
- The input layer is discovered/enabled for the game process by the addon;
  no global OpenXR runtime or registry change is required.

## Save/lifecycle boundary

The addon has no SaveGame handler, AuxVar marker, custom co-save data, item
recovery, or AddItem fallback. TTWVR and the game own native held-item saving.
Pre-load/exit discard addon pointers and previews without activating/stowing
native objects. Menu/focus loss does not automatically stow native held aid.
An interrupted pre-handoff transfer relinquishes addon tracking of its world
reference. Selection replacement and failed-handoff rollback remain supported.

A held ordinary item is already outside inventory as an actual world reference.
Inventory count alone after loading does not establish item loss. This addon
does not promise automatic inventory restoration across native save/load.

## Verification at this checkpoint

The prior native-handoff revision passed all 27 checks in `tests/acceptance.ps1`
in the isolated QA profile: touch equip/holster, native pistol fire/reload,
right-hand aid and authored scale, one-instance transfer, replacement/backpack
return, native Stimpak consumption, physical walk-away dismissal, and height
calibration/reset/input isolation. The two-slot release fix was included.

After removing custom save handling and automatic menu/focus/height stow,
`build.cmd` completed successfully. Source audit found no remaining save marker
or recovery path. Those 27 earlier checks are not a rerun of this final binary;
the final demonstration below covers the narrowed selection-only binary.
Experimental recovery tests do not establish native save behavior and are not
release acceptance evidence. The abandoned combined save/pose test was removed.

## Revision 1.1 local deliverables and recorded checks (historical)

The installed binary and the MO2 ZIP match the SHA-256 above. Actual MO2 archive
installation succeeded in the normal `TTWVR - Seated Custom` profile with the QA
helper disabled. The ZIP has `NVSE` at its root and does not contain that helper,
the private runtime, launchers, game assets, or save files.

`tests/capture-guide.ps1` passed 13 assertions on the final binary, covering
touch closure, intended/right-hand pistol selection, automatic reload, native
right-hand aid at scale 1, backpack stowing, native Stimpak handoff, height
calibration/reset, and recording completion. A targeted natural-pose weapon
take passed two additional weapon/reload checks. No save/load commands were
used in the recordings after loading the isolated fixture.

Source captures: 2,100 frames / 70.052 wall seconds and 510 frames / 17.076 wall
seconds at 30 fps, 2064x1104 final composited stereo, zero repeated source frames.
The guide retains the complete right eye and uses a reviewed upright wrist
pose. It is a captioned/silent guide; simulated controllers are labelled. This
demonstrates natural demonstration poses, not a change to TTWVR's hand rig.
Final source and visual reviews used two independent reviewers. No publication
has occurred; the user retains final approval.

Local deliverables are in `outputs/`: the ZIP, MP4, and `guide/mo2-installed.png`.
Capture metadata/timelines remain beside the source clips. Every test session
closed the game; the runner also has a PID/creation-time/path watchdog.

At this earlier checkpoint physical confirmation was pending. The user has
since tested the native-handoff revision and reported it seemed okay, as noted
in the 1.2 section above. Simulator results alone do not establish physical
acceptance. Preserve user saves and the original mod rollback archive.
