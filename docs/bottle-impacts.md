# Empty-bottle impacts (local candidate, 2026-11-13)

Scope: the four matching original-game empty bottle bases produced by sticky
consumption, plus those same empties subsequently held by the player. No scan
or conversion of all clutter, no consumable duplication, and no save handler.

## Decision

`bottle_impact.hpp` requires a live native Havok contact, two samples from the
same body less than 100 ms apart, no held/caught state in either sample, at least
1.25 m/s velocity change and at least 2.5 m/s relative closing velocity along that
change. Contact constraint type is checked; grab joints do not qualify. The
other body's velocity is subtracted, including either constraint ordering.
These are observed velocity-change estimates, not direct solver impulse values.
A collision occurring entirely between samples may be missed.

`bottle_impact_adapter.hpp` reads retail FNV 1.4.0.525 layouts documented in
JIP LN's `internal/havok.h` and `internal/class_vtbls.h`: native hkpRigidBody
contacts AC/B0, constraints B4/B8, linear velocity 1B0 and simple-contact-data
vtable 10CC934. Velocities use 0.1 metres/second per native unit. Only the game
thread reads these structures; no new code patch or Havok callback is installed.

## Native impact presentation

The earlier world-mesh experiment is superseded. The current adapter submits
our original transparent glass artwork through Fallout's geometry decal manager,
with a small owned-game glass particle effect and a positional smash sound.
The engine owns projection, decal caps and fading. No custom glass-mesh timer,
save handler, copied game assets or texture-pack replacement is used.

Original artwork: assets/bottles/glass-impact-v1.png (generated for this project).
Build tools/build-glass-assets.py with --texconv pointing to DirectXTex. Output
is a texture-only Filthy Casual Glass.esp and original DDS files. The plugin has
no overrides and depends only on FalloutNV.esm.

Compatibility: retail FNV 1.4.0.525; native constructors/calls are byte-guarded.
Floor and three sign impacts were recorded through native hand releases.
The decal uses outward surface normals from the native ray, exact Havok unit
conversion and excludes the bottle's own geometry. The verified TTR sound hook
is preserved; the positional sound is not attached to the deleted bottle.
Only our decals set forceFade=1; native 10-second lifetime/fade and caps apply.
No global decal settings are modified. Pending counters are not visual proof.
Installed hashes and rollback paths: evidence/bottle-impact-installed.json.

## Checks

- `tests/build-bottle-impact.cmd`: hard contact passes; fast flight, gentle drop,
  glancing slide, co-moving surface, held/caught items, stale samples and changed
  body identity do not break.
- `tests/bottle-impact-runtime.ps1`: native physics drops, actual collision
  detection, exactly one break, projected-glass visibility. The film provides the separate timed-fade proof.
- `tests/bottle-handoff.ps1`: whiskey, Scotch, Nuka-Cola and Sunset consumption,
  matching native right-hand empties, preserved grip pose and released hand ownership.
  This older assertion alone did not prove continued world motion.
- `tests/bottle-release-diagnostic.ps1`: the actual consume-to-empty release path,
  comparing the visible node against native Havok motion; reproduced the reported
  freeze while the physics body continued falling. Ordinary ground pickup was
  the matched control and continued rendering its trajectory.
- `tests/bottle-combined.ps1` runs both runtime paths in the disposable save.

Evidence and candidate installation status are recorded in STATUS.md and
VALIDATION.md. Simulator acceptance does not certify physical headset feel.

## Consumed-bottle release correction

The native world body could fall while the rendered replacement stayed suspended.
The selector now positions same-cell refs with one native SetPosEx, avoiding its
prior MoveTo and three SetPos operations. The full-to-empty relative grip is
preserved, and TTR still owns pickup, grip, release and throw velocity. Four-family
regression now checks actual rendered flight after release. Installed locally;
physical-headset acceptance is pending. See the latest STATUS/VALIDATION entries.
