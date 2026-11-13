# Blueprint-only enemy

For the complete internal function, event, animation, variable and spawning reference, see [Enemy internals](Documentation/EnemyInternals.md).

German presentation material, researched sources, slide text and an evidence dossier are available in [Fachvortrag](Documentation/Fachvortrag/README.md).

Open **L_EnemyTest_BP** in this folder and press Play. It is the flat test arena with three enemies spawned by **BP_EnemySpawner**. The earlier C++ enemy and its test map remain available separately.

The new character, components, spawner, animation Blueprint and input bridge have standard Unreal engine parents. Their graphs use standard Unreal Blueprint nodes and contain no calls to `/Script/ProgressEnemy`. No custom C++ gameplay class or custom runtime node implements this version.

## Assets

| Asset | Purpose |
| --- | --- |
| BP_Enemy | Character, sensing, patrol/chase/search, combat timing, block, scaling, death and finishers |
| AC_EnemyHealth_BP | Independent health, health scaling and finisher eligibility |
| AC_EnemyCombat_BP | Point damage through the existing player's damage/block system |
| ABP_Enemy | BP_Enemy_punch's locomotion/falling/landing graph and montage slot, with generic owner updates |
| BP_PlayerBridge | Existing finisher input and timed taser movement lock, without editing the player asset |
| BP_EnemySpawner | Standalone arena and procedural-room spawning |
| BP_TestObserver | Optional Blueprint smoke-test observer; inactive during normal Play |

The enemy uses the same existing punch animations and retargeted nurse hit-reaction/finisher animations as the working version. The block animation is still the named idle placeholder. The blue taser beam is a simple visual placeholder.

The animation setup now matches `BP_Enemy_punch`: mesh transform, capsule dimensions, cached locomotion pose, state machine, falling/landing transitions and montage slot. Idle, walk, run, falling, light punch and heavy punch were evaluated against the reference; their bone positions match. Melee attacks cache their selected montage and wait for its full playback duration at the configured speed before another attack can start. The generic Blueprint owner update avoids casts to the old enemy classes. The shared blend-space asset is used as-is.

## Tuning

Edit **BP_Enemy → Class Defaults**, or an enemy instance. `EnableLight`, `EnableHeavy`, `EnableTaser` and `EnableBlock` are independent. `SetAttackEnabled(Index, Enabled)` also cancels a pending disabled attack: 0 = light, 1 = heavy, 2 = taser.

Defaults match the working version: random reachable patrol goals; cone/line-of-sight detection; immediate alert from player damage; 2.5 seconds of continuous close presence; light/heavy attacks at 12/25 damage and 1.35/0.75 animation speed; two taser shots before a six-second recharge; 75% frontal damage reduction for three block hits before recharging; chase, search, then patrol when awareness is lost. Attack impacts recheck cover and range.

Call `SetDifficultyLevels(Damage, Block, HealthLevelInput, MovementSpeed)` on an enemy. Four independent integer levels scale damage (+15% of base per level), block capacity (+1 hit), maximum health (+20), and movement speed (+8%, capped at 750). Coefficients are editable. Live health scaling preserves its percentage. The spawner exposes the same function plus `UpdateExisting` to apply changes to its current enemies.

`State` is visible for debugging: 0 Patrol, 1 Chase, 2 Search, 3 Attack, 4 Block, 5 Finisher, 6 Dead. Functions are separated by behavior; start at the character EventGraph to follow initialization, Tick and incoming damage.

## Spawning

For a plain navigable level, place **BP_EnemySpawner** and enable **SpawnAroundSpawner**. `SpawnRadius` controls the radius, `MinimumPerRoom`/`MaximumPerRoom` set the wave size in this mode, and `MaximumAlive` caps population. The supplied arena sets all three counts to 3. Successful spawns consume quota; killed enemies are not continuously replaced.

For the existing procedural dungeon, leave **SpawnAroundSpawner** off and **WaitForMap** on. The spawner waits for `Map_Generator.MapComplete`, visits `Master_Room` instances, skips spawn rooms and positions close to the player, samples reachable navigation points, and checks capsule clearance and enemy separation. Navigation bounds must cover the generated map.

When testing this version in `ProceduralDungeon`, disable **Project Settings → Game → Progress Enemy → Auto Create Dungeon Spawner**, or the original plugin will also populate that map with its C++ enemies. The Blueprint-only arena already uses only the new spawner. Shared project/player assets still belong to the existing project; this folder does not remove the original plugin from the project.

## Authoring and checks

The `Authoring` directory contains editor-only Python/JSON automation used to assemble these normal Blueprint graphs. The editor builder and verification harness reside in the existing plugin, solely as authoring/testing utilities; the generated enemy never calls them. `Authoring/build.py` creates the generated assets in a fresh destination and refuses to overwrite existing ones. For regeneration, close Unreal and back up those generated assets first; manual graph edits are not merged by the generator. Normal editing and Play require no authoring script execution.

`Verification.txt` records compile/parent/gameplay checks, including complete montage playback timing. `AnimationPoseVerification.txt` compares evaluated poses to `BP_Enemy_punch`; `AnimationFixReport.txt` records the animation changes and compile results. `Authoring/BuildReport.txt` records the initial generation results. The arena can also be launched with `-BlueprintOnlySmoke`; its Blueprint observer reports enemy/controller/movement counts and exits after 12 seconds. Without that flag, the observer does nothing.

The original project's missing-sword errors in the old enemy base remain separate from the new Blueprint assets. Multiplayer replication has not been implemented or tested.

## Intelligence

The four difficulty levels now also determine decision speed, path update frequency, pursuit prediction and situational combat choices, using standard Blueprint nodes. See [Intelligence.md](../Intelligence.md) for the shared formula, defaults and tuning.
