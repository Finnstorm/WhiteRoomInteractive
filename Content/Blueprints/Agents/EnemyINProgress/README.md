# Progress Enemy

New enemy implementation for Unreal Engine 5.5. All new source, assets and tools live here. The only changes outside this folder are plugin registration in `WhiteRoomInteractive.uproject` and six compatibility routes in the existing `AC_EnemyHealth`. The player Blueprint and existing enemy AI are unchanged.

## Use in the project

For a simple arena, open `L_EnemyTest` in this folder and press Play. It contains a 60 m × 60 m flat floor, lighting, a player start, navigation bounds, and one spawner configured for three enemies. The spawner's `Spawn Around Spawner` option uses its own position as the spawn center and does not require procedural rooms or a map generator. Adjust `Room Spawn Radius` to change the arena spawn radius, and `Minimum/Maximum Per Room` to change its total wave size in this mode.

Open `WhiteRoomInteractive.uproject` and play `ProceduralDungeon`. The plugin automatically creates one `BP_ProgressEnemySpawner`; a spawner you place yourself takes priority. It waits for the existing generator's `MapComplete` flag and dynamic navigation, then populates generated rooms. It skips the starting room, keeps enemies away from the player, checks capsule clearance and navigation connectivity, and limits population to 20 by default. A room's spawn quota is consumed only after a successful spawn. Killed enemies do not respawn automatically.

To customize population, place `BP_ProgressEnemySpawner` in the map and edit its Details. To disable automatic integration, use **Project Settings → Game → Progress Enemy → Auto Create Dungeon Spawner**. Other maps require a placed spawner; turn off `Wait For Procedural Map` when they have no map generator. They still need `Master_Room` actors and usable navigation.

Place `BP_ProgressEnemy` directly to test an individual enemy. It creates its controller and the original `AC_EnemyHealth` / `AC_EnemyCombat` components automatically. There is no inherited enemy behavior tree or old enemy character logic.

## Behavior and combat

- Patrol chooses reachable random points around its spawn position and pauses between moves.
- Sight uses distance, a forward cone and a visibility trace. Continuous close proximity also triggers awareness after 2.5 seconds, including behind the enemy; walls prevent this detection, and leaving range resets the timer.
- Player damage alerts the enemy immediately, including attacks from behind. Generic, point and radial Unreal damage use the same health path.
- Chase follows the last known location after losing awareness. Search explores around that location for 10 seconds, then returns to patrol. It does not continually read the hidden player's position for navigation.
- Light and heavy melee share three existing punch montages. Defaults are 12 damage at 1.35× speed and 25 damage at 0.75× speed. Both recheck range, facing and cover at impact. Taking an unblocked hit cancels pending attack damage.
- Taser fires two shots, then recharges both over 6 seconds. Each shot deals 8 damage and temporarily locks player movement for 1.25 seconds. The existing player block prevents damage and stun. Cover can stop a shot after its windup, but the fired shot is still spent. A small blue beam is the current visual placeholder; `OnTaserFired` supplies start/end positions for replacement effects.
- Block reduces incoming frontal damage by exactly 75%, for three hits by default. It holds for 1.2 seconds at a time; exhausted charges recharge after 6 seconds. Unblocked and zero-damage hits do not spend charges. Rear attacks bypass the guard.
- `Enable Light Melee`, `Enable Heavy Melee`, `Enable Taser` and `Enable Block` are independent Details settings. `SetAttackEnabled` also cancels pending damage for the disabled attack. Taser-only enemies can shoot at close range.

All distances, delays, damage values, chances and movement speeds are editable on the enemy Blueprint or an instance. Runtime state, health, target, remaining charges and recharge timers are exposed for inspection. State, attack, taser and death dispatchers support additional presentation.

## Progress scaling

Call `SetDifficultyLevels(Damage, Block, Health, MovementSpeed)` from your progression system on an enemy or spawner. These are four independent integer levels; negative values clamp to zero. The spawner applies them before enemy BeginPlay and can update its existing enemies too.

| Category | Default scaling |
| --- | --- |
| Damage | Each level adds 15% of base damage to every attack |
| Block | Each level adds one hit to the base capacity of three |
| Health | Each level adds 20 to the base health of 100 |
| Movement speed | Each level adds 8% of patrol/chase base speed, capped at 750 cm/s |

These coefficients are editable. Scaling recalculates from the base values rather than multiplying previous results. Live health changes preserve health percentage; they do not heal or resurrect dead enemies. An ongoing shield recharge remains ongoing when difficulty changes. `ApplyDifficulty(Levels, RefillHealth)` is also available when an explicit refill is desired.

## Shared-system integration and animation

`AC_EnemyHealth` first asks the native compatibility router whether its owner is a new enemy. If so, native code handles initialization, damage processing and finisher events. Otherwise its original execution path runs. This retains its health-bar and finisher component identity for other systems. The new enemy uses `AC_EnemyCombat.ApplyDamageToPlayer`, clearing its per-attack hit list before each attack.

The existing player's finisher action recognizes only old enemy classes. A single runtime input bridge on the player handles the same `IA_Finisher` action for a new enemy under the crosshair within 350 cm. It leaves old enemy input behavior in place and edits no player asset. Finishers play the existing paired montages and restore player movement on completion or enemy removal. Enemy death retains the existing sanity-manager notification.

`ABP_ProgressEnemy` is a fresh animation Blueprint using the existing nurse mesh, `BS_EnemyWalkRun` and `DefaultSlot`; native speed updates avoid old animation Blueprint casts. Hit-reaction and enemy-finisher animations are retargeted copies in `Animations`, because the originals target the guard skeleton. No block sequence was found in this checkout, including the player animation assets. `AS_BlockPlaceholder` is an explicitly named idle placeholder on the existing enemy skeleton. Replace `Block Animation` with a proper guard sequence when available. Taser animation is also replaceable with `Taser Montage`.

The current integration targets this project's local-player gameplay. Network replication has not been implemented or tested.

## Build and validation

Editor binaries have been built on this machine. They are ignored by Git. After cloning, close Unreal and run:

```powershell
& '.\Content\Blueprints\Agents\EnemyINProgress\Tools\Build.ps1'
```

Use `-EngineRoot` for a different engine location and `-PackageTargets` to include Win64 Development/Shipping plugin builds. Generated Blueprints are already saved in this folder. `Tools/setup_assets.py` can regenerate their initial defaults through Unreal's Python commandlet if needed; it also installs the shared health routes idempotently.

`Verification.txt` records 38 passing isolated runtime checks. `DungeonVerification.txt` records the real procedural-map smoke test: 20 enemies, 20 controllers, and 8 enemies moving at the sample time. `AnimationVerification.txt` checks every assigned enemy animation, the retargeted sequence references and the player's finisher skeleton. Editor, Win64 Development and Win64 Shipping plugin compilation passed; the full game was not cooked or packaged. The resource test passed and is available in Unreal Automation as `ProgressEnemy.Combat.Resources`. To repeat the dungeon smoke test, launch the editor executable with the project, `/Game/Maps/ProceduralDungeon -game -nullrhi -unattended -ProgressEnemyVerifyDungeon`. This explicit test mode exits after 20 seconds and writes the report.

Loading the existing shared components also loads the old `EnemyAI/Enemies/BP_Enemy_Base`, which already has three missing-sword Blueprint compile errors. The new enemy assets compile independently, but those unrelated errors still appear in project commandlet logs and prevent claiming a clean whole-project cook.

Implementation references: Epic's [navigation system documentation](https://dev.epicgames.com/documentation/unreal-engine/navigation-system-in-unreal-engine), [world-partitioned dynamic navigation](https://dev.epicgames.com/documentation/unreal-engine/world-partitioned-navigation-mesh), and [point-damage API](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Game/Damage/ApplyPointDamage). Engine API signatures were checked against the installed UE 5.5 headers.

## Intelligence

The four difficulty levels now also determine decision speed, path update frequency, pursuit prediction and situational combat choices. See [Intelligence.md](Intelligence.md) for the shared formula, defaults and tuning.
