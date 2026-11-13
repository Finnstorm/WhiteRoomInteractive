"""Reopen and validate both enemy implementations and the corrected Blueprint poses."""
from pathlib import Path
import unreal

root = Path(__file__).parent.parent
for destination, check in [
    ('Verification.txt', unreal.ProgressEnemyEditorLibrary.run_integration_checks),
    ('BlueprintOnly/Verification.txt', unreal.ProgressEnemyEditorLibrary.verify_blueprint_only),
    ('BlueprintOnly/AnimationPoseVerification.txt', unreal.ProgressEnemyEditorLibrary.verify_punch_animation_poses),
]:
    report = check()
    (root / destination).write_text(report, encoding='utf-8')
    unreal.log(destination + '\n' + report)
    assert 'RESULT failures=0' in report, destination

registry = unreal.AssetRegistryHelpers.get_asset_registry()
options = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=True, include_hard_package_references=True)
dependencies = registry.get_dependencies('/Game/Blueprints/Agents/EnemyINProgress/BlueprintOnly/BP_Enemy', options)
assert '/Script/ProgressEnemy' not in [str(d) for d in dependencies], dependencies
unreal.log('INTELLIGENCE_VERIFICATION_PASS: both versions; animation poses; Blueprint runtime independence')
