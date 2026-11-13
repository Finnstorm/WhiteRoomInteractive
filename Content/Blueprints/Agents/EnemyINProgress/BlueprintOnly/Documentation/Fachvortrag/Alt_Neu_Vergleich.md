# Alter und neuer Gegner: belegter Vergleich

Vergleichsstand: 13.11.2026. „Alt“ bezeichnet hier **BP_Enemy_punch** samt zugewiesenem **Enemy_punch/Behaviour/AIC_Enemy_Base** und ausgewählten Tasks. „Neu“ bezeichnet **EnemyINProgress/BlueprintOnly/BP_Enemy**. Die frühere C++-Version des neuen Systems ist nicht die Alt-Spalte.

## Kurzvergleich

| Aspekt | Bisheriger Punch-Gegner | Neuer Blueprint-Gegner |
| --- | --- | --- |
| Steuerung | Zugewiesener Blueprint-AIController; Run Behavior Tree; Blackboard-Zustand und benannte State-Handler. | Standard-AIController für Bewegung; sieben Character-Zustände, verarbeitet in UpdateBrain und Think. |
| Wahrnehmung | AIPerception, Handler für Sight, Damage, Lost Sight und Forgot Actor; außerdem Sound-/Investigating-Handler vorhanden. | Eigene Sichtlinien-/Kegelprüfung, kontinuierlicher Nähetimer und explizite Reaktion auf zugeordneten Spielerschaden. Keine Hörsensorik implementiert. |
| Verfolgung und Suche | Lost-Sight-Handler ruft Set State Seeking auf. Der alte Gegner ist deshalb nicht als allwissend oder suchunfähig zu beschreiben. | LastKnownLocation wird bei Wahrnehmung oder zugeordnetem Schaden geschrieben; Chase/Search verwenden den gespeicherten Punkt. |
| Patrouille | GetPatrolPath, BTT_MoveAlongRoute und BTT_FindRandomPatrolPoint sind vorhanden. | Zufällige erreichbare Ziele um PatrolOrigin, Suchziele um LastKnownLocation. Keine konfigurierte Patrol-Route. |
| Angriff | Attack stoppt Bewegung und spielt eine zufällige PunchAnims-Montage mit PlayRate 1. Der DefaultAttack-Task bindet OnAttackEnd. | Light/Heavy aus gleichem Montagepool bei unterschiedlichen Raten/Schäden; zusätzlich Taser und Guardressourcen. |
| Bewegung/Progression | SetMovementSpeed wählt feste Werte aus E_Speeds: 0, 220, 270, 350. | Vier externe Level; zusätzliche Ableitung von Entscheidungspause, Pfadintervall und Auswahlwahrscheinlichkeiten. |
| Animation | Bestehender Nurse-/Punch-Referenzaufbau. | Referenzsetup wird wiederverwendet; ausgewählte Montage ist gecacht und bestimmt die Mindestaktionsdauer. |
| Nachweise | Für diesen Auftrag Graphen gelesen; keine neue Alt-Version-Testsuite ausgeführt. | Vorhandene 55 Blueprint-/Gameplay- und 27 Animationschecks; kein A/B-Playtest gegen Alt. |

## Warum ändern?

Der vereinbarte neue Funktionsumfang erfordert begrenzte Ressourcen, unterschiedlich schnelle Aktionen und extern steuerbare Progression. Dafür wurden Übergänge, Timer und Abhängigkeiten explizit gemacht. Das ermöglicht eine konkrete Frage pro System: Welchen Spielraum soll der Spieler bekommen, welche Regel erzeugt ihn, und wie kann diese Regel geprüft werden?

Das ist keine Begründung dafür, dass Behavior Trees grundsätzlich schlechter wären. Die alte Aufteilung in Controller, Tasks und Interface bietet bereits getrennte Zuständigkeiten. Der neue Character bündelt mehr Logik; dadurch ist der aktuelle Ablauf lokal nachvollziehbar, bei wachsendem Umfang können jedoch mehr Übergänge und Timer-Abhängigkeiten entstehen.

## Wie unterscheiden sich die Abläufe?

**Alt, sichtbarer Angriffspfad:** Behavior-Tree-Task → Attack über Enemy-Interface → Bewegung stoppen → zufällige Punch-Montage → OnAttackEnd/HitActors-Cleanup. Die sichtbaren Montagepfade enthalten sowohl direkte als auch Abschluss-/Unterbrechungsverbindungen zum Delegate. Deren Laufzeitverhalten wurde für diesen Vergleich nicht neu getestet; daraus wird kein pauschaler Fehlerbefund abgeleitet.

**Neu, autonomer Angriffspfad:** Think prüft DecisionTime → TryAttack prüft Startbedingungen → Attack-State hält die Aktion → ImpactTime löst ResolveAttack genau einmal aus → Trefferprüfung und Ressourcenverbrauch → Ende von ActionTime → Chase mit neuer Entscheidungspause.

**Beibehalten:** Punch-Assets, Referenz-Rig/Animationsprinzip sowie Integration mit der bestehenden Spielerlogik. **Erweitert:** neue Kampffähigkeiten, Ressourcen, Progressionswerte und heuristische Entscheidungen. **Anders aufgeteilt:** Controller-/Tree-Verhalten versus Character-Funktionen. **Nicht übernommen:** Patrol-Routen- und Sound-Handler der alten Struktur.

## Belegstellen und Grenzen

Read-only-Inspektion über `inspect_legacy_comparison.py`; Prüfmarker `LEGACY_COMPARISON_INSPECTION_READY`. Geprüfte Pfade:

- `/Game/Blueprints/Agents/Enemy-Agent/EnemyAI/Enemy_punch/BP_Enemy_punch`: Attack, SetMovementSpeed, GetPatrolPath und Controllerzuweisung.
- `/Game/Blueprints/Agents/Enemy-Agent/EnemyAI/Enemy_punch/Behaviour/AIC_Enemy_Base`: Run Behavior Tree, AIPerception-Event und Zustands-/Sense-Handler.
- `Enemy_punch/Behaviour/Tasks/BTT_DefaultAttack`: ReceiveExecuteAI, Interface-Attack und OnAttackEnd-Bindung.
- `Enemy_punch/Behaviour/Tasks/BTT_MoveAlongRoute` und `Enemy_punch/Behaviour/Tree/SubTree/BTT_FindRandomPatrolPoint`: vorhandene Routen-/Zufallspatrouillenaufgaben.
- Neue Implementierung: [EnemyInternals.md](../EnemyInternals.md), Blueprint-Authoring und Intelligence-Patch.

Ein vorhandener Handler belegt zunächst eine Graphfunktion, nicht jede mögliche aktiv konfigurierte Sensorart im laufenden Spiel. Insbesondere wird aus dem vorhandenen Sound-Handler keine vollständige Hörverhaltensprüfung behauptet. Die Bewertung ist auf diese Assets begrenzt, nicht auf alle früheren Enemy-Varianten oder das gesamte Projekt. Keine Behauptung zu besserer Performance, tatsächlicher Fairness oder höherem Spielspaß ohne vergleichende Messung.
