# Creature rides — local test build

These optional modes belong to the Vehicles addon. They use the player's owned
Fallout creature meshes and animations; no game assets are included.

Enable either mode in `NVSE/Plugins/FilthyCasualVehicles.ini`:

```ini
[Bighorner]
Enabled=1
ChallengeSeconds=30

[Deathclaw]
Enabled=1
ChallengeSeconds=60
```

ChallengeSeconds is set independently for each creature and read at game startup.
**0 unlocks steering immediately after a valid mount.** It does not bypass the
Deathclaw sneak/behind/undetected rules or change hostility after dismount.
Missing or invalid values use 30 seconds for Bighorners and 60 for Deathclaws.

Both options default off. Filthy Casuals discovers their control modes when the
installed addon has verified the supported TTR build and initialized successfully.

## Playing

- **Bighorner:** touch the nearby animal with both empty hands and squeeze both
  grips to mount, facing forward. Survive a 30-second challenge.
- **Deathclaw:** you must be sneaking behind an unaware creature that is not in
  combat. Touch its body with both empty hands and squeeze both grips. A spotted
  player cannot mount, even from behind. Survive 60 seconds to earn steering.
- During a buck, at least one gripping hand must be on the creature. Squeezing
  in empty air does not count. The other hand stays free. A brief native notice
  warns before a buck; losing the hold for over 0.65 seconds during it ejects you.
- After earning control, **left stick forward/back** drives and **right stick
  left/right** turns. **B** dismounts. Native weapon/item triggers remain native.
- Every dismount resets the challenge. Deathclaws resume native combat against
  the player on dismount; they never become permanent companions.

## Frame and state ownership

The rider's camera, body and UI share the existing mounted publication pipeline.
The camera uses a level locomotion frame, without inheriting spine animation.
Deathclaw hand contacts follow the current animated shoulder frame. The native
arm solver retains responsibility for the player's arms.

The ride temporarily restrains only its carrier and disables player movement.
The addon records the original movement/restraint ownership in one namespaced
JIP auxiliary array, `_FC_CreatureRide`, on the player. Ordinary dismount removes
the record. Loading a save made during a ride restores those owned states,
detaches the player safely and resets the challenge. It does not restore items,
change inventory, or rewrite save files. Dismount before uninstalling the addon.

## Verification

Prior native simulator checks cover front/standing/detected refusal, hidden
rear mounting, body contact, the previous 60/120-second timers, earned steering,
dismount, native hostile reaction and loading a mid-ride save. Final capture and
physical-headset acceptance are tracked in `STATUS.md`; simulator checks alone
do not certify comfort or controller haptics.

Focused C++ checks cover the new 30/60-second defaults, custom durations, zero
unlock, invalid configuration fallback, pause and missed-hold ejection.
