import unreal
root='/Game/Blueprints/Agents/EnemyINProgress/BlueprintOnly'
path=root+'/L_EnemyTest_BP'
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    assert unreal.EditorAssetLibrary.duplicate_asset('/Game/Blueprints/Agents/EnemyINProgress/L_EnemyTest',path)
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(path)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if isinstance(actor,unreal.ProgressEnemySpawner): actors.destroy_actor(actor)
enemy_class=unreal.load_asset(root+'/BP_EnemySpawner').generated_class()
spawner=next((a for a in actors.get_all_level_actors() if a.get_class()==enemy_class),None)
if not spawner: spawner=actors.spawn_actor_from_class(enemy_class,unreal.Vector(500,0,0))
spawner.set_actor_label('Blueprint Enemy Spawner - 3 enemies')
for name,value in {'SpawnAroundSpawner':True,'WaitForMap':False,'MinimumPerRoom':3,'MaximumPerRoom':3,'MaximumAlive':3,'SpawnRadius':1200.0,'MinimumPlayerDistance':1000.0}.items(): spawner.set_editor_property(name,value)
observer_class=unreal.load_asset(root+'/BP_TestObserver').generated_class()
if not any(a.get_class()==observer_class for a in actors.get_all_level_actors()):
    actors.spawn_actor_from_class(observer_class,unreal.Vector(0,0,0)).set_actor_label('Optional Blueprint Smoke Test')
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
registry=unreal.AssetRegistryHelpers.get_asset_registry()
options=unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True)
for name in ['BP_Enemy','AC_EnemyHealth_BP','AC_EnemyCombat_BP','BP_PlayerBridge','BP_EnemySpawner','ABP_Enemy','BP_TestObserver','L_EnemyTest_BP']:
    dependencies=registry.get_dependencies(root+'/'+name,options)
    assert not any(str(d)=='/Script/ProgressEnemy' for d in dependencies), (name,dependencies)
unreal.log('BLUEPRINT_ONLY_TEST_MAP_READY')
