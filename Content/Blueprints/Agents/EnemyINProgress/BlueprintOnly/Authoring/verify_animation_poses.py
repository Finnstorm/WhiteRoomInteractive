import unreal, os
report=unreal.ProgressEnemyEditorLibrary.verify_punch_animation_poses()
with open(os.path.join(os.path.dirname(os.path.dirname(__file__)),'AnimationPoseVerification.txt'),'w',encoding='utf-8') as f: f.write(report)
unreal.log(report)
assert 'RESULT failures=0' in report, report
