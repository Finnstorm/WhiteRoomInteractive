"""V3 companion material for the selected sensory/scaling presentation scope."""
from pathlib import Path
import json

ROOT=Path(__file__).parent

def build():
    data=json.loads((ROOT/'content.json').read_text(encoding='utf-8'))
    pages=[]
    def page(title,content):pages.append(dict(title=title,content=content.strip()))
    page('Enemy AI: Sensory Setup and Difficulty scaling','''
Begleitdossier zur Blueprint-Version · WhiteRoomInteractive · V3

Name: [eintragen]

Matrikelnummer: [eintragen]

Modul: [eintragen]

Fachdozent:in: [eintragen]

Stand: 13.11.2026. Technische Testprotokolle: 12.11.2026.

Der gewählte Abschnitt konzentriert sich auf Sensory Setup, Difficulty Scaling und die Einstellmöglichkeiten für das Design Department. Er erklärt den Stand, die Gründe für die Umsetzung, ihre Vor-/Nachteile und mögliche Erweiterungen.

Die Reihenfolge entspricht dem gewünschten Vortrag. Der Folienentwurf ist für einen achtminütigen Einzelbeitrag geplant. Persönlicher Eigenanteil, bestätigte Persona, verbindliche Production-Ziele und Pflichtangaben fehlen weiterhin. Keine Aussage zu bereits gemessenem Spielspaß oder besserer Performance.

Die V1-/V2-Ausgaben bleiben erhalten. Die vollständige Funktionsreferenz und Alt_Neu_Vergleich.md enthalten zusätzliche technische Details außerhalb dieses Schwerpunkts.
''')
    page('01 · Reihenfolge und Abgrenzung','''
## Folien und Zielzeiten
| Folie | Inhalt | Ziel |
| --- | --- | --- |
| 1 | Enemy AI: Sensory Setup and Difficulty scaling | 0:15 |
| 2 | Nur Themen und Reihenfolge | 0:20 |
| 3 | Comparison Old enemy / New enemy | 0:50 |
| 4 | Sensory Setup erklären | 1:05 |
| 5 | Vor- und Nachteile der Sensorik | 0:40 |
| 6 | Designerzugang zur Responsiveness | 0:55 |
| 7 | Difficulty Scaling: Berechnung | 1:10 |
| 8 | Warum dieser Ansatz und Designerzugang | 0:55 |
| 9 | Vor- und Nachteile des Scalings | 0:45 |
| 10 | Ausbauoptionen beider Systeme | 1:05 |

Summe: 480 Sekunden = 8:00 Minuten. Folien 11–12 sind Quellenanhang ohne geplante Sprechzeit. Die Zeiten sind ein Regieplan und müssen laut geprobt werden. Die Agenda enthält nur die Themen in Reihenfolge; sie erklärt noch keine Funktionen.

## Fachlicher Schwerpunkt
Ergebnisse werden über zwei Fragen begründet: Welche Informationen darf der Gegner erhalten? Und welche Werte und Zeitbedingungen darf das Design Department verändern? Kampfschaden, Block und Bewegung erscheinen nur soweit nötig, um die Difficulty-Faktoren zu erklären. Montage- und Spawnerdetails sind kein eigenes Vortragsthema mehr.

## Bedeutung von „Zugang“
Gemeint ist der Zugang für Designer zu Variablen und Blueprint-Funktionen. Das ist weder eine Behauptung über Barrierefreiheit für Spieler noch ein bereits entwickeltes eigenes Editor-Tool. Persona-Bezug und Production-Ziele müssen vor Abgabe konkret ergänzt werden.
''')
    page('02 · Vergleich: Alt und Neu','''
## Klarer Vergleichsrahmen
Alt: BP_Enemy_punch samt zugewiesenem Enemy_punch/Behaviour/AIC_Enemy_Base und ausgewählten Tasks. Neu: BlueprintOnly/BP_Enemy. Die C++-Version des neuen Prototyps ist nicht die Alt-Spalte. [E5]

| Bereich | Alter Punch-Gegner | Neuer Blueprint-Gegner |
| --- | --- | --- |
| Wahrnehmung | AIPerception und Controller-Handler. | UpdateSenses, ClearLineTo und NotifyPlayerDamage. |
| Sichtverlust | Handle Lost Sight ruft Set State Seeking auf. | LastKnownLocation und expliziter Chase-/Search-Ablauf. |
| Nähe | Für diesen Vergleich kein äquivalenter kontinuierlicher Nähetimer belegt. | ProximityTime steigt nur bei kontinuierlicher Nähe; Unterbrechung setzt zurück. |
| Tuning/Progression | Geprüftes SetMovementSpeed wählt feste Speedstufen. | Vier Level plus abgeleitete Entscheidungs- und Pfadintervalle. |

## Was nicht als Neuheit behauptet wird
Der alte Aufbau besitzt bereits Wahrnehmung, Reaktionen auf Sichtverlust und Patrouillen-Tasks. Außerdem existieren Sound-/Investigating-Handler; ihr Vorhandensein ist kein neuer Laufzeitnachweis für vollständig konfigurierte Hörsensorik. Der neue Aufbau implementiert aktuell keine Hörsensorik und keine Patrol-Route.

## Warum die Änderung trotzdem relevant ist
Die neue Struktur macht die kontinuierliche Näheerkennung, die gespeicherte Zielposition und die externe Progressionsschnittstelle konkret abstimmbar. Die Entscheidung ist für unseren Umfang begründet, nicht als generelle Überlegenheit eines Zustandsautomaten über Behavior Trees. Bei größerem Umfang können zusätzliche Zustandsübergänge und Timer-Abhängigkeiten den Wartungsaufwand erhöhen.

Die Grundlage ist eine Read-only-Graphinspektion vom 13.11.2026. Sie ist kein A/B-Playtest und kein Performancevergleich. Vollständige Belegstellen: Alt_Neu_Vergleich.md. [E1, E5]
''')
    page('03 · Sensory Setup: Ablauf und Zweck','''
## Drei Auslöser
Sicht: UpdateSenses prüft einen gültigen Player Character 0 mit bCanBeDamaged. Visible benötigt Entfernung innerhalb SightRadius, Vorwärtskegel innerhalb SightHalfAngle und eine freie Sichtlinie durch ClearLineTo. Standard: 1800 cm und 65° Halbwinkel, also 130° Kegelöffnung.

Nähe: NearPlayer benötigt höchstens ProximityRadius Abstand und freie Sichtlinie, aber keinen Blickkegel. ProximityTime muss kontinuierlich ProximitySeconds erreichen. Standard: 250 cm für 2,5 s. Eine Unterbrechung setzt den Timer auf null.

Schaden: NotifyPlayerDamage akzeptiert einen zugeordneten, playerkontrollierten Character. Dieser wird sofort als Target übernommen. Unzugeordneter oder nicht vom Spieler stammender Schaden kann Gesundheit reduzieren, ohne diesen Spieler-Alarmweg auszulösen.

## Gedächtnis und Suche
Bei Wahrnehmung bzw. zugeordnetem Spielerschaden werden Target und LastKnownLocation aktualisiert. Nach Kontaktverlust bewegt Think den Gegner zum gespeicherten Ort. Suche startet bei den definierten Erinnerungs-/Ankunftsbedingungen oder dem zusätzlichen Timeout; PatrolOrSearch erkundet erreichbare Punkte und beendet Search nach SearchDuration. Standard: SightMemory 2 s, SearchDuration 10 s.

## Warum so?
Die beabsichtigte Wirkung ist eine nachvollziehbare Gegenmaßnahme: Deckung kann die laufende Kenntnis der Spielerposition unterbrechen. Die Nähezeit ergänzt den Blickkegel, ohne jede kurze Berührung sofort als Alarm zu behandeln. Eine Schadensreaktion macht einen zugeordneten Treffer relevant. Das ist Projektanalyse und eine zu prüfende Wirkungshypothese, keine schon gemessene Fairness. Q2/Q3 begründen den allgemeinen Fokus auf verständliche und konfigurierbare Wahrnehmung; unsere konkreten Zahlen kommen aus E1.

Sicht reagiert momentan binär und ohne zusätzlichen Verdachtstimer. Erneute Wahrnehmung oder ein neuer zugeordneter Treffer kann den Ort wieder aktualisieren. Es gibt keine Garantie absoluter Unwissenheit nach einer Ecke. [E1]
''')
    page('04 · Sensorik: Vor-/Nachteile und Designerzugang','''
## Abwägung
| Vorteile | Nachteile/Grenzen |
| --- | --- |
| Auslöser lassen sich einzeln erklären und testen. | Sichtbarkeit springt zwischen wahr und falsch; abrupte Reaktionen möglich. |
| Sicht, Nähe und Suchdauer sind getrennt einstellbar. | Kein Geräuschkanal im neuen Setup; keine eigene Visualisierung für das Tuning. |

## Wo der Designer ansetzt
BP_Enemy öffnen → Class Defaults für allgemeine Standardwerte; alternativ eine platzierte Instanz auswählen → Details für eine lokale Variante. Variablen sind editierbar. Ein Graphumbau ist für eine Parameteränderung nicht nötig. Die Oberfläche ist noch kein speziell kuratiertes Preset- oder Debug-Tool. [E1]

| Einstellung | Einheit | Bedeutung |
| --- | --- | --- |
| SightRadius / SightHalfAngle | cm / Grad | Reichweite und Halbwinkel des Sichtkegels. |
| ProximityRadius / ProximitySeconds | cm / Sekunden | Nähebereich und kontinuierliche Aufenthaltsdauer. |
| SightMemory | Sekunden | Teilbedingung für den Wechsel vom Verfolgen zum Suchen. |
| SearchDuration / SearchRadius | Sekunden / cm | Suchdauer und Bereich um den letzten bekannten Ort. |

## Responsiveness richtig zuordnen
Erkennung ist nicht dieselbe Zeitbedingung wie die anschließende Handlung. Sichtbarkeit kann sofort alarmieren; nach Eintritt in Chase wartet die nächste autonome Kampfentscheidung auf DecisionTime. DecisionInterval wird durch Difficulty/Intelligenz bestimmt. ProximitySeconds zu erhöhen verzögert daher den Nähealarm, nicht automatisch die Sichtdetektion oder jedes spätere Angreifen.

Beispiel für eine Testvariante: ProximitySeconds von 2,5 auf 4 setzen. Jetzt muss die kontinuierliche Nähe vier Sekunden halten; die Sichtbedingungen bleiben gleich. Das ist ein nachvollziehbares Vergleichsszenario, keine Empfehlung für einen optimalen Wert. Pro Test einen Faktor ändern, gleiche Position/Wand/Szene verwenden und Zustandswechsel beobachten. [E1]
''')
    page('05 · Difficulty Scaling: Berechnung','''
## Eingang und Stat-Berechnung
SetDifficultyLevels erhält Damage, Block, HealthLevelInput und MovementSpeed als Integer. Sie werden auf 0–10000 begrenzt. Die Grenze verhindert ungültige/extreme Eingaben außerhalb dieses Bereichs; sie ist kein empfohlenes Progressionsziel.

| Ergebnis | Formel mit Standardkoeffizienten |
| --- | --- |
| Ausgeteilter Schaden | Basisschaden × (1 + 0,15 × DamageLevel). |
| Maximale Schildtreffer | clamp(3 + BlockLevel, 1, 10000). |
| Maximale Gesundheit | max(1, 100 + 20 × HealthLevel). |
| Bewegungstempo | Basis für Patrol/Chase × (1 + 0,08 × MovementLevel), begrenzt auf 1–750 cm/s. |

## Abgeleitete Entscheidungsgeschwindigkeit
IntelligenceLevel = Mittelwert aller vier Level.

Alpha = clamp(IntelligenceLevel / max(0,01, IntelligenceLevelForMaximum), 0, 1).

DecisionInterval = 0,35 − 0,25 × Alpha. PathUpdateInterval = 0,40 − 0,25 × Alpha. Standard für IntelligenceLevelForMaximum: 10.

Bei durchschnittlichem Level 0 gelten 0,35/0,40 s; bei Level 10 oder höher 0,10/0,15 s. Das sind Sollintervalle. Der 0,05-s-Tick und der aktuelle Aktionszustand beeinflussen die tatsächliche Ausführung. Laufende Angriffe oder Guardzustände werden nicht durch jede neue Entscheidung überschrieben.

## Rechenbeispiel
Level 2/2/3/4 ergeben 160 maximale HP, fünf Schildtreffer, 1,3-fachen Basisschaden und 475,2 cm/s Chase-Geschwindigkeit. Mittelwert 2,75 → Alpha 0,275 → DecisionInterval 0,28125 s. Die Werte kommen aus E1, nicht aus einer externen Optimalitätsstudie.

RefreshCombatChoices verwendet Alpha zusätzlich für situative Heavy-/Blockwahrscheinlichkeiten. Sichtbare Bewegung kann beim Chase vorausberechnet werden. Kein maschinelles Lernen und keine automatische Reaktion auf gemessene Spielerleistung. [E1]
''')
    page('06 · Warum dieser Ansatz und Zugang für Designer','''
## Begründung
Die vier Level sollen von der externen Progression steuerbar bleiben. Benannte Integer-Eingaben geben anderen Blueprint-Systemen eine einfache Schnittstelle. Getrennte Baselines und Koeffizienten erlauben lokale Änderungen am Modell. Der aktuelle lineare Ansatz ist leicht zu berechnen und zu vergleichen; seine Eignung über den gesamten Fortschritt muss noch getestet werden.

Die gleich gewichtete Intelligenzableitung ist eine eigene Projektentscheidung. Sie erweitert Progression um Entscheidungsdruck, ist aber kein wissenschaftlicher Intelligenzwert. Q1 ist ein Analysebezug für die Verbindung von Regel und beabsichtigter Spielerwirkung, keine Quelle für diese Gleichung.

## Welche Werte sind für Designer zugänglich?
| Ziel | Stellschraube |
| --- | --- |
| Allgemeine Grundgesundheit | BaseHealth. |
| Gesundheitszuwachs je Level | HealthPerLevel. |
| Schadensanstieg je Level | DamagePerLevel. |
| Schildkapazität | BaseBlockHits und BlockHitsPerLevel. |
| Bewegungsskalierung | PatrolSpeed, ChaseSpeed, MovementPerLevel, MaximumSpeed. |
| Intelligenzsättigung | IntelligenceLevelForMaximum. |

## Wie Änderungen angewendet werden
Für einen Gegner: SetDifficultyLevels mit vier Werten aufrufen. Für eine Population: dieselbe Spawner-Funktion mit UpdateExisting=true verwenden. Neue erfolgreiche Spawns erhalten die gespeicherten Werte ebenfalls. Bei Basis-/Koeffizientenänderungen die Difficulty erneut anwenden. Im normalen Play kann die Bewegung auf dem nächsten Denkupdate aktualisiert werden; Health/Intelligenz werden im Setter berechnet.

DecisionInterval und PathUpdateInterval sind berechnete Ergebnisse. Sie direkt zu überschreiben ist keine stabile Konfiguration; RefreshIntelligence berechnet sie erneut. Für gewünschte Progression die Eingänge und Koeffizienten nutzen. Sinnvolle nichtnegative Zuwächse wählen; nicht jedes editierbare interne Feld ist als Designer-Stellschraube gedacht. Es gibt noch keine fertigen Kurven, Presets oder Designer-Gizmos. [E1]
''')
    page('07 · Scaling: Vor- und Nachteile','''
## Vorteile
Planbare Grundwerte: Gleiche Level und Konfiguration ergeben gleiche berechnete Stats und Intervalle. Die zufällige Aktionsauswahl macht das gesamte Verhalten trotzdem nicht deterministisch.

Getrennte Stat-Koeffizienten: Designer können z. B. HealthPerLevel ändern, ohne DamagePerLevel mitzuziehen. Eine einheitliche Funktion verbindet Einzelgegner, Spawner und externe Progression.

Live-Skalierung: UpdateMaximum erhält den Gesundheitsanteil statt automatisch vollzuheilen. Ein Gegner mit 85 % Gesundheit hat nach dem Wechsel auf 160 Maximum entsprechend 136 aktuelle HP. Tote Health-Komponenten werden dabei nicht wiederbelebt. [E1, E2]

## Nachteile und Risiken
Gekoppelte Faktoren: HealthLevel erhöht durch den Mittelwert auch Alpha. Mehr Gesundheit kann also gleichzeitig schnellere Entscheidungen erzeugen. Die Stat-Koeffizienten sind getrennt, die Ableitung des Entscheidungsdrucks ist es derzeit nicht.

Lineare Verläufe: Hohe Eingaben können mathematisch gültig sein und trotzdem ungeeignete Gegner ergeben. Die zeitlichen Untergrenzen begrenzen Reaktionsintervalle, nicht jede mögliche Überforderung des Spielers.

Keine Leistungsanpassung: Die KI beobachtet weder Erfolgsquote noch Frustration. Difficulty kommt von außen. Eine Aussage wie „passt sich automatisch an jeden Spieler an“ wäre falsch.

## Was wird tatsächlich geprüft?
E2 dokumentiert 55 Blueprint-/Gameplaychecks, darunter Health-Anteil, Kapazitäten und tatsächliche Alarm-bis-Angriff-Zeiten bei niedrigem/hohem Level. Das sind technische Testfälle, keine Nutzerstudie. Balancing braucht zusätzlich passende Spieler, identische Szenarien und getrennte Variation von Stats und Entscheidungstempo.

Vor dem Test Persona und Zielkriterium bestätigen. Verständlichkeit, Fairness und Herausforderung getrennt erfragen und Verhalten beobachten. Eine kleine explorative Stichprobe liefert Hinweise, keine repräsentative Optimalitätsaussage. [E1, E2; allgemeiner Motivationsbezug Q4]
''')
    page('08 · Ausbauoptionen beider Systeme','''
| Bereich | Mögliche Erweiterung | Nutzen und neuer Aufwand |
| --- | --- | --- |
| Sensorik | Geräusche als Ereignisse mit Ort/Stärke. | Zusätzliche Informationsquelle; erfordert Quellen, Reichweiten und Prioritäten. |
| Sensorik | Verdachtswert statt sofortiger binärer Sicht. | Abstufbare Erkennung; zusätzliche Auf-/Abbauparameter und Grenzfälle. |
| Sensorik | Sichtkegel, Nähebereich und LastKnownLocation visualisieren. | Designer sieht die Regeln beim Tuning; Debugdarstellung aktuell noch nicht vorhanden. |
| Difficulty | Intelligenz als eigener Eingang oder separat gewichten. | Stats und Entscheidungsdruck entkoppeln; zusätzliche Abstimmung nötig. |
| Difficulty | Benannte Presets und Kurven. | Verständliche Designvarianten und nichtlineare Progression; Datenpflege/Validierung nötig. |
| Difficulty | Optionale Anpassung an Spielerleistung. | Denkbare spätere Adaptivität; benötigt klare Metriken, Grenzen und Tests. |

## Priorisierung
Zuerst die Designerübersicht verbessern und Intelligenz entkoppeln. Danach Hör-/Verdachtsmodelle und Kurven anhand konkreter Ziele untersuchen. Automatische Leistungsanpassung erst erwägen, wenn Persona, Messgrößen und gewünschte Wirkung geklärt sind.

Alle Einträge sind geplante Möglichkeiten, keine implementierten Features. Aus dem alten Sound-Handler folgt nicht, dass die neue Version bereits hört. Kein Designer-Preset-Tool oder automatische Adaptivität behaupten.

## Vortragsschluss
Das Ergebnis ist ein explizites Sensory Setup mit editierbaren Parametern und ein von außen steuerbares Difficulty-Modell. Die weitere Arbeit liegt in gezielter Designerunterstützung und dem Nachweis, dass die angestrebte Spielerwirkung tatsächlich entsteht. [E1]
''')
    for group in ([q for q in data['sources'] if q['id'] in ('Q1','Q2','Q3')], [q for q in data['sources'] if q['id']=='Q4']):
        content=[]
        for q in group:
            content += [f'## {q["id"]} · {q["short"]}',q['title'],q['detail'],f'**Befund:** {q["finding"]}',f'**Übertragung:** {q["transfer"]}',f'**Grenze:** {q["limit"]}',f'**Direktquelle:** {q["url"]}','Abruf: 13.11.2026.']
        if len(group)==1:
            for e in data['evidence']:
                if e['id'] in ('E1','E2','E5'):content += [f'## {e["id"]} · {e["title"]}',e['description'],f'Datei: {e["path"]}']
        page('09 · Quellenkritik' if len(group)>1 else '10 · Quellen und Projektnachweise','\n\n'.join(content))
    page('11 · Probe und Abgabe','''
## Formales
Die Vorlage hat eine Deckblattseite. Name, Matrikelnummer, Modul und Fachdozent:in ergänzen. Den Titel unverändert wie gewählt verwenden. Verbindliche Production-Ziele, bestätigte Persona und individuellen Eigenanteil in die Sprechtexte einsetzen. Anschließend PDF entsprechend Matrikelnummer_NameDerAbgabe.pdf benennen und am Vortag hochladen. Ein konkretes Prüfungsdatum wurde nicht genannt; nichts wurde hochgeladen.

## Zeit und Team
Der Beitrag ist für eine Person und acht Minuten geplant. Jede weitere Person benötigt einen gleich langen eigenen 5–10-Minuten-Anteil; diesen Beitrag nicht einfach halbieren. Die Aufgabenstellung enthält unterschiedliche Untergrenzen (±10 % bzw. 4–11 Minuten). Acht Minuten liegen in beiden genannten Lesarten; Zeit durch lautes Proben sichern.

## Probe entlang der neuen Reihenfolge
Agenda kurz benennen, dort noch keine Systeme erklären. Beim Alt-Neu-Vergleich zuerst den Rahmen nennen. Bei Sensorik an einem Eck-/Nähebeispiel erläutern, welche Bedingung auslöst. Bei den Designerparametern erklären, was sich ändert und was davon getrennt bleibt. Bei Difficulty einen Level oder Koeffizienten ändern und die Folgen nachvollziehbar benennen. Vor-/Nachteile als Trade-off erläutern; Ausblick klar als noch nicht umgesetzt markieren.

## Mögliche Rückfragen
- „Warum verzögert ProximitySeconds nicht die Sicht?“ Es ist der kontinuierliche Nähepfad, nicht der Sichtpfad.
- „Wann greift der Gegner nach Entdeckung an?“ Unter den Startbedingungen erst bei fälliger Entscheidung im passenden Zustand; Wahrnehmung und Action-Timer sind getrennt.
- „Warum macht mehr Health ihn schneller?“ HealthLevel fließt gleichgewichtet in Alpha ein; diese Kopplung ist eine dokumentierte Grenze.
- „Wie tune ich mehr HP ohne andere Level zu ändern?“ HealthPerLevel oder BaseHealth bei gleichen Levels ändern und erneut anwenden; die Intelligenzableitung verwendet die Level, nicht diese Koeffizienten.
- „Gibt es bereits Kurven oder automatische Anpassung?“ Nein; aktuell Parameter, lineare Formeln und externe Level. Beides ist ein möglicher Ausblick.

Die Präsentation und PDFs werden als Renderings geprüft, nicht als durchgeführte Desktop-PowerPoint- oder Sprechprobe ausgegeben.
''')
    (ROOT/'dossier_pages.json').write_text(json.dumps(pages,ensure_ascii=False,indent=2),encoding='utf-8')
    (ROOT/'EnemyAI_Hintergrund_und_Quellen.md').write_text('\n\n'.join('# '+p['title']+'\n\n'+p['content'] for p in pages)+'\n',encoding='utf-8')

if __name__=='__main__':build()
