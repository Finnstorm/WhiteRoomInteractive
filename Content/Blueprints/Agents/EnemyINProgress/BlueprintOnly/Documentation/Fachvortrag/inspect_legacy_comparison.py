"""Read-only inspection for the presentation's explicitly scoped legacy comparison."""
import json
from pathlib import Path
import unreal

root=Path(__file__).parent
legacy='/Game/Blueprints/Agents/Enemy-Agent/EnemyAI/Enemy_punch/'
paths=[legacy+'BP_Enemy_punch',legacy+'Behaviour/AIC_Enemy_Base',
       legacy+'Behaviour/Tasks/BTT_DefaultAttack',legacy+'Behaviour/Tasks/BTT_MoveAlongRoute',
       legacy+'Behaviour/Tree/SubTree/BTT_FindRandomPatrolPoint',
       '/Game/Blueprints/CombatSystem/Combat/AC_EnemyCombat']
records={}
build=root/'.build';build.mkdir(exist_ok=True)
with (build/'LegacyComparisonInspection.txt').open('w',encoding='utf-8') as stream:
    for path in paths:
        bp=unreal.load_asset(path)
        assert bp,path
        dump=unreal.ProgressEnemyEditorLibrary.inspect_blueprint(path)
        stream.write('\n### '+path+'\n'+dump)
        records[path]=dict(graphs=[l[6:] for l in dump.splitlines() if l.startswith('GRAPH ')])
        if path.endswith('/BP_Enemy_punch'):
            cdo=unreal.get_default_object(bp.generated_class())
            records[path]['ai_controller_class']=str(cdo.get_editor_property('ai_controller_class'))
        stream.write('\nRECORD '+json.dumps(records[path])+'\n')
(build/'LegacyComparisonInventory.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
unreal.log('LEGACY_COMPARISON_INSPECTION_READY')
