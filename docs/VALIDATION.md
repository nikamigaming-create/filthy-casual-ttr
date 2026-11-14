# TTR 1.1.5 / Filthy Casuals 1.8.0-beta.2 - 2026-11-14

Installed and hash-verified locally; five personal INIs preserved, game closed.
Final core SHA-256: `8bac9a9baaefaedafaf28fe1aa954ccc9fd622e10c6dd317b08e5bdfedb769c2`.

Compatibility port for upstream TTWVR.dll SHA-256
`E14567BC1C7D183130F902F9489B8CFFE6269DE06EEA0D79BF8D1D3098C595B1`
(PE timestamp `6AF8509E`). Older/unknown hosts refuse integration. Keep beta 1
for its supported older TTR 1.1.2 DLL. Update Core and optional Vehicles together.

Passed fresh isolated native-game checks with simulated XR input:
- Weapon/aid touch selection, center deselection, native right-hand item scale,
  automatic reload, firing, calibration and settings close without input leakage.
- Recovered magazines: ordinary pickup, insertion, empty chamber retained until
  native slide operation, repeated pickup and pouch return without duplicate ammo.
- Kebab, iguana, Buffout and whiskey consumed once; mouth follows head rotation.
- Live remapping, profile save/load, defaults and recovery; native Pause/Back and
  physical finger curls preserved. Offline checks cover all 70 input sources.
- Dialogue name and panel placement, stable anchors during head turns, scoped
  weapons at three distances; exact-build scope/HUD guards passed.
- Native driving and steering grip, takeoff/movement/climb/landing, separate car
  and flight seat offsets, restored ground height and controls after exit.
- Bighorner body hold, 30-second challenge, steering/run/idle and dismount.
- Glass impact projection on desert LAND and road geometry; final-eye captures
  reviewed. Earlier sign/fade tests are historical, not a new every-surface claim.

A final-eye review caught premature bottle breakage during staged native pickup.
The pending replacement is now excluded from impact judgment until handoff ends.
The regression test explicitly checks the empty reference, native ownership and
correct bottle base; consumption alone is not sufficient proof. Near-wall pickup
and four bottle families passed in session PID 17296, including native release
and continuous rendered physics with zero observed render stalls. Actual impact
effects still fired after release. The session closed normally.

Evidence: runtime-basic.log, runtime-combined.log, runtime-vehicles.log and
runtime-finish.log under private work/ttr-e14567bc. Earlier vehicle failures in
those logs were superseded by corrected adapter and fresh-state tests; final
session PID 35224 exited normally. Public source builds independently.

Limits: this is beta acceptance in the simulator, not new physical-headset comfort
certification. Deathclaw-specific behavior and every controller family were not
rerun physically on 1.1.5. Cockpit geometry remains the existing beta geometry.
Pool is excluded. Original game/mod assets and the private Meta helper are not
part of the public release.

---

Historical checks follow; their host/version limits still apply.

# Current 1.8.0-beta.1 release candidate

Fresh core/input/vehicle builds pass. Focused mouth, 70-input remapping, typed INI,
and 30/60-second creature challenge unit checks pass. Current native checks pass
repeated recovered-mag pickup/insertion/pouch return, no automatic chambering,
native slide cycling, four ingestible families, separate car/flight seat saving,
ground restoration, driving, takeoff/climb/landing, Bighorner body hold/run/idle/exit.
Core SHA-256: 1F2C930882B28DAA9C08015B7BBB60DFACEF58F7EF255BEC50C2B0204988509C.
See [current beta coverage and limits](CURRENT-BETA.md). Earlier entries below are historical.

# Controller profiles and typed INI access — 2026-11-13

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
- Final montage capture refresh improves model lighting/view angle. Physical
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

# Split car controls and mounted selectors - 2026-11-13

Installed locally; manifest evidence/car-axes-selectors-installed.json.
work/vehicle-controls/car-axes-selectors.log (PID35056) passed:
- Left Y forward/brake/reverse, right X steering; inactive axes tested negative.
- Actual thumb-rest combos open both wheels during native vehicle travel.
- Weapon previews remain in reach and open while the car moves; physical touch
  equips the native pistol in the right hand, then real trigger firing works.
- Free left-hand touch selects a drink into the native right hand without use.
- Reopening while holding an item, B close without dismount, then B exit pass.
- No accidental native pause; test game closed. Both-eye selector images reviewed.
- Mounted preview forward distance is capped at 30 game units to stay inside the
  seated arm reach; on-foot spacing and the physical touch radius are unchanged.

The original longer reach failed the small consumable contact check. Earlier
bot paths also crossed the center cap or overextended the gun hand. The final
check uses a direct approach and the free hand, retaining real native selection.
Control-profile unit checks pass (car-controls-unit.log). No personal INI,
profiles, saves, Link settings or public releases changed. Headset feel remains
for the user's acceptance; simulator results are not an all-items certification.

---
# Vehicle control correction - 2026-11-13

- work/vehicle-controls/frame-final.log: native hand offset sentinels, frame
  transport and signed throttle/brake/reverse logic pass.
- work/vehicle-controls/menu-corrected.log (PID27816): formerly failing arm,
  grip and touch sequence stays outside MenuMode; final stereo reviewed.
- work/vehicle-controls/flight-native-combat.log (PID30912): actual aircraft
  displacement/climb, physical controls and independent release, native rifle
  bolt cycle then firing, landing/survival and restored controls pass. Both-eye
  mounted-native-bolt-back capture reviewed; test game closed.
- work/vehicle-controls/car-bottle-final.log (PID12316): four drink families
  preserve hand pose and native squeeze/release; car real travel/steering,
  wheel attachment, no Pause and native gun fire while driving pass.
  The reverse assertion was invalid (GetVariable optional argument is a quest,
  not a vehicle ref). It is corrected to an object-context native UDF query.
- work/vehicle-controls/mounted-final.log (PID40428): corrected reverse and B
  exit pass. Its subsequent extra flight test failed selection because a world
  anchored wheel was left behind during travel; this remains a documented
  selector limitation and is not claimed fixed here.
- Extra manual reload checks exposed a real ownership conflict: an already-held
  magazine squeeze was reacquired by the flight handle during travel. Vehicle
  ownership is now latched only at a fresh squeeze started near its control.
- Native bottle replacement errors are below 0.75 game units / 0.06 matrix-element
  delta in all four families. SetAngleEx/SetPosEx synchronizes Havok as well as
  visible geometry; scene-only placement failed and is no longer used.

- work/vehicle-controls/flight-native-reload-final.log (PID37396): real native
  magazine ejection, pouch pickup, insertion and subsequent shot pass while the
  aircraft traverses over 15,000 units. MenuMode remains zero; owned game closed.

- work/vehicle-controls/tutorial-flight-final.log (PID38612): native reload/fire
  passes again during over 10,000 units of travel. Tutorial uses committed cabin
  frame, publishes after native poses and retains the same cabin-local anchor.
  Reviewed both-eye capture across pickup/insertion/ready transitions: reading
  card remains fixed relative to cockpit while terrain moves; native hands visible.
- work/vehicle-controls/tutorial-foot-final.log: current final core passes
  on-foot head/hand motion, fixed reading-zone anchor and clearing on holster.
  Earlier fixture attempts lacked manual reload metadata; not product regressions.
- First mounted recording's generic helper named an obsolete tutorial profile
  in its addon provenance. Marked invalid rather than treating it as a hash proof.
  QA wrapper now explicitly supplies its staged DLL path to capture metadata.

No all-weapons or physical-headset certification is implied by these checks.
See current installation manifest for exact DLL hashes and rollback directory.

---

# Local bottle checks - 2026-11-13

Installed core SHA256 A143540AF56DFC14CC9086EEF1CCE9737D7A8444E232AEF2D2F8A2C51F1A94D8.
Upstream TTR 7EB72860, xNVSE 6.4.9, isolated OpenXR simulator/MO2 profile.

- Impact-rule negatives pass: flight, gentle contact, glancing slide, co-moving
  surface, held/caught state, stale samples and replaced bodies remain intact.
- PID40596: gentle/hard native contacts and all four consume-to-empty families
  pass native right-hand identity, inventory counts, squeeze and release.
- PID36756: real native pickup and gentle release leave the bottle intact.
- PID34604: recorded native grip release throws the bottle onto ground; actual
  collision produces glass decal, small particle puff and positional sound.
- PID27420: three native throws hit actual Stop-sign geometry (confirmed by
  surface node and horizontal contact normals), rather than the ground beyond.
  Final-eye video shows glass on the sign and its engine-owned timed fade.
- Audio: loopback waveform matches the owned FXMeleeGenericGlass recording with
  normalized correlation 0.966 at 0.860s in evidence/bottle-impact-audio.wav.
  Delivered footage contains captured game audio, common volume gain only.
- 41-second H.264/AAC video, 720x770, real-time 30fps; source recording metadata
  includes frame identity, repeat counts, audio synchronization and owned PID.

The earlier gun diagnostic only isolated the decal API; it is not bottle proof
and its hook/commands are removed from the plugin. Earlier pending-queue counts
were incorrectly treated as active decals; that counter is now glassPending.
Original artwork only is installed. Game sound/particle assets load from the
owned game; no copied game assets, global graphics changes or save hooks.

Installed profiles, backups and exact hashes: evidence/bottle-impact-installed.json.
Physical headset acceptance was not performed in this turn.

Vehicle changes remain WIP as described in STATUS.md; this bottle test does not
certify wheel grip, knee animation or combined armed driving. Earlier release
validation below is historical and should not be applied to that candidate.

---

# Validation - 1.7.0-dev.1

Checked 13 November 2026 against TTWVR.dll SHA-256
`7EB72860D209D9B9BDD774607220EE63323367AA9FA525F6EB1B23184F30F91E`.
Unknown builds are refused. Windows x86, isolated MO2 profile, real game with
scripted OpenXR controller input. No personal saves or global runtime changes.

| Area | Result |
| --- | --- |
| Controls model | Strict profile parsing, analog preservation, gestures/chords, defaults, release barriers, external-edit protection and input loss passed |
| Modes and offline editor | Three advertised modes, inactive-mode editing, mode switching, absent-provider preservation, profile v3 migration and offline import/export passed |
| Real game controls | Pointer record/apply/cancel; native Pause/Back; saved profiles; fixed recovery; raw finger input separate from remapping passed |
| Driving | Grip/steer, throttle, brake, exit and return to on-foot mode passed |
| Flight | Native takeoff, movement, climb, landing and return to on-foot mode passed; native menu input preserved |
| Recovered magazines | 10mm pistol and assault carbine: native eject, ordinary grip pickup, native reinsertion, second pickup and return to native pouch passed |
| Ammo accounting | Pistol recovered 5 rounds; carbine recovered 23 rounds. Exact inventory debit/credit on both cycles; four handoff commits, zero failures; pouch return spawned no extra magazine |
| Presentation | Populated controls, driving, flight, weapons, aid and height panels captured in game; both eyes reviewed |
| Tutorial | Triple-Menu activation and native-world part outlines/card presence passed |

The recovered-magazine bridge does not spawn drops, write clip counts, fabricate
ammo or replace save handling. Native TTR owns those operations. It validates
reference identity, count, model, compatible held weapon and native hand mode,
then hands the recovered object back to native reload initialization.

Coverage is bounded: this is a development prerelease, not exhaustive headset
acceptance. Gravity-glove retrieval has the same held-reference entry path but
was not separately exercised in this round. Empty magazines, every weapon family,
arbitrary custom bindings, physical headset comfort and long-session/save-load
recovery have not all been certified. Vehicle visuals remain development work;
the vehicle provider is source-only here and requires separate native mods and
local asset preparation. Pool is optional and excluded from this core package.

Reproducible checks: `tests/check-controls.cmd`,
`node tests/controls-offline-unit.js`,
`tests/native-worldmag-recovery-build.cmd`, and
`tests/native-worldmag-recovery-static.py` against the supported owned DLL.
Private live runners: `tests/controls-vehicles.ps1`,
`tests/controls-vehicle-ready.ps1`, `tests/native-worldmag-runtime.ps1`.
Local accepted logs: controls-vehicles-run (PID 27516), controls-ready-run
(PID 30108), final-controls-recovery (PID 23276), robco-final-photos (PID 32216).
Runtime assertions, rather than the earlier overwritten screenshot-sidecar JSON,
are the magazine-count evidence. The sidecar naming collision is corrected.
