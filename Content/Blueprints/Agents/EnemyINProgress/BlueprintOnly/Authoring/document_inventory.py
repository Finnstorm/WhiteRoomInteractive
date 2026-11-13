"""Read-only Blueprint graph inventory used to check the internal reference document."""
import json
from pathlib import Path
import unreal

root = '/Game/Blueprints/Agents/EnemyINProgress/BlueprintOnly/'
names = ['BP_Enemy', 'AC_EnemyHealth_BP', 'AC_EnemyCombat_BP', 'ABP_Enemy',
         'BP_PlayerBridge', 'BP_EnemySpawner', 'BP_TestObserver']
inventory = {}
for name in names:
    description = unreal.ProgressEnemyEditorLibrary.inspect_blueprint(root + name)
    assert description and 'BLUEPRINT ' in description, name
    inventory[name] = [line.removeprefix('GRAPH ').strip()
                       for line in description.splitlines() if line.startswith('GRAPH ')]
destination = Path(__file__).parent.parent / 'Documentation' / 'GraphInventory.json'
destination.parent.mkdir(exist_ok=True)
destination.write_text(json.dumps(inventory, indent=2), encoding='utf-8')
unreal.log('BLUEPRINT_DOCUMENTATION_INVENTORY_READY ' + str(inventory))
