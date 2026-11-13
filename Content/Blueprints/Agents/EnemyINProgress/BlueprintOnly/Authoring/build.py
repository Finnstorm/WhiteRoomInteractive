import unreal, runpy, os
folder=os.path.dirname(__file__)
namespace=runpy.run_path(os.path.join(folder,'generate_blueprints.py'))
specification=namespace['write_spec']()
for asset in namespace['assets']:
    path=asset['path']
    assert path.startswith('/Game/Blueprints/Agents/EnemyINProgress/BlueprintOnly/')
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError('Close Unreal and move the previously generated asset aside before rebuilding: '+path)
report=unreal.ProgressEnemyEditorLibrary.build_blueprint_only(str(specification))
with open(os.path.join(folder,'BuildReport.txt'),'w',encoding='utf-8') as f: f.write(report)
unreal.log(report)
assert report.startswith('PASS'), report
animation_report=unreal.ProgressEnemyEditorLibrary.match_punch_animation_setup()
with open(os.path.join(os.path.dirname(folder),'AnimationFixReport.txt'),'w',encoding='utf-8') as f: f.write(animation_report)
assert animation_report.startswith('PASS'), animation_report
