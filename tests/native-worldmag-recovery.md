# Native world-mag recovery: integration and acceptance

Integrated in plugin.cpp. Live pistol/carbine recovery, native insertion and
pouch return passed on 2026-11-13. See docs/VALIDATION.md for exact coverage.
Upstream world-mag creation, Havok, cleanup, saves and model restoration remain
authoritative. The old physical_reload_adapter installation remains disabled.

Exact upstream SHA256:
`7EB72860D209D9B9BDD774607220EE63323367AA9FA525F6EB1B23184F30F91E`.
The DLL is private evidence, not a distributable dependency.

Static contract

- First gate34DD22 replaces `cmp byte ptr [ebp-1751h],0` (7 bytes). Eligibility
  skips only the input-edge gate to34DD38; native calibration, held-mode and
  installed-mode gates, and the complete preparation34DD72..34E860 still run.
- Second gate34E861 replaces `cmp byte ptr [ebp-16C5h],0` (7 bytes). It rechecks
  the same actual held reference, stack count, ammo, weapon model/node and hand.
- Mode0/5 enters the full spare initializer34E90F with EAX/ESI=actual count.
  Mode4 enters the full installed-held initializer350852 with ESI=actual count;
  that initializer retains the native capacity clamp. Neither directly inserts.
- Both thunks preserve flags, GPRs, x87/MMX/XMM state and MXCSR; the original CMP
  is replayed on ordinary fallback. PUSHAD offsets are ESI+4 and EAX+28.
- Native registry18C68A8/AC has 0x1E8-byte records (max256). RefID+0, ammoID+4,
  ExtraCount+8, model string+C, node string+24 and retired byte+1E5 are read.
  Registry pointers are never retained across calls or written.
- Native builder328C40(out84,node,pos3,rot9,velocity3) supplies the exact currently
  selected native model and node names. Model resource paths permit only case
  and slash normalization; no basename-only matches or weapon-list guesses.
- Native lookup41A910 and stack-count41C350 resolve the actual current reference.
  Exactly one generic native held slot must own it in the native reload hand
  (320320). Dual/alias holds, stimulation, mismatches and unsupported states stay
  native. Zero-round references are not supported: native41A170 rejects them.
- The only inventory mutation is existing stowSelectedGrab -> native Activate.
  The exact inventory increase must equal the actual/registered stack count,
  the old reference must resolve absent/deleted, and weapon/ammo/state must still
  match. No AddItem, RemoveItem, registry erase or custom save metadata is used.
- A numeric stack-frame cookie is used only between the two hooks, never
  dereferenced, plus a 50ms deadline. Every MainLoop and load/exit clears it.

Integration

Review/apply `native_worldmag_recovery.integration.patch`; it currently applies
with `git apply --check`. Install once during NVSEPlugin_Load after all failure
returns, before native updates. Never hot install/unload. Existing NVSE runtime
040020D0 guard remains required. Call tick each MainLoop even on early exits;
reset only private eligibility on NewGame/PreLoad/exit. No SaveGame handler.
The proposal adds an opt-in `[Reload] RecoverWorldMagazines=1`; default is0 until
runtime acceptance. It reuses existing getCount/getMenu/getPlayer scripts.

The sc_state proposal publishes worldmagRecovery installed, poisoned, candidate,
transferred, rounds, hand, firstEntries, secondEntries, prepared, attempts,
committed, failures, inventoryBefore/After and reason. Existing pouch diagnostics
show the actual native modes/counts after the initializer.

Bounded simulator acceptance (parent runs; no game was launched for this work)

Use the isolated QA profile and existing bounded runner/watchdog. Keep ordinary
native grip/gravity-glove controls. Do not create references by state writes,
force installed state, call the initializer from a test, or Activate the target
through console: that would bypass the feature under test.

1. Positive full and partial magazine, pistol and carbine. Start with known ammo
   inventory I, actual native magazine count N, and ordinary manual reload.
   Native eject/drop must create a registered real ref and debit N once. Grip it
   in the reload hand. Exactly one attempt and commit must occur; inventory must
   increase by N, old ref must disappear/deleted, and native held count must be N.
   Move it through the normal well-insertion action. Verify native insertion,
   correct clip count (record chamber separately), firing, no extra world ref,
   and no additional recovery attempts. Repeat once.
2. Both state routes. Test empty well (mode4 -> installed-held mode2) and an intact
   gun (mode0/5 -> spare-held mode1). Eject the installed magazine normally if
   required, then insert the recovered spare. Do not infer one route from the other.
3. Pouch return. After recovery, move the authored native held magazine to the
   native visible pouch and release normally. Verify native held state clears,
   inventory stays at the post-Activate count, and no new world ref appears.
   Pull a fresh magazine from the same pouch to prove it is still usable.
4. Re-drop. Recover, throw away from pouch/well using ordinary native release.
   Native must debit N and create exactly one new upstream reference. Recover it
   again; total inventory plus live native dropped counts must be conserved.
   Do not add the loaded clip twice to that total if the engine inventory count
   already includes loaded ammunition; record the native baseline convention.
5. Save/load and cell reload. Save a native dropped ref, reload the same save
   twice, then grip/reinsert or pouch-return it. Repeat after cell unload/return.
   Exactly one conversion per current reference; no addon metadata is expected.
6. Refusal cases: manual off, wrong ammo, same ammo but different model/node,
   over-capacity stack, ordinary AMMO box absent from upstream registry, wrong
   hand, dual/alias hold, active stim, selector/settings/control-editor open,
   inactive tracking and loading. Inventory unchanged; original native pickup,
   take and throw remain available. Toggle manual on and retest an eligible ref.
7. End-to-end physical check: actual grip, correct magazine appearance/grip pose,
   insertion and pouch release with both reload handedness configurations.

Failure/atomicity limit

Native Activate plus script inventory inspection is not an engine transaction.
If activation completes but a postcheck fails or faults, the reference may already
be consumed and its ammunition restored. The adapter leaves that result native,
does not enter held initialization, and permanently poisons recovery for this
process. It never retries that transfer, fabricates a rollback or adds ammo.
Restart is required after investigating the diagnostic failure. Native Activate
scripts that independently duplicate items cannot be undone by this adapter.
The poisoned flag intentionally survives addon load/reset events.

Identity limit

Upstream records contain model/node/ammo identity, not source weapon FormID.
Weapons sharing all three identities and sufficient capacity are indistinguishable
here. No claim of original individual-weapon ownership is made.

Verification so far

The offline x86 /W4 check build passes count overflow/mismatch rejection, full-path
identity, candidate identity, bounded MSVC native strings and cross-page pointer
checks. Native instruction bytes and full file hash are checked separately by the
static script. These checks do not establish successful runtime insertion/pouch
return; those remain unproven until the cases above pass.
