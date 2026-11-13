import unreal, os

root = os.path.dirname(os.path.dirname(__file__))
paths = [
 '/Game/Blueprints/CombatSystem/Health/AC_EnemyHealth',
 '/Game/Blueprints/CombatSystem/Combat/AC_EnemyCombat',
 '/Game/Blueprints/Agents/Enemy-Agent/EnemyAI/Enemy_punch/BP_Enemy_punch',
 '/Game/Blueprints/Agents/Enemy-Agent/EnemySpawner/BP_EnemySpawner',
 '/Game/Blueprints/Agents/Player-Agent/BP_FirstPersonCharacter',
 '/Game/Blueprints/Agents/Enemies/Animations/AI/ABP_EnemiesReal',
 '/Game/Blueprints/Agents/Player-Agent/Animations/ABP_test',
 '/Game/Blueprints/CombatSystem/Combat/ABP_CombatModule',
]
assets = unreal.EditorAssetLibrary.list_assets('/Game/Blueprints', recursive=True, include_folder=False)
paths += [p for p in assets if any(s in p.lower() for s in ['bpi_', 'dungeon', 'generator', 'playerhealth', 'playercombat', 'block', 'taser']) and '3D_Models' not in p]
with open(os.path.join(root, 'ProjectInspection.txt'), 'w', encoding='utf-8') as f:
 for path in dict.fromkeys(paths):
  f.write('\n### '+path+'\n')
  obj = unreal.load_asset(path)
  if not obj: continue
  try:
   if hasattr(unreal, 'ProgressEnemyEditorLibrary'):
    f.write(unreal.ProgressEnemyEditorLibrary.inspect_blueprint(path))
  except Exception as e: f.write(str(e)+'\n')
  try:
   cls=obj.generated_class()
   cdo=unreal.get_default_object(cls)
   f.write('CDO '+str(cdo)+'\n')
   if isinstance(cdo, unreal.Character):
    f.write('MESH '+str(cdo.mesh.skeletal_mesh_asset)+' ANIM '+str(cdo.mesh.anim_class)+'\n')
   for name in dir(cdo):
    if name.startswith('_'): continue
    try:
     value=cdo.get_editor_property(name)
     f.write(name+' = '+str(value)+'\n')
    except: pass
  except: pass
 f.write('\nANIMATIONS\n')
 for p in assets:
  if any(s in p.lower() for s in ['montage','block','bs_enemy','abp_']): f.write(p+'\n')
unreal.log('ENEMY_INSPECTION_COMPLETE')
