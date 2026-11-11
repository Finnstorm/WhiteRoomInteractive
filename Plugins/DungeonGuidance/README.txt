Installed changes - WhiteRoomInteractive, 10 November 2026

Implementation order
1. Available Points and per-node costs.
2. Hide exit arrow markers on the used room Blueprints.
3. Bidirectional room graph and live objective routing.
4. Traveling red doorway pulses.
5. Routed, translucent overhead scent line.
6. Unused-exit connection detection and physical tunnels.

Editor controls
Open /Game/Blueprints/ProceduralMap/Map_Generator and select its new
DungeonGuidance component. Settings are also editable per placed generator instance.

Guidance / Timing:
  DoorHintDelaySeconds = 10
  SmellHintDelaySeconds = 20
  RouteRefreshCooldownSeconds = 30
The first route is calculated when the earlier hint delay expires. Recalculations
are throttled to 30 seconds since the previous calculation. The scent anchors to
the player and doorframes already passed are trimmed between recalculations.
Collecting the key immediately removes the old effects and restarts both delays
for the exit-door objective. Pausing the world pauses these timers.

Guidance / Door Pulse:
  MaxSimultaneousDoors = 5
  PulseStepSeconds = 0.3
  PulseGrowthSeconds = 1
  PulseWidthCurve = CF_DoorPulseWidth
  PulseSpeedCurve = CF_DoorPulseSpeed
Both curves live in /Game/Blueprints/ProceduralMap/Guidance.
Their time range is normalized 0..1 across PulseGrowthSeconds. Width values are
normalized 0..1, mapping from one doorway to MaxSimultaneousDoors. Speed values
multiply PulseStepSeconds: smaller values advance the pulse faster. The window
travels toward the objective, fades out at the end and repeats. Both ends of a
tunnel are included in the doorway sequence.

Guidance / Smell:
  SmellHeightAboveFloor = 220 cm
  SmellRadius = 1.5 cm
  SmellWaveAmplitude = 4 cm
  SmellSegmentLength = 150 cm
  SmellMaterial = M_SmellTrail (translucent, opacity 0.3)
The line follows room connections and doorways. Within rooms a bounded, collision-
tested 3D search lets this floating effect pass above furniture without going
through walls or ceilings. A capsule/ground search is available as a fallback.
If a safe local route cannot be established, the scent is hidden instead of
drawing an incorrect shortcut through a wall. Long paths adapt segment length
to a maximum of 256 mesh segments. The effect has no collision or shadows.

Tunnels:
  bConnectUnusedExits = true
  MaxTunnels = 4
  MaxExitPairDistance = 3500 cm
  MaxExitHeightDifference = 10 cm
  MaxTunnelTurns = 3
  TunnelWidth = 240 cm; TunnelHeight = 280 cm; TunnelThickness = 20 cm
  TunnelGridSize = 200 cm
  ExitMarkerHeightAboveFloor = 0 (matches the current room exit markers)
  TunnelCandidatePairBudget = 40; TunnelSearchNodeBudget = 5000
  TunnelCube = Engine BasicShapes/Cube (100 cm cube); optional TunnelMaterial

Before CloseExits iterates its list, BeforeCloseExits examines unused exits from
different rooms, closest pairs first. The exit-door reservation is excluded.
Aligned exits can receive a direct bridge, including very short gaps. Other
compatible exits use a rectilinear search with at most three 90-degree turns.
The search uses a basis aligned to the first exit, so rotated rooms work too.
The two exits must have compatible directions in that basis; arbitrary angled
or sloping corridors are not part of this pass.

Corridor-sized sweeps reject physical obstructions. Other rooms' placement
volumes are excluded as well. A search that fails, exceeds its budget or finds
incompatible endpoints leaves those exits for the normal sealing process.
Successful corridors build floor, ceiling and boundary walls from a union of
rectangles. Shared internal edges have no walls, so turns remain open.
Only after construction succeeds are both endpoints removed from ExitList and
registered in the owners' UsedExits. A bidirectional graph edge stores the
connector polyline for both route hints. Remaining unused exits are sealed by
the existing EndWall logic.

Generator analysis / integration
Weighted room selection and random placement selection were retained. The used
classes are Hallway_I_1, Hallway_I_2, Hallway_T_1, Hallway_T_2, Hallway_T_3,
X_Hallway_1, Real_Spawn_Room and Security_Office, plus their Master_Room parent.
Their arrow components are Hidden In Game; existing editor visibility is retained.

The old pathfinder followed each room's parent chain and precomputed spawn->key
and key->exit routes. Those activation calls are bypassed. Purple path arrows
are no longer spawned. The new graph includes original room connections and
new tunnel links, so it supports loops. Unused legacy drawing functions/assets
remain available for compatibility but are not part of the runtime flow.

The farthest-exit result is now consistently used for the exit-door room, exit
reservation and removal from ExitList. BP_Key only accepts the player, sets
HasKey, notifies the generator to switch objectives, and destroys itself. It
also handles collection in a level without a generator. New-map guidance clears
HasKey before starting the key objective.

Skill tree UI
WBP_SkillTree shows live available Points beside its existing DP display.
WBP_SkillNode shows NodeData.Cost in its center, counter-rotated to remain upright.
Prices come from DT_SkillTreeNodes; existing price values were not changed.
Purchase, refund, loading a tree and external point changes refresh the display.

Persistent implementation
Plugins/DungeonGuidance contains the runtime C++ component, source and compiled
UE 5.5 Windows binaries. It is enabled by default. It has no dependency on the
temporary editor commandlet used for making and testing these Blueprint changes.
Rebuilding or porting the plugin requires the normal Unreal C++ build toolchain.
References to room/objective/visual classes, meshes, curves and material are
assigned on the Blueprint component, rather than hard-coded asset paths.

Validation completed
- Native module built for Unreal Editor, Windows Development and Windows Shipping.
- 52 node costs, live points, null-save handling, purchase and refund tested.
- Door hint at 10s, scent at 20s, 30s route refresh and key timer reset tested.
- Six-door fixture grows from one to exactly five lit frames, travels and repeats.
- Interior obstruction detours and routed scent meshes tested.
- Straight, short, right-angle, three-turn and 45-degree rotated tunnels tested.
- Capsule sweeps through all three-turn segments/corners pass; blocked and height-
  mismatched connectors are rejected without consuming exits.
- Actual Hallway_I_1 pair tested through the generator's CloseExits event:
  tunnel builds, both end caps are excluded, guidance starts and uses the new edge.
- Actual BP_Key overlap tested with a non-player and the player.
- Full-size generated dungeons tested with seeds 123, 789 and 2026: 35, 28 and
  34 rooms. Both key and exit objectives have routed scent paths; all spawned
  room arrows are hidden. These particular seeds needed no valid extra tunnels;
  physical connection creation was verified separately with the actual room pair.
- Offscreen doorway/scent preview rendered and inspected.
- Installed plugin loaded and assets/room markers checked in the original project.

Existing project issue outside this change
Loading the original project reports BP_Enemy_Base compile errors: a SpawnActor
node has no class, and Get/Set nodes contain stale Sword Actor 0 pins. These also
appeared before the guidance changes. The enemy Blueprint was not modified.
Consequently this validation is not a claim that the entire project compiles.

Backups
Original changed assets are backed up in:
E:\Github\WhiteRoomInteractive\Saved\CodexBackups\DungeonGuidance_20261110_233506
installed-assets.json there records installed file hashes. No player progression
reset, price edits or save-game migration was performed in this task.
