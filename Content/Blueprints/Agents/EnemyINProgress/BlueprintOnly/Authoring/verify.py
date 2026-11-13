import unreal, os
report=unreal.ProgressEnemyEditorLibrary.verify_blueprint_only()
report='\n'.join(line for line in report.splitlines() if not line.startswith('DIAGNOSTIC '))+'\n'
with open(os.path.join(os.path.dirname(os.path.dirname(__file__)),'Verification.txt'),'w',encoding='utf-8') as f: f.write(report)
unreal.log(report)
assert 'RESULT failures=0' in report, report
