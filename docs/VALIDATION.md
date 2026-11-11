# Current TTR 1.1 snapshot validation

This is a prerelease checkpoint, not complete weapon-tutorial coverage.

- Public seated, input, pool and Meta source builds compile.
- Four seated tutorial/control suites passed. Pool cue unit tests passed.
- Meta API/FOV/scope/shadow guards passed against the exact installed TTR 1.1
  mapped image, without executing game code. Unknown hashes remain refused.
- Seated release DLLs are the exact installed/captured binaries: core
  DF7721089E0915D099B90C2E824E52C19977B00F400C013D20A8084834E5605C;
  input 52481E040D1A91127BF7B843AD194AEE1AC726916F493741E7ECE7DA528E4DD1.
  The public compile check is separate; byte-identical reproduction is not claimed.
- Pool and Meta rebuilt code sections match their local compatibility builds.
- Local TTR 1.1 simulator checks exercised selectors/fingers, aid handoff,
  reload mode/height controls, recovered-magazine reinsertion and pouch return,
  dialogue placement, scope surface restoration, and pool stroke/exit.
- The tutorial video covers ten weapon examples plus activation, ammo types
  and recovery. Fat Man, missile/heavy weapon and other special lessons remain
  open. See WEAPON-TUTORIAL.md and TUTORIAL-COVERAGE.json.
- Native hand poses can obscure final clip contact. Loot-target acquisition,
  a full pool match and new physical Meta headset acceptance are not certified.

No saves, original TTR/game/pool files, private runtime or personal graphics
presets are included in the downloads. Original dependencies are linked.
