"""Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>. Idempotent."""
import unreal

assert unreal.ProgressEnemyEditorLibrary.install_health_compatibility(), 'Health compatibility graph did not compile/save'
assert unreal.ProgressEnemyEditorLibrary.create_project_assets(), 'Enemy assets did not compile/save'
root = '/Game/Blueprints/Agents/EnemyINProgress'
label = unreal.load_asset(root + '/PAL_ProgressEnemy')
if not label:
    label = unreal.AssetToolsHelpers.get_asset_tools().create_asset('PAL_ProgressEnemy', root, unreal.PrimaryAssetLabel, unreal.DataAssetFactory())
label.set_editor_property('is_runtime_label', True)
label.set_editor_property('label_assets_in_my_directory', True)
rules = label.get_editor_property('rules')
rules.set_editor_property('cook_rule', unreal.PrimaryAssetCookRule.ALWAYS_COOK)
label.set_editor_property('rules', rules)
label.set_editor_property('explicit_assets', [unreal.load_asset(p) for p in [
    '/Game/Blueprints/CombatSystem/Health/AC_EnemyHealth.AC_EnemyHealth',
    '/Game/Blueprints/CombatSystem/Combat/AC_EnemyCombat.AC_EnemyCombat',
    '/Game/Input/Actions/IA_Finisher.IA_Finisher',
    '/Game/ShaderMats/Sanity.Sanity',
]])
assert unreal.EditorAssetLibrary.save_loaded_asset(label)
for path in ['BP_ProgressEnemy', 'BP_ProgressEnemySpawner', 'ABP_ProgressEnemy', 'AS_BlockPlaceholder']:
    assert unreal.load_asset(root + '/' + path), path
enemy = unreal.get_default_object(unreal.load_asset(root + '/BP_ProgressEnemy').generated_class())
assert enemy.mesh.skeletal_mesh_asset, 'Enemy mesh defaults were not saved'
assert enemy.mesh.anim_class, 'Enemy animation defaults were not saved'
assert len(enemy.get_editor_property('melee_montages')) == 3, 'Melee montage defaults were not saved'
spawner = unreal.get_default_object(unreal.load_asset(root + '/BP_ProgressEnemySpawner').generated_class())
assert spawner.get_editor_property('enemy_class'), 'Spawner class was not saved'
unreal.log('PROGRESS_ENEMY_SETUP_PASSED')
