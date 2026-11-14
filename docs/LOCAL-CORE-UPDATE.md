# Local core build, verification and installation

Use one frozen candidate for the entire handoff. Rebuilding after a test means a
new candidate and a new check; do not copy a DLL from a changing build directory.

```powershell
.\tools\check-food-glass-candidate.ps1 -Candidate D:\code\ttwvr-seated-controls\outputs\food-glass-NEW
```

This builds the core and input layer, runs focused mouth checks, stages the two
DLLs, input manifest, glass ESP and textures together, and runs the native food
and ground-impact checks with a copy of the actual personal INI. Personal saves
and settings are not edited. The isolated game closes at completion.

Review both eyes in the generated glass 2s/4s/12s evidence images: actual shards
must appear and then fade. `verification.json` records runtime success; set
`visualReviewed` to true only after inspecting those captures. Then install:

```powershell
.\tools\core-candidate.ps1 -Action Install -Candidate D:\code\ttwvr-seated-controls\outputs\food-glass-NEW
```

Installation refuses a running game, changed candidate, missing pass or missing
visual review. It backs up existing files, verifies installed hashes, rolls back
on copy failure, and leaves SeatedControls.ini unchanged. `installed.json` records
the exact files and rollback folder. The shared Filthy Casual TTR MO2 mod supplies
the normal Seated Custom and Vehicle Test profiles; glass ESP must remain enabled.

This workflow covers the changed core/input/glass payload, not a complete public
release or validation of every vehicle feature. Simulator checks do not establish
physical-headset reliability on their own.
