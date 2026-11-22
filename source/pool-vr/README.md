# Filthy Casual Pool — standalone optional addon

Physical VR pool at the Prospector Saloon for KUPOkinz's **A Tale of Two Realities (TTR)**.
Ask Sunny for a game or practice at the table. This is a separate addon: **Filthy
Casual TTR and the Meta Link helper are NOT required.**

## What you need

1. Your legally owned game and a working TTR installation, with its normal
   xNVSE, JIP LN, JohnnyGuitar, SUP and kNVSE requirements.
2. Luke2172's [Pool Tables Of The Mojave v4](https://www.nexusmods.com/newvegas/mods/100097),
   downloaded from its author and installed separately.
3. **Filthy Casual Pool** from this release.

Optional: use **the TTR 1.1.2 build of Filthy Casual TTR (1.6.0-dev.2)** alongside pool for seated
height, selectors and gestures. Update that addon before combining them. Older
versions are refused for pool because their input helpers can interfere.
Standalone users do not need to install it at all. Use your existing working VR
runtime; neither SteamVR nor our direct Meta helper is added as a requirement.

```text
Working TTR + its normal dependencies
  + Original Pool Tables Of The Mojave v4
    + Filthy Casual Pool

Filthy Casual TTR 1.6.0-dev.2: optional companion, not a parent dependency
```

## Install in MO2

1. Install the original pool mod separately and keep it enabled.
2. Install `Filthy-Casual-Pool-0.2.1-beta.zip` as **Filthy Casual Pool**.
3. Right-click our mod in MO2, choose **Open in Explorer**, and run
   `Prepare-Pool.cmd`. Select the ORIGINAL pool mod's folder (the one containing
   `Pool Table Minigame.esp`). Wait for the success message. Do this once per
   addon update before enabling it.
4. Enable our mod below the original pool mod in MO2's left pane; enable
   `Filthy Casual Pool.esp` in the right pane. Keep the original ESP enabled.
5. Launch the usual TTR game entry through MO2.

The one-time setup reads four verified original scripts and generates three
integration scripts inside OUR mod folder. It does not change the original mod,
download files, touch saves, register a global XR layer or change your runtime.
Unknown upstream versions fail before writing output. Our ZIP includes only our
addon, new animation tracks, patch instructions and license notices—no original
pool meshes, sounds, textures, ESP or completed upstream-derived scripts.

## Play

- Put your weapon away and activate the saloon pool table for practice.
- Ask Sunny **Care for a game of pool?** while she is free from combat/quest travel.
- Hold both grips: left hand guides the shaft; right hand strokes forward.
- Release either grip to put the cue away. Move around the table normally.
- After a scratch, position the cue ball with the right hand and press right trigger.
- **B** leaves the game. Menu remains available. Pool input ownership ends on exit.

Sunny plays a newly authored 7.2-second setup, stroke and recovery. The real cue
tip contacting the ball starts the shot. The original mod supplies the planner,
ball physics and rules. No wagers. Integration currently targets this saloon
and Sunny's supported 0.95 rig scale. Loading resets the current match; no save
interception or inventory recovery is added.

## Compatibility and current limits

This 0.2.1 beta targets TTR 1.1.2. FNV 1.4.0.525, xNVSE 6.4.9 and TTR SHA256:
`C565A6C296B597F89FCF84D7F6001D47D1635792F0E8CACAD92C6A88F6823E73`.
Other TTR builds are refused. Our pool-only input companion runs independently;
with the current seated addon it delegates to the active matching input instance,
avoiding duplicate input filtering. It adds no seated controls when used alone.

Standalone physical shots, tracking-loss protection, exit and native camera have
passed local native-game tests. Combined addon tests are recorded in the release
validation notes. Physical comfort, a full eight-ball match and active-match
save/restart remain outside the completed test coverage. This is a beta.

Credit: KUPOkinz (TTR), Luke2172 (original pool mod), Nikami (VR addon and new
animation). Their mods remain separate downloads. Do not redistribute your locally
generated integration scripts as an original pool-mod replacement.

TTR 1.1.2 checks include a combined physical stroke/exit and a captured Sunny/player
turn sequence. A complete eight-ball match and fresh physical-headset acceptance
are not certified. Pool-cue/chalk/rack pickup polish remains in development.
