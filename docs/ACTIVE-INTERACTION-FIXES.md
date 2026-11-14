# Active user-requested interaction fixes (2026-11-08)

1. X on selected Iguana on a Stick crashed the user's game. Windows Event1000:
   FalloutNV.exe 1.4.0.525, c0000005, offset55F98B (VA95F98B). Exact X-only
   isolated test did not reproduce. Live instruction reads vtable+100 from an
   object; no register/stack evidence from the user's crash, so cause unproven.
   New sticky-only A/X route releases native ownership before stowing, instead
   of letting the activation ray pick an item still held. Tests pending below.
2. User clarified: food-to-mouth consumption ONLY in our sticky mode. A grip
   squeeze restores native handling with no automatic eating. One food instance,
   native effects, no save changes. Approach from outside25cm, dwell650ms inside
   12cm. Only engine Food-flagged ingestibles, excluding live Stimpaks/other aid.
3. Blend our free-hand pose into TTR's native interaction/approach poses. Use
   existing native distance fade at pose dispatch; retain native held/contact
   ownership and downstream wrap/smoothing. Built; runtime visual proof pending.
4. Scope defect: intermittent **2D frame around scope**, not whole-view zoom.
   SuppressVanillaScope defaults1 already; don't claim setting1 fixes it.
   Native bInFrame defaults1; inspect native scope composition before changing.
5. Floating loot/pop-up menus and animal HUDs: latch pose within interaction
   zone, don't follow tiny hand movements; dismiss on leaving. Left/right page,
   up/down remains navigation. Needs native hit-test and display same transform.
6. Dialogue clarification: steady but misplaced when close to NPC. Offset to
   the side, never over face/body, retain minimum comfortable distance from eyes.
   Don't claim dialogue needs temporal smoothing or blanket UI stabilization.

Preserve personal saves/settings; no QA game left running. Public1.4.4 release
and old video stay unchanged until final implementation is reviewed. Changes
currently build locally against exact supported TTR hash. UI/scope work has not
been implemented yet. Follow STATUS plus newest test evidence for completion.
