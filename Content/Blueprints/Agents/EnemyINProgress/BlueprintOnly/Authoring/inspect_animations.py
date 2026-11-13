import unreal, os
folder=os.path.dirname(__file__)
paths=[
 '/Game/Blueprints/Agents/Enemy-Agent/EnemyAI/Enemy_punch/BP_Enemy_punch',
 '/Game/Blueprints/Agents/Enemy-Agent/EnemyAI/Enemy_punch/BP_Enemy_punch_Male',
 '/Game/Blueprints/Agents/Enemy-Agent/EnemyAI/Enemy_punch/BP_Enemy_punch_Guard',
 '/Game/Blueprints/Agents/EnemyINProgress/BlueprintOnly/BP_Enemy',
 '/Game/Blueprints/Agents/EnemyINProgress/BP_ProgressEnemy',
 '/Game/Blueprints/Agents/EnemyINProgress/BlueprintOnly/ABP_Enemy',
 '/Game/Blueprints/Agents/Enemies/Animations/AI/ABP_EnemiesReal',
]
with open(os.path.join(folder,'AnimationInspection.txt'),'w',encoding='utf-8') as f:
 for path in paths:
  bp=unreal.load_asset(path)
  f.write('\n### '+path+'\n')
  if not bp: continue
  cdo=unreal.get_default_object(bp.generated_class())
  if isinstance(cdo,unreal.Character):
   mesh=cdo.mesh
   for prop in ['skeletal_mesh_asset','anim_class','animation_mode','relative_location','relative_rotation','relative_scale3d','visibility_based_anim_tick_option','enable_update_rate_optimizations','pause_anims','no_skeleton_update','component_tick_interval','primary_component_tick']:
    try: f.write(prop+' = '+str(mesh.get_editor_property(prop))+'\n')
    except: pass
   f.write('capsule '+str(cdo.capsule_component.get_unscaled_capsule_half_height())+'\n')
  f.write(unreal.ProgressEnemyEditorLibrary.inspect_blueprint(path))
unreal.log('ANIMATION_INSPECTION_COMPLETE')
