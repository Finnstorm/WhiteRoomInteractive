import unreal, os
report = unreal.ProgressEnemyEditorLibrary.run_integration_checks()
with open(os.path.join(os.path.dirname(os.path.dirname(__file__)), 'Verification.txt'), 'w', encoding='utf-8') as f:
    f.write(report)
unreal.log(report)
assert 'RESULT failures=0' in report, 'Runtime verification failed'
