import unreal, os
root = '/Game/Blueprints/Agents/EnemyINProgress'
enemy = unreal.get_default_object(unreal.load_asset(root + '/BP_ProgressEnemy').generated_class())
mesh = enemy.mesh.skeletal_mesh_asset
mesh_skeleton = mesh.get_editor_property('skeleton')
rows = ['Mesh: ' + mesh.get_path_name(), 'Skeleton: ' + mesh_skeleton.get_path_name()]
animations = list(enemy.get_editor_property('melee_montages')) + [enemy.get_editor_property('block_animation'), enemy.get_editor_property('hit_reaction_montage'), enemy.get_editor_property('enemy_finisher_montage')]
animations += [unreal.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets(root + '/Animations', recursive=True, include_folder=False)]
for animation in dict.fromkeys(animations):
    if not isinstance(animation, unreal.AnimationAsset):
        continue
    assert animation
    skeleton = animation.get_editor_property('skeleton')
    rows.append(animation.get_name() + ': ' + skeleton.get_path_name())
    assert skeleton == mesh_skeleton, 'Animation skeleton mismatch: ' + animation.get_name()
player = unreal.get_default_object(unreal.load_asset('/Game/Blueprints/Agents/Player-Agent/BP_FirstPersonCharacter').generated_class())
assert enemy.get_editor_property('player_finisher_montage').get_editor_property('skeleton') == player.mesh.skeletal_mesh_asset.get_editor_property('skeleton'), 'Player finisher skeleton mismatch'
rows.append('Player finisher skeleton: PASS')
rows.append('RESULT PASS')
with open(os.path.join(os.path.dirname(os.path.dirname(__file__)), 'AnimationVerification.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(rows) + '\n')
unreal.log('PROGRESS_ENEMY_ANIMATIONS_PASSED')
