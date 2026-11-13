"""Build the editable evidence dossier from project facts and verified source records."""
from pathlib import Path
import json

ROOT=Path(__file__).parent

def build():
    data=json.loads((ROOT/'content.json').read_text(encoding='utf-8'))
    pages=[]
    def page(title,content): pages.append(dict(title=title,content=content.strip()))
    page('Enemy AI · Hintergrund und Quellen', '''
Fachvortrag zur Blueprint-Version in WhiteRoomInteractive

Name: [eintragen]

Matrikelnummer: [eintragen]

Modul: [eintragen]

Fachdozent:in: [eintragen]

Stand: 13.11.2026 · Projektnachweise: 12.11.2026

Dieses Begleitdossier erklärt die ausgewählten Vortragsthemen, ihre Implementierung, die fachliche Begründung und die Grenzen der vorhandenen Evidenz. Die vollständige Funktionsdokumentation bleibt EnemyInternals.md.

VORLAGE: Pflichtangaben, freigegebene Persona, Production-Ziele und persönlicher Eigenanteil vor Abgabe ergänzen. Die PDF-Dateien anschließend nach dem vorgegebenen Muster Matrikelnummer_NameDerAbgabe.pdf benennen. Es wurde nichts hochgeladen.
''')
    page('01 · Themenwahl und Bewertungsbezug', '''
## Leitfrage
Wie unterstützt eine verständliche, konfigurierbare Enemy AI das angestrebte Spielerlebnis?

Für acht Minuten werden drei technische Entscheidungen vertieft: Wahrnehmung mit Gedächtnis, begrenzte Kampfressourcen und progressionabhängige Entscheidungszeiten. Der Funktionskatalog bleibt im Dossier. Das schafft Platz für Begründung und Analyse statt einer langen Feature-Liste.

| Kriterium laut Bewertungsbogen | Umsetzung im Entwurf |
| --- | --- |
| Einstieg, 17 % | Konkrete Ecksituation als verständliches Problem. |
| Präsentation, 17 % | Ergebnis → Begründung → Nachweis → Ausblick; wenige Aussagen je Folie. |
| Auftreten, 17 % | Sprechtexte als Probenhilfe; Blickkontakt, Pausen und sichere Übergänge. |
| Verständlichkeit, 17 % | Zustandsfolge, Kampfvergleich und drei berechnete Progressionswerte. |
| Fachliche Substanz, 34 % | Quellenbezug, explizite Formeln, Grenzen und getrennte technische/Nutzer-Evidenz. |

Die genannten Gewichtungen ergeben rechnerisch 102 %. Sie werden hier unverändert aus dem Bewertungsbogen wiedergegeben; es wird keine eigene Notenberechnung erfunden.

## Ziele und Annahmen
Bestätigter technischer Umfang: Patrol/Chase/Search, Sicht- und Schadensreaktion, zeitbasierte Näheerkennung, zwei Nahkampfvarianten, Taser, Block, vier Progressionswerte, Angriffs-Toggles und prozedurales Spawning. Die Blueprint-Version benötigt keine eigenen C++-Gameplayklassen.

Noch nicht bestätigt: eine konkrete Persona und die exakten freigegebenen Production-Ziele. „Verständliche Herausforderung mit taktischen Entscheidungen“ ist deshalb eine Arbeitsannahme, kein dokumentierter Zielgruppenbefund. [Q1: Analysemodell; E1: Implementierung]

## Eigenanteil
Vor dem Vortrag ergänzen: Welche Entscheidungen, Graphen, Tests und Integrationsarbeiten hat die vortragende Person selbst verantwortet? Teamleistungen und persönliche Leistungen sauber trennen.
''')
    page('02 · Alt und Neu: Warum und wie', '''
## Vergleichsrahmen
Alt ist der ursprüngliche BP_Enemy_punch mit zugewiesenem AIC_Enemy_Base und ausgewählten Behavior-Tree-Tasks. Neu ist BlueprintOnly/BP_Enemy. Die C++-Version des neuen Systems ist nicht die Alt-Spalte. Grundlage ist eine Read-only-Graphinspektion am 13.11.2026, keine neue Laufzeitvergleichsstudie. [E5]

| Aspekt | Bisheriger Punch-Gegner | Neuer Blueprint-Gegner |
| --- | --- | --- |
| Steuerung | Controller, Behavior Tree und Blackboard-State. | Sieben Character-Zustände, UpdateBrain und Think. |
| Kampf | Zufällige Punch-Montage mit Rate 1. | Light/Heavy, Taser-Magazin, begrenzter Guard. |
| Progression | SetMovementSpeed mit festen Speedstufen. | Vier Level plus abgeleitete Entscheidungspausen. |
| Bestehende Grundlagen | Wahrnehmung, Lost-Sight/Seeking, Route und Random-Patrol-Tasks vorhanden. | Expliziter LastKnownLocation-Ablauf; Random-Patrouille, keine Routen-/Hörsensorik. |

## Warum umbauen?
Der neue Umfang erfordert Ressourcen und zeitlich unterschiedliche Aktionen. Wir möchten jeweils erklären können, welche Gegenmaßnahme eine Regel ermöglichen soll: Deckung bei verlorenem Sichtkontakt, ein Angriffsfenster nach zwei Taser-Schüssen oder eine andere Antwort auf einen schweren Schlag. Ob diese Möglichkeiten erkannt werden, ist noch zu testen.

## Wie wird der Ablauf verändert?
Alt: Behavior-Tree-Task → Interface-Attack → Bewegung stoppen → zufällige Montage → Attack-End-Verbindungen. Neu: Entscheidungspause prüfen → Startbedingungen prüfen → Impact planen → Treffer und Ressourcen prüfen → erholen → nächste Chase-Entscheidung. Die ausgewählte Montage wird für die neue Mindestaktionsdauer gecacht.

## Was bleibt, was kostet die Änderung?
Punch-Assets und Referenz-Animationssetup werden weiterverwendet. Wahrnehmung und Suche wurden nicht erstmals eingeführt. Die neue Aufteilung macht den aktuellen Ablauf lokal nachvollziehbar, bündelt aber mehr Verantwortung im Character und erzeugt zusätzliche Timer-/Übergangsabhängigkeiten. Behavior Trees sind damit nicht grundsätzlich schlechter. Ohne A/B-Tests keine bessere Performance oder höheren Spielspaß behaupten. Ausführliche Belege: Alt_Neu_Vergleich.md.
''')
    page('02 · Architektur und Ablauf', '''
## Verantwortlichkeiten
| Asset | Kernaufgabe |
| --- | --- |
| BP_Enemy | Zustände, Wahrnehmung, Navigation, Kampftimer, Scaling, Finisher, Tod. |
| AC_EnemyHealth_BP | Gesundheitswerte und Health-basierte Todes-/Finisherflags. |
| AC_EnemyCombat_BP | Point Damage mit Gegner als Verursacher. |
| ABP_Enemy | Bewegungspose und Montage-Slot. |
| BP_PlayerBridge | Finisher-Input und zeitlich begrenzter Movement-Input-Lock. |
| BP_EnemySpawner | Räume, Quoten, gültige Spawnpunkte und Progressionsübergabe. |
| BP_TestObserver | Optionaler Smoke-Test, im normalen Play inaktiv. |

## Updatefolge
BeginPlay speichert den PatrolOrigin, initialisiert Health, wendet Difficulty an, erzeugt den AIController und findet/erstellt den PlayerBridge. Anschließend wird die Darstellung aktualisiert.

Der Character-Tick ist auf 0,05 s eingestellt. Reihenfolge: UpdateResources → UpdateBrain → AdvanceFinisher → UpdatePresentation. Die Timer arbeiten mit DeltaSeconds; sichtbare Reaktionen sind durch das Updateintervall quantisiert.

## Zwei unterschiedliche Zustandsmaschinen
Der Gameplay-Zustandsautomat verwendet Integer: 0 Patrol, 1 Chase, 2 Search, 3 Attack, 4 Block, 5 Finisher, 6 Dead. SetState kümmert sich um Übergangsfolgen, etwa Guard-Montage stoppen, Bewegungsziel verwerfen und Entscheidungspause setzen.

Die Animations-State-Machine im ABP ist davon getrennt: Idle/Walk/Run, Falling und Landing erzeugen Posen. Die Animationszustände entscheiden nicht über Taser, Schaden oder Verfolgung.

Es gibt hier keinen Behavior Tree, Blackboard oder trainiertes Modell. Die vorhandene, kleine Verhaltensmenge wird durch normale Blueprint-Funktionen dispatcht. Das ist nachvollziehbar; mit stark wachsender Zustands-/Übergangszahl steigt der Pflegebedarf. [E1]
''')
    page('03 · Wahrnehmung und begrenztes Wissen', '''
## Implementierte Regeln
UpdateSenses holt Player Character 0. Bei gültigem, beschädigbarem Spieler prüft ClearLineTo eine Sichtlinie; Distanz und Blickkegel bestimmen Visible. Defaults: 18 m Sichtweite, 65° Halbwinkel. Das sind 130° Kegelöffnung, nicht 65° Gesamtöffnung.

NearPlayer benötigt freie Sicht und höchstens 2,5 m Abstand, aber keinen Blickkegel. ProximityTime muss 2,5 s durchgehend steigen. Beim Verlassen der Bedingung wird sie zurückgesetzt. NotifyPlayerDamage alarmiert bei einem zugeordneten, playerkontrollierten Character ohne diese Wartezeit.

Bei Wahrnehmung werden Target und LastKnownLocation aktualisiert. Ohne Wahrnehmung läuft UnseenTime hoch. Think verfolgt dann den letzten bekannten Ort. Die Suche beginnt beim Erreichen dieses Bereichs nach SightMemory oder nach dem zusätzlichen Timeout; sie dauert standardmäßig zehn Sekunden.

## Was das für den Vortrag bedeutet
Die Ecksituation zeigt die Verbindung von Regel, beobachtbarer Reaktion und gewünschter Verständlichkeit. Die genaue Ausgestaltung stammt aus E1. Die fachlichen Prinzipien sind in Q2/Q3 eingeordnet; daraus folgt kein Nachweis, dass unsere Werte optimal sind.

## Prüffall
1. Spieler sichtbar platzieren: Patrol → Chase beobachten.
2. Sicht mit einer Wand unterbrechen: LastKnownLocation muss unverändert bleiben.
3. Zum bekannten Ort laufen lassen und Search abwarten.
4. Hinter dem Gegner 2,5 m nah bleiben; kurze Unterbrechung muss den Nähetimer löschen.

Grenze: Positionswissen kann durch neuen Spielerschaden wieder aktualisiert werden. „Keine Allwissenheit“ sollte deshalb nur auf die Verfolgung nach verlorenem Wahrnehmungskontakt bezogen werden. Keine Hörsensorik oder Squad-Kommunikation behaupten.
''')
    page('04 · Kampf, Animation und Game Feel', '''
| Werkzeug | Basisregel | Zu prüfende Spielerentscheidung |
| --- | --- | --- |
| Leichter Nahkampf | 12 Schaden, 1,35× Play Rate | Timing erkennen und ausweichen/kontern. |
| Schwerer Nahkampf | 25 Schaden, 0,75× Play Rate | Längeres Zeitfenster gegen höheren Schaden abwägen. |
| Taser | 2 Schüsse, dann 6 s Recharge | Distanz/Deckung nutzen; kein automatischer Reload nach einem Schuss. |
| Block | 75 % Reduktion für 3 akzeptierte Treffer, danach Recharge | Position oder Angriffszeitpunkt ändern. |

Die rechte Spalte enthält Designhypothesen. Ob Spieler diese Möglichkeiten verstehen und nutzen, ist noch nicht empirisch gezeigt. Game Feel wird hier über Timing, Reaktion und Rückmeldung besprochen, nicht als bereits gemessener Kennwert.

## Ausführung
TryAttack prüft Startbedingungen und plant ImpactTime und ActionTime. ResolveAttack prüft am Einschlag erneut Deckung, Entfernung und beim Nahkampf die Ausrichtung. Ein gestarteter Angriff ist also noch kein bestätigter Treffer. Ein ungeblockter Treffer gegen den Gegner kann einen ausstehenden Impact abbrechen.

Nominaler Impact: leicht 0,5 / 1,35 ≈ 0,370 s; schwer 0,5 / 0,75 ≈ 0,667 s. Die Aktion wartet zusätzlich mindestens Montage-Länge / Rate + 0,1 s. Die autonom nächste Entscheidung folgt erst wieder im Chase-Zustand. [E1]

## Animation als Teil der Rückmeldung
Light und Heavy verwenden dieselben drei Punch-Montagen. ActiveMeleeMontage hält die tatsächlich ausgewählte Animation fest, damit die Dauerberechnung keine andere zufällig auswählt. Der Referenzaufbau entspricht BP_Enemy_punch mit passendem Rig, AnimGraph und Slot. Der Block und der Taserstrahl sind noch Platzhalter. Q5 ist die technische Referenz für Montage/Slot-Mechanik; die konkrete Korrektur und Posen sind E1/E3.

Die 75-%-Reduktion gilt nur für akzeptierte Treffer im Guard-Winkel und mit verfügbaren Charges. Rückwärtige oder nicht zugeordnete Treffer sind keine pauschal geblockten Treffer. Balancingwerte einzeln testen, bevor eine gute taktische Wirkung behauptet wird.
''')
    page('05 · Progression und Intelligenz-Heuristik', '''
## Berechnung
Alle vier Eingaben sind Integer und werden auf 0 bis 10000 begrenzt. Das Intelligenzlevel ist ihr Mittelwert. Alpha = clamp(Mittelwert / max(0,01, IntelligenceLevelForMaximum), 0, 1); der Standardnenner ist 10.

DecisionInterval = 0,35 − 0,25 × Alpha

PathUpdateInterval = 0,40 − 0,25 × Alpha

| Mittelwert | Alpha | Entscheidung | Pfadauftrag |
| --- | --- | --- | --- |
| 0 | 0 | 0,350 s | 0,400 s |
| 5 | 0,5 | 0,225 s | 0,275 s |
| 10 oder mehr | 1 | 0,100 s | 0,150 s |

Das sind berechnete Sollintervalle, keine gemessene Framerate, Reaktionszeit eines Menschen oder CPU-Latenz. Die tatsächliche Ausführung hängt vom Tick und aktuellen Aktionszustand ab.

## Was sich im Verhalten ändert
Schwere Angriffe werden bei hohem Alpha gegen Ziele innerhalb 80 % der Nahkampfreichweite und mit höchstens 75 cm/s horizontalem Tempo bevorzugt. Die Wahrscheinlichkeit nähert sich dort 80 %, andernfalls 0 %. Wenn nur Heavy eingeschaltet ist, bleibt dieser Angriff verwendbar. Verletzte Gegner erhalten mehr Blockpräferenz, gedeckelt auf 85 %. Verfolgung kann sichtbare Bewegung bis zu 0,45 s vorausberechnen; verdeckte Bewegung wird nicht laufend weiter extrapoliert. [E1]

## Grenzen und Kopplungsrisiko
Die Heuristik lernt nicht. Sie erfasst keine Spielerleistung und betreibt keine automatische Difficulty-Anpassung. Gleiche Gewichtung bedeutet: Auch ein reiner Health-Anstieg erhöht Entscheidungstempo. Das kann unerwünschte Doppelsteigerungen erzeugen. In einem Balancingtest deshalb Stats und Alpha getrennt variieren; später gegebenenfalls einen separaten Intelligenzparameter einführen.

Q2 liefert Kontext für wahrgenommene Glaubwürdigkeit; die Gleichung selbst ist eine eigene Projektentscheidung und wird nicht dieser Quelle zugeschrieben.
''')
    page('06 · Spawning und Systemintegration', '''
## Prozedurales Spawning
SpawnPending wartet im Dungeonmodus auf Map_Generator.MapComplete und einen gültigen Spieler. Danach verarbeitet es Master_Room-Instanzen; Spawn_Room im Objektnamen und zu nahe Raumursprünge werden ausgeschlossen. ProcessRoom verwaltet parallele Arrays für Räume, erfolgreiche Spawns und zufällige Lebenszeitquoten.

Ein Kandidat benötigt einen erreichbaren NavMesh-Punkt, Mindestabstand zum Spieler, Capsule-Clearance und Abstand zu anderen BP_Enemy. Erst ein erfolgreicher Spawn verbraucht Quote. Der neue Gegner erhält die vier Difficulty-Werte nach dem Spawn. MaximumAlive begrenzt lebende Population, SpawnBudget erfolgreiche Spawns pro Update. MapComplete ist keine Garantie für Navigation. [E1, Q6]

## Testarena
L_EnemyTest_BP enthält eine große ebene Fläche, Navigation, PlayerStart und einen Blueprint-Spawner mit drei Lebenszeitplätzen. Im Standalone-Modus dient die Spawnerposition als Raumursprung. Kills setzen die verbrauchte Quote nicht zurück; die Arena ist kein Endlos-Respawn-System.

## Bestehende Systeme
AC_EnemyCombat_BP liefert Point Damage an die bestehende Spielerlogik. Incoming AnyDamage führt beim Gegner zu ReceiveCombatDamage. BP_PlayerBridge übernimmt den zusätzlichen Finisher-Input und den Taser-Movement-Lock; die alte Spieler-Blueprint bleibt unverändert. Die Gegner finden außerdem Widgets und optional den Sanity-Actor.

Wenn die Blueprint-Population im ProceduralDungeon getestet wird, muss die automatische Dungeon-Population des alten C++-Plugins deaktiviert sein, um beide Versionen nicht zu vermischen. Die Gameplay-Assets der neuen Version selbst verwenden Standard-Engine-Eltern und keine direkten ProgressEnemy-Runtimeaufrufe.

## Noch prüfen
Mehrere Map-Seeds; fehlende/verspätete Navigation; enge Durchgänge; Spawnpunkte hinter Wänden; Abstände zum Spieler; konkurrierende Spawner. Das aktuelle Clearance-Volumen ist fest konfiguriert und wird nicht automatisch vom später geänderten Enemy-Capsule-Setup übernommen. Mehrspielerbetrieb und Replikation sind nicht implementiert bzw. geprüft.
''')
    page('07 · Nachweise und geplanter Playtest', '''
## Vorhandene Daten
| Nachweis | Tatsächliche Aussage | Nicht daraus ableitbar |
| --- | --- | --- |
| E2: 55 PASS, failures=0 | Geskriptete Kompilier-, Parent- und Gameplayprüfungen bestanden. | 55 unabhängige Experimente oder Zufriedenheit. |
| E3: 27 PASS, failures=0 | Setup und ausgewertete Posen stimmen in den Testfällen mit der Referenz überein. | Perfekte Animation in jeder Spielsituation. |
| E4: 3 / 3 / 1 | 3 Gegner, 3 Controller, 1 bewegter Gegner am Stichprobenzeitpunkt. | Dauerhafte Bewegung aller Gegner oder Performanceprofil. |

Die Reports stammen vom 12.11.2026 und wurden für diese Dokumentation gelesen, nicht als neue Nutzerstudie ausgeführt. Bestehende Missing-Sword-Fehler der alten BP_Enemy_Base sind hiervon getrennt. Die Zahlen werden nicht mit den Tests der C++-Version vermischt.

## Vorgeschlagene nächste Untersuchung
Zunächst 5–8 zur bestätigten Persona passende Personen für einen explorativen Test einladen. Diese Zahl ist ein pragmatischer Plan, keine statistisch begründete Mindeststichprobe. Gleiche kurze Szenen: Sichtverlust, leichter/schwerer Schlag, zwei Taser-Schüsse, Schilderschöpfung.

Erfassen: erste erkannte Ankündigung; eingesetzte Gegenmaßnahme; Treffer-/Fehlerereignisse; kurze Erklärung nach der Szene; subjektive Verständlichkeit, Fairness und Herausforderung getrennt auf einer 1–7-Skala. Die selbst formulierten Skalen sind explorativ und keine validierte PENS-Erhebung.

Variantenreihenfolge wechseln, um Übungseffekte zu reduzieren. Bei Timingtests Schaden und Bewegung konstant halten. Median und Spannweite berichten, Beobachtungen und Widersprüche dokumentieren. Keine Signifikanz- oder Repräsentativitätsbehauptung bei dieser kleinen Pilotstudie.

Vorab vorgeschlagene Abnahmekriterien: Jede kritische Fehlfunktion beheben; zentrale Regeln sollen von einer vorher vereinbarten Mehrheit korrekt erklärt werden. Den Schwellenwert vor Teststart festlegen, nicht nachträglich passend wählen. Anschließend gezielte Iteration und erneuter Test.
''')
    page('08 · Persona, Ziele und Analyse', '''
## Verbindliche Angaben fehlen noch
Die Persona, Teamgröße, der individuelle Eigenanteil und die freigegebenen Production-Ziele liegen nicht als Projektquelle vor. Weder diese Felder noch Erfolgsmessungen werden erfunden.

| Nachzutragen | Leitfrage für die Zuordnung |
| --- | --- |
| Persona und relevanter Bedarf | Welches tatsächliche Spielmotiv oder Problem adressiert der Gegner? |
| Production-Ziel | Welcher vorher definierte, überprüfbare Meilenstein wurde erreicht? |
| Fachlicher Eigenanteil | Welche technische/designbezogene Entscheidung kann die Person selbst begründen? |
| Erfolgskriterium | Welche Beobachtung würde die gewünschte Wirkung stützen oder widerlegen? |

## Vorläufige Wirkungskette
Regel: Ein gestarteter Schlag prüft Deckung am Impact erneut. Beobachtbares Verhalten: Rückzug hinter eine Wand kann den Treffer verhindern. Gewünschte Wirkung: Der Spieler erlebt eine verständliche Gegenmaßnahme. Dieses Beispiel ist Projektanalyse, kein Ergebnis einer bereits durchgeführten Spielerbefragung.

Ein zweites Beispiel ist der leere Taser nach zwei Schüssen. Das mögliche Angriffsfenster ist nur dann eine taktische Möglichkeit, wenn der Spieler die Schüsse und die Pause erkennen kann. Ein funktionierender Timer allein belegt das nicht.

## Warum die ausgewählten Quellen passen
Q1 bietet die Analysebegriffe; Q2/Q3 adressieren die Verständlichkeit und Wahrnehmung eines Gegners; Q4 ordnet Kompetenz und Handlungsmöglichkeiten ein; Q5/Q6 erklären die tatsächlich genutzten Engine-Mechanismen. Methodenliteratur, Praxisbericht und technische Dokumentation werden dabei nicht als gleichartige empirische Belege behandelt.

## Ausblick mit Priorität
1. Blockpose und Taser-Ankündigung fertigstellen.
2. Persona bestätigen und Pilot-Playtest planen.
3. Stats und Entscheidungszeit einzeln balancen.
4. Mehrere Map-Seeds und Navigationsfälle regressionsprüfen.
5. Erst nach neuen Daten die Aussage über Spielerlebnis und Persona-Passung aktualisieren.
''')
    for start in (0,3):
        blocks=[]
        for q in data['sources'][start:start+3]:
            blocks += [f'## {q["id"]} · {q["short"]}',q['title'],q['detail'],f'**Befund:** {q["finding"]}',f'**Übertragung:** {q["transfer"]}',f'**Grenze:** {q["limit"]}',f'**Direktquelle:** {q["url"]}','Abruf: 13.11.2026.']
        page('09 · Quellenkritik' if start==0 else '10 · Quellenkritik', '\n\n'.join(blocks))
    page('11 · Projektnachweise und Prüfungsfragen', '\n\n'.join(f'## {e["id"]} · {e["title"]}\n\n{e["description"]}\n\nDatei: {e["path"]}' for e in data['evidence'])+'''

## Kritische Fragen mit Antwortkern
- „Ist der Gegner wirklich intelligent?“ Regelbasierte Heuristik und messbare Timer, kein Lernen; Wahrnehmung durch Nutzer noch offen.
- „Warum Blueprint?“ Erfüllt den ausdrücklich gewünschten Blueprint-Umfang, nutzt vorhandene Engine-Systeme und erlaubt visuelles Tuning; keine pauschale Performanceüberlegenheit behaupten.
- „Warum sind die Ergebnisse gut?“ Die vereinbarten Funktionen sind vorhanden und durch benannte Tests gestützt; die Erlebnisziele sind noch zu validieren.
- „Warum diese Zahlen?“ Konfigurierbare Ausgangswerte aus der Implementierung, keine Literatur-Optima.
- „Wo ist der nächste Engpass?“ Lesbare Rückmeldung, getrenntes Balancing und empirischer Persona-Abgleich.
''')
    page('12 · Abgabe, Probe und faire Sprechzeit', '''
## Formale Abgabe
Gefordert ist PDF mit genau einer Deckblattseite am Anfang, einschließlich Matrikelnummer, Modul und Fachdozent:in. Name und Beitragstitel sind zusätzlich vorbereitet. Dateimuster nach Ergänzung: 123456_Name_EnemyAI_Fachvortrag.pdf. Der Vortrag muss am Vortag der Prüfung hochgeladen werden; das konkrete Prüfungsdatum wurde nicht genannt. Prüfungsbeginn laut Aufgabe: 09:30 Uhr.

Die mitgelieferten Dateien sind wegen offener Pflichtdaten als VORLAGE bezeichnet. Sie sind noch nicht unmittelbar abgabefertig. Kein Upload wurde ausgeführt. Die bearbeitbare PowerPoint ist eine Arbeitsdatei; die geforderte Einreichung ist die PDF.

## Zeitplan
Folien 1–10: 20 + 60 + 40 + 65 + 65 + 70 + 50 + 35 + 45 + 30 Sekunden = 480 Sekunden = 8:00 Minuten. Folien 11–12 sind Quellenanhang. Diese Dauer ist geplant, nicht durch eine Sprechprobe gemessen. Die vereinfachten Kernfolien zeigen Warum, Wie und ein Beispiel oder eine Grenze; Funktionsdetails und Trade-offs stehen in den Notizen.

Die Aufgabenstellung kombiniert „5–10 Minuten ±10 %“ mit „4–11 Minuten“. Mathematisch wären 10 % unter fünf Minuten 4:30. Deshalb nicht an die Untergrenze planen; acht Minuten liegen in beiden Lesarten sicher. Für den konkreten Prüfungsmaßstab die Lehrperson fragen.

## Teamregel
Dieser Entwurf ist ein eigener Beitrag für eine Person. Bei zwei Personen ergibt ein einfaches Halbieren nur vier Minuten und erfüllt nicht die Zielvorgabe. Für jede weitere Person einen gleich langen eigenen Beitrag planen oder das Thema fachlich erweitern. Sprechzeiten einzeln stoppen; gleiche Folienzahl garantiert keine gleiche Zeit.

## Vortragsprobe
Zweimal laut mit Stoppuhr proben. Zielkorridor 7:30–8:30. Erst den roten Faden ohne Notizen erklären, dann Zahlen und Übergänge sichern. Den Einstieg mit Blickkontakt sprechen; Fachbegriffe einmal definieren; keine Blueprint-Knoten ablesen. Rückfragen nach Datum, Quelle, Testgrenze und Eigenanteil trainieren.

Ein echter Projektclip ist optional, aber nicht enthalten. Eine Live-Demo nur mit getesteter Zeitreserve; die vorhandene Textfolge ist die vollständige Rückfallebene. Mit fehlender Persona oder offenen Pflichtdaten nicht unverändert einreichen.
''')
    for number, section in enumerate(pages[1:],1):
        section['title']=f'{number:02} · '+section['title'].split(' · ',1)[-1]
    (ROOT/'dossier_pages.json').write_text(json.dumps(pages,ensure_ascii=False,indent=2),encoding='utf-8')
    (ROOT/'EnemyAI_Hintergrund_und_Quellen.md').write_text('\n\n'.join('# '+p['title']+'\n\n'+p['content'] for p in pages)+'\n',encoding='utf-8')

if __name__=='__main__':
    from focus_dossier import build as build_current_focus
    build_current_focus()
