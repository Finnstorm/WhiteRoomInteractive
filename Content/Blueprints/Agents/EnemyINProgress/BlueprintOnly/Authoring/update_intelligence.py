"""Patch only AI functions and new variables; retain the existing rig, animations and tuning."""
import json, os, runpy
import unreal
from pathlib import Path

folder = Path(__file__).parent
namespace = runpy.run_path(str(folder / 'generate_blueprints.py'))
enemy = next(a for a in namespace['assets'] if a['path'].endswith('/BP_Enemy'))
names = {'SetState', 'SetDifficultyLevels', 'RefreshIntelligence', 'RefreshCombatChoices',
         'MoveToPoint', 'Think', 'UpdateBrain', 'ReceiveCombatDamage'}
variables = {'IntelligenceLevelForMaximum', 'IntelligenceLevel', 'IntelligenceAlpha',
             'DecisionInterval', 'PathUpdateInterval', 'PursuitGoal',
             'HeavyDecisionChance', 'BlockDecisionChance'}
patch = {'path': enemy['path'], 'patch': True,
         'variables': [v for v in enemy['variables'] if v['name'] in variables],
         'graphs': [g for g in enemy['graphs'] if g['name'] in names]}
assert len(patch['graphs']) == len(names)
spec = folder / 'intelligence_patch.json'
spec.write_text(json.dumps({'assets': [patch]}, indent=2), encoding='utf-8')
report = unreal.ProgressEnemyEditorLibrary.build_blueprint_only(str(spec))
(folder.parent / 'IntelligenceBuildReport.txt').write_text(report, encoding='utf-8')
unreal.log(report)
assert report.startswith('PASS'), report
