# Revision 1.1 acceptance plan

All tests use TTWVR - Addon QA, native game commands, and per-process bvr-xrsim32.
No user saves. Captures must come from the final compositor, both eyes. Keep a
bounded sequence of current images; use actual wall-clock intervals, not a
synthetic video cadence. Simulator results do not prove headset comfort.

| Claim | Positive / negative test | Required evidence |
| --- | --- | --- |
| Runtime | Launch new hash and OpenXR session | Fresh PID, FOCUSED, advancing frame counters |
| World-fixed wheel | Open/release; head yaw/translation; page; B close | Both-eye sequence with world and six visible models; anchored transforms; absent after close |
| Touch equip | Approach with grip zero; touch each hand; switch; retouch drawn weapon | Native equipped form and weapon-out state; visible held model; no selection outside threshold |
| Input isolation | Held trigger on touch, page/B/stick release, joystick dismissal | No unintended shot/jump/VATS; normal movement resumes |
| Held aid | Either-hand touch -> native right grab; move; switch; backpack release | Authored scale; one actual inventory instance transferred, restored on stow; no custom consume action |
| Live Stimpak | Select -> native right hand; grip-release; trigger-held body stab | No dose consumed on selection/put-away; exactly one on native use |
| Firing | Pistol and rifle empty/fire/reload | Native ammo counts, visible held firearm, automatic continuation |
| Height | Seated 1.1 m -> target 1.65 m; step/reset/save/restart | Effective head/hand poses and final eyes; saved offset; no stale space errors |
| Lifetime | Close/reopen, focus loss, save/load, cell change | No preview leftovers/crash; cleanup and reopening |
| Headset | User seated reach, comfort, button feel | Physical user acceptance; unproven until tried |

Record per-claim pass/fail/unproven and artifact references in the final report.
