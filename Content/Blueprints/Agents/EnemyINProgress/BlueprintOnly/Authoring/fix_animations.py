import unreal, os
report=unreal.ProgressEnemyEditorLibrary.match_punch_animation_setup()
with open(os.path.join(os.path.dirname(os.path.dirname(__file__)),'AnimationFixReport.txt'),'w',encoding='utf-8') as f: f.write(report)
unreal.log(report)
assert report.startswith('PASS'), report
