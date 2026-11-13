# Intelligence scaling — both enemy versions

Keep calling the existing enemy or spawner `SetDifficultyLevels` function with the four progression integers. Intelligence updates automatically, including for existing enemies when the spawner's update-existing option is enabled.

`IntelligenceLevel = (Damage + Block + Health + MovementSpeed) / 4`

The inputs first clamp to 0–10000. The fractional average is intentional: raising any category contributes equally. `IntelligenceAlpha` clamps the average divided by `IntelligenceLevelForMaximum` to 0–1. The default maximum is an average level of 10; change that enemy setting to match your progression range, then reapply difficulty. These values are visible on each enemy.

| Average difficulty | Decision delay | Path update interval |
|---|---|---|
| 0 | 0.350 s | 0.400 s |
| 5 | 0.225 s | 0.275 s |
| 10 or above | 0.100 s | 0.150 s |

Decision delay applies when entering chase, including first detection and recovery after an attack. Hit reactions use 1.143 times the decision delay; ending a block uses 1.429 times the delay. This speeds up choices without changing attack animations, attack windups, taser cooldowns or shield recharge. Sight and continuous-proximity sensing retain their existing rules. Runtime timers are processed on the existing actor tick, so timings are quantized by that tick.

As intelligence rises:

- Heavy-attack probability moves from the configured `HeavyChance` toward 80% against a stationary target within 80% of melee range, or toward 0% against other targets. Stationary means horizontal speed at most 75 cm/s. An enemy configured with only heavy melee still uses that enabled attack.
- Block probability increases by up to 40 percentage points as health falls, capped at 85%. Enable toggles, charges and cooldowns still apply.
- Pursuit leads a visible target by up to 0.45 seconds of velocity, with the sampled velocity capped at 450 cm/s. Losing visibility removes prediction; chase and search continue using the last known location.

The native implementation uses `AProgressEnemyCharacter`. The Blueprint implementation contains equivalent standard Unreal nodes in `RefreshIntelligence`, `RefreshCombatChoices`, `Think`, and the timing functions. It retains its engine Character parent and has no direct `/Script/ProgressEnemy` dependency. `BlueprintOnly/Authoring/update_intelligence.py` patches those named functions while preserving components, defaults and animation setup; back up before rerunning because it replaces those functions.

Validation is recorded in `Verification.txt`, `BlueprintOnly/Verification.txt`, and `BlueprintOnly/AnimationPoseVerification.txt`. `Tools/verify_intelligence.py` reopens both implementations, checks actual alert-to-attack timing at levels 0 and 10, checks bounded scaling and combat choices, and verifies the Blueprint poses against `BP_Enemy_punch`.
