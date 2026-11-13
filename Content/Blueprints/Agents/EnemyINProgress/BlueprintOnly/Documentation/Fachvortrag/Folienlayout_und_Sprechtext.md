# Folienlayout und Sprechtext: Enemy AI: Sensory Setup and Difficulty scaling

Vorlage für **eine Person, 8:00 Minuten**. Alle Texte sind deutsch. Folien 11–12 sind Quellenanhang ohne geplante Sprechzeit. Nicht als fertige Team-Zeitaufteilung verwenden.

## Vor Abgabe zu ergänzen

- Name, Matrikelnummer, Modul und Fachdozent:in auf der ersten Seite.
- Freigegebene Persona, verbindliche Production-Ziele und persönlicher Eigenanteil.
- Prüfungsdatum und tatsächliche Uploadfrist (am Vortag).
- Bei mehreren Vortragenden: pro Person einen eigenen 5–10-Minuten-Block mit gleichem Umfang planen; diesen 8-Minuten-Beitrag nicht einfach aufteilen.

Die Aufgabenstellung nennt sowohl ±10 % als auch 4–11 Minuten. Rechnerisch ergeben sich aus 5 Minuten minus 10 % 4:30 Minuten. Der Entwurf bleibt mit 8:00 sicher innerhalb aller genannten Grenzen; die Abweichung sollte mit der Lehrperson geklärt werden.

## Layoutsystem

16:9, 1280 × 720 px; Arial; dunkler Hintergrund (#112326), warmweiß (#F4F1E8), Mint (#A9DBC8). Titel mindestens 32 pt, Haupttext etwa 23–27 pt. Flache typografische Kompositionen, viel Freiraum, keine Screenshots langer Blueprint-Graphen. Tabellen bleiben in PowerPoint editierbar.

Deckblatt mit dem gewählten Titel. Die zweite Folie enthält ausschließlich Themen und Reihenfolge. Danach folgen Alt-Neu-Vergleich, Sensorik, ihre Vor-/Nachteile und Designersteuerung, Difficulty-Berechnung, Ansatzbegründung und Designerzugang, ihre Vor-/Nachteile sowie ein Ausblick auf beide Systeme. Genauere Funktionen, Beispiele und Trade-offs stehen in den Sprechernotizen. Implementierte Einstellungen und geplante Werkzeuge bleiben klar getrennt.

## Folie 1: Enemy AI: Sensory Setup and Difficulty scaling

**Zeit:** 0:00–0:15 (15 s)

**Sichtbarer Folientext**

Sensory Setup and
Difficulty scaling

- Blueprint-Version · WhiteRoomInteractive
- [Name] · Matrikelnummer: [eintragen]
- Modul: [eintragen] · Fachdozent:in: [eintragen]

**Sprechtext**

Mein Teil heißt Enemy AI: Sensory Setup and Difficulty scaling. Ich zeige, wie der Gegner Informationen über den Spieler erhält und wie sein Verhalten an den Fortschritt angepasst wird. Dabei geht es besonders um nachvollziehbare Regeln und die Stellschrauben für das Design Department. [Persönlichen Eigenanteil und Deckblattangaben ergänzen.]

**Nachweise:** Projekt-/Deckblattangaben.

## Folie 2: Themen und Reihenfolge

**Zeit:** 0:15–0:35 (20 s)

**Sichtbarer Folientext**

- 01  Alter und neuer Gegner
- 02  Sensory Setup: Funktionsweise
- 03  Sensorik: Vor- und Nachteile, Designerzugang
- 04  Difficulty Scaling: Funktionsweise
- 05  Scaling: Begründung, Designerzugang, Vor- und Nachteile
- 06  Ausbau beider Systeme

**Sprechtext**

Ich beginne mit dem Vergleich zur bisherigen Enemy AI. Danach erkläre ich das Sensory Setup, seine Vor- und Nachteile und die Einstellmöglichkeiten für Designer. Anschließend folgt die Difficulty-Skalierung: ihre Berechnung, die Begründung und der Designerzugang. Zum Schluss zeige ich mögliche Erweiterungen in beiden Bereichen.

**Nachweise:** Projekt-/Deckblattangaben.

## Folie 3: Comparison: Old enemy / New enemy

**Zeit:** 0:35–1:25 (50 s)

**Sichtbarer Folientext**

- BP_Enemy_punch → BlueprintOnly/BP_Enemy
- Patrouille und Reaktion auf Sichtverlust gab es bereits.

| Bereich | Alter Punch-Gegner | Neuer Blueprint-Gegner |
| --- | --- | --- |
| Wahrnehmung | AIPerception + Controller | Sicht, Nähezeit und Schaden |
| Sichtverlust | Lost Sight → Seeking | Gespeicherter Ort → Search |
| Balancing | Feste Bewegungsstufen | Vier Level + Entscheidungstempo |

**Sprechtext**

Alt bezeichnet hier BP_Enemy_punch mit seinem zugewiesenen AIC_Enemy_Base, nicht unseren C++-Prototyp. Der alte Controller verwendet AIPerception, Behavior Tree und Blackboard und hat bereits Handler für Sicht, Schaden und Sichtverlust. Außerdem gibt es Patrouillen-Tasks und einen Sound-Handler; dessen vollständiges Laufzeitverhalten wurde nicht neu geprüft. Wir haben Wahrnehmung und Suche also nicht erstmals erfunden. Im neuen Blueprint-Gegner sind die Sichtprüfung, die kontinuierliche Nähezeit und die Schadensreaktion explizit in Funktionen beschrieben. Der zuletzt bekannte Ort steuert die Suche. Für die Progression kommen vier externe Level und daraus abgeleitete Entscheidungsintervalle hinzu. Die Behauptung ist eine andere Aufteilung mit erweitertem Tuning, nicht eine gemessene Überlegenheit oder eine grundsätzliche Abwertung von Behavior Trees.

**Nachweise:** E5, E1

## Folie 4: Sensory Setup: Wie erkennt er den Spieler?

**Zeit:** 1:25–2:30 (65 s)

**Sichtbarer Folientext**

- Sicht
- 18 m · 65° Halbwinkel
- Distanz + Blickkegel + Sichtlinie
- Nähe
- 2,5 s
- Durchgehend in 2,5 m, freie Sicht
- Schaden
- Sofort
- Treffer einem Spieler zugeordnet
- Kontakt verloren → letzter bekannter Ort → Suche

**Sprechtext**

Das neue Setup hat drei Wege zur Aufmerksamkeit. Erstens die Sicht: UpdateSenses kombiniert Entfernung, Blickkegel und ClearLineTo. Alle drei Bedingungen müssen passen. Standardmäßig sind das 18 Meter Sichtweite und ein Halbwinkel von 65 Grad. Sichtbarkeit alarmiert ohne zusätzlichen Verdachtstimer. Zweitens die Nähe: Sie funktioniert auch außerhalb des Blickkegels, verlangt aber eine freie Sichtlinie und zweieinhalb Sekunden ununterbrochene Nähe innerhalb von zweieinhalb Metern. Wird die Bedingung unterbrochen, beginnt der Timer wieder bei null. Drittens die Schadensreaktion: NotifyPlayerDamage übernimmt einen zugeordneten, playerkontrollierten Character als Ziel und alarmiert sofort. Bei aktueller Wahrnehmung wird LastKnownLocation gespeichert. Nach verlorenem Kontakt verfolgt der Gegner diesen Punkt und wechselt unter den Ankunfts- und Zeitbedingungen in Search. Das ist begrenztes, regelbasiertes Wissen; ein neuer Treffer oder erneute Wahrnehmung darf den Ort wieder aktualisieren. Die Zahlen sind Projektdefaults, keine wissenschaftlich optimalen Werte.

**Nachweise:** Q2, Q3, E1

## Folie 5: Sensorik: Vor- und Nachteile

**Zeit:** 2:30–3:10 (40 s)

**Sichtbarer Folientext**

- Klare Auslöser
- Sinne getrennt einstellbar
- Binäre Sicht-Erkennung
- Keine Hörsensorik im neuen Setup
- Fairness und Lesbarkeit müssen im Playtest geprüft werden.

**Sprechtext**

Der Vorteil liegt in den klaren Auslösern: Ich kann erklären und testen, welche Kombination zur Entdeckung führt. Sicht und Nähe sind getrennt einstellbar. Das gibt dem Designer Kontrolle über unterschiedliche Situationen, etwa einen Spieler vor oder hinter dem Gegner. Die Grenzen sind ebenfalls konkret: Die Sichtentscheidung ist binär. Sie besitzt noch keinen ansteigenden Verdachtswert; beim Überschreiten der Grenze kann die Reaktion abrupt wirken. Im neuen Setup fehlen außerdem Geräuschereignisse. Der alte Sound-Handler darf deshalb nicht als hier bereits übernommene Funktion präsentiert werden. Die technische Nachvollziehbarkeit unterstützt das Balancing, beweist aber noch keine wahrgenommene Fairness.

**Nachweise:** Q3, E1

## Folie 6: Sensorik: Zugriff für das Design Department

**Zeit:** 3:10–4:05 (55 s)

**Sichtbarer Folientext**

- Class Defaults oder Instanz → Details
- Wahrnehmen und Handeln haben getrennte Zeitbedingungen.

| Einstellen | Variable(n) | Wirkung |
| --- | --- | --- |
| Sichtbereich | SightRadius / SightHalfAngle | Reichweite und Winkel |
| Nähealarm | ProximitySeconds | Benötigte Nähezeit |
| Suchwechsel | SightMemory | Erinnerungsbedingung |
| Suchdauer | SearchDuration | Länge der Suchphase |

**Sprechtext**

Für das Balancing muss das Design Department den Graphen nicht für jeden Zahlenwert umbauen. Die Variablen sind in Class Defaults und auf Instanzen editierbar. SightRadius und SightHalfAngle bestimmen den Sichtbereich; ProximityRadius und ProximitySeconds bestimmen Distanz und Dauer der Näheerkennung. SightMemory beteiligt sich am Übergang zur Suche, während SearchDuration die anschließende Suchphase begrenzt. Ein Beispiel: Eine längere ProximitySeconds lässt dem Spieler außerhalb des Blickkegels länger Zeit; sie verzögert nicht automatisch die Sichtdetektion. Ebenso wichtig ist die Trennung zum Handeln: Nach dem Wechsel nach Chase gilt eine DecisionTime, deren Intervall aus der Difficulty abgeleitet wird. Ein Designer sollte also wissen, ob er Entdeckung, Suche oder die anschließende Entscheidung verändern möchte. Der Zugang ist derzeit eine editierbare Variablenoberfläche, kein eigenes Designer-Tool mit fertigen Presets oder Sichtkegel-Gizmos. Werte einzeln ändern und dieselbe Testsituation wiederholen.

**Nachweise:** E1

## Folie 7: Difficulty Scaling: Wie funktioniert es?

**Zeit:** 4:05–5:15 (70 s)

**Sichtbarer Folientext**

- Vier Integer-Level: Damage · Block · Health · Movement
- Mittelwert der Level → Entscheidungspause von 0,35 bis 0,10 s

| Bereich | Basis | Zuwachs je Level |
| --- | --- | --- |
| Damage | 12 / 25 / 8 Schaden | +15 % des Basiswerts |
| Block | 3 Treffer | +1 Treffer Kapazität |
| Health | 100 HP | +20 HP |
| Movement | 160 / 360 cm/s | +8 % der Basis, max. 750 |

**Sprechtext**

Von außen kommen vier Integer-Level. SetDifficultyLevels begrenzt sie zunächst auf null bis zehntausend. Die Tabelle zeigt die Standardkoeffizienten: Schaden ist Basisschaden mal eins plus 0,15 mal DamageLevel. Blockkapazität ist drei plus BlockLevel. Maximale Gesundheit ist hundert plus zwanzig mal HealthLevel. Bewegung wird vom Patrol- beziehungsweise Chase-Basiswert mit eins plus 0,08 mal MovementLevel skaliert und auf 750 Zentimeter pro Sekunde begrenzt. Zusätzlich bildet RefreshIntelligence den Mittelwert aller vier Level. Geteilt durch IntelligenceLevelForMaximum, standardmäßig zehn, ergibt sich ein Faktor zwischen null und eins. DecisionInterval ist 0,35 minus 0,25 mal diesen Faktor. Bei Durchschnitt null sind das 350 Millisekunden, bei zehn 100. Auch das Pfadupdate wird häufiger. Die Entscheidungspause gilt unter anderem beim Wechsel nach Chase; laufende Aktionen bleiben durch ihren Zustand und ihre Timer geschützt. Diese Heuristik lernt nicht und beobachtet keine Spielerleistung. Die maximalen Eingabegrenzen sind Schutzgrenzen, keine Empfehlung für ausgewogene Progression.

**Nachweise:** E1

## Folie 8: Warum dieser Ansatz – und wie wird er getunt?

**Zeit:** 5:15–6:10 (55 s)

**Sichtbarer Folientext**

- Eine Schnittstelle, getrennte Stellschrauben.
- SetDifficultyLevels → Gegner oder Spawner
- Basiswerte, Koeffizienten und IntelligenceLevelForMaximum editieren.

**Sprechtext**

Ich habe diesen Ansatz gewählt, weil die Progression von anderen Spielsystemen kommen soll und das Design Department einzelne Bereiche abstimmen können muss. Vier benannte Integer sind als Blueprint-Schnittstelle einfach zu übergeben. Baselines und Zuwächse bleiben direkt editierbar; eine neue Balance erfordert keinen neuen Angriffsgraphen. Für einzelne Gegner wird SetDifficultyLevels aufgerufen. Für eine Population bietet der Spawner dieselbe Schnittstelle mit UpdateExisting; damit erreicht eine Änderung vorhandene und zukünftige Gegner. Designer können etwa HealthPerLevel ändern, ohne DamagePerLevel zu verändern. IntelligenceLevelForMaximum bestimmt, bei welchem Durchschnitt das schnellste Entscheidungstempo erreicht wird. Die abgeleiteten Intervalle sollte man nicht als dauerhafte Konfiguration direkt überschreiben, weil RefreshIntelligence sie neu berechnet. Nach Änderungen die Difficulty erneut anwenden. Das ist ein zugänglicher Parameteransatz, aber noch kein spezialisiertes Preset-/Kurventool. Die erhoffte Wirkung auf Herausforderung ist weiterhin zu testen.

**Nachweise:** E1, Q1

## Folie 9: Difficulty Scaling: Vor- und Nachteile

**Zeit:** 6:10–6:55 (45 s)

**Sichtbarer Folientext**

- Planbar und reproduzierbar
- Live-Update erhält den Health-Anteil
- Health erhöht auch Entscheidungstempo
- Keine automatische Leistungsanpassung
- Stats und Reaktionsdruck im Balancing getrennt beobachten.

**Sprechtext**

Der erste Vorteil ist Reproduzierbarkeit: Gleiche Eingaben erzeugen dieselben berechneten Grundwerte und Intervalle. Die späteren zufälligen Aktionsentscheidungen sind damit nicht vollständig deterministisch. Außerdem bleibt beim Live-Skalieren der Gesundheitsanteil erhalten, statt den Gegner wieder vollzuheilen. Die getrennten Stat-Koeffizienten erleichtern gezielte Änderungen. Ein Nachteil ist die zusätzliche Kopplung durch den Mittelwert: Selbst ein reiner Health-Anstieg macht den Gegner auch schneller in seinen Entscheidungen. Das kann unbeabsichtigt zwei Schwierigkeitsfaktoren erhöhen. Außerdem ist das Modell überwiegend linear und passt sich nicht selbst an die Leistung eines Spielers an. Bei hohen Leveln kann es deshalb trotz mathematisch gültiger Werte unausgewogen werden. Genau deshalb sollten Stats und Reaktionsdruck getrennt beobachtet werden. Eine technische Testsuite bestätigt die Berechnung und einzelne Abläufe, nicht das ideale Balancing.

**Nachweise:** E1, E2

## Folie 10: Ausblick: Sensorik und Difficulty Scaling

**Zeit:** 6:55–8:00 (65 s)

**Sichtbarer Folientext**

- Geräusche als Ereignisse
- Verdacht statt binärer Sicht
- Sichtkegel und letzten Ort visualisieren
- Intelligenz als eigenen Faktor lösen
- Designer-Presets und Kurven
- Optionale Anpassung an Spielerleistung
- Ausbauoptionen – noch nicht umgesetzt

**Sprechtext**

Bei den Sinnen sehe ich drei konkrete Richtungen. Erstens Geräusche als Ereignisse, etwa Schritte oder einen Kampf, mit Position und Stärke. Zweitens einen Verdachtswert, der bei kurzen oder schwachen Sichtkontakten erst steigt und bei Verlust wieder sinkt. Drittens Debug-Ansichten für Sichtkegel, Nähebereich und letzten bekannten Ort, damit Designer ihre Änderungen direkt überprüfen können. Bei der Difficulty wäre der erste Schritt, Intelligenz von Gesundheit und anderen Stat-Leveln zu entkoppeln. Danach könnten benannte Presets und Kurven die bisher linearen Koeffizienten ergänzen, sodass das Design Department gezieltere Progressionsverläufe erstellt. Eine automatische Anpassung an Spielerleistung wäre eine weitere Möglichkeit, aber nur mit einem klaren Designziel, begrenzten Änderungen und Playtests; sie ist im aktuellen System nicht vorhanden. Ich würde zunächst die Designerübersicht und Entkopplung priorisieren und erst danach zusätzliche Adaptivität prüfen. Diese Optionen sind Ausblick, keine bereits gelieferten Features.

**Nachweise:** E1

## Folie 11: Quellen · Design und Wahrnehmung

**Zeit:** Quellenanhang, nicht mitsprechen.

**Sichtbarer Folientext**

- Hunicke, LeBlanc & Zubek (2004): MDA: A Formal Approach to Game Design and Game Research
- Butcher & Griesemer (2002): Creating the Illusion of Intelligence: Where AI and Level Design Overlap in Halo’s AI
- Martel (2017): Can You See Me Now? Building Robust AI Sensory Systems
- Ryan, Rigby & Przybylski (2006): The Motivational Pull of Video Games: A Self-Determination Theory Approach

**Sprechtext**

Quellenanhang ohne geplante Sprechzeit. Q4 ist Hintergrund für Kompetenz/Autonomie und keine Bestätigung unserer Persona oder Difficultywerte. Vollständige Nachweise und Grenzen stehen im Begleitdossier.

**Nachweise:** Q1, Q2, Q3, Q4

## Folie 12: Quellen · Implementierung und Vergleich

**Zeit:** Quellenanhang, nicht mitsprechen.

**Sichtbarer Folientext**

- E1
- E2
- E5

**Sprechtext**

Quellenanhang ohne geplante Sprechzeit. E5 begrenzt den Alt-Neu-Vergleich auf den ursprünglichen Punch-Gegner, seinen Controller und ausgewählte Tasks. Kein vergleichender Performance- oder Spielerlebnisversuch.

**Nachweise:** E1, E2, E5

## Probenplan und faire Zeitverteilung

Die angegebenen Zeiten sind ein Regieplan, keine gemessene Redezeit. Die Sprechtexte lassen bewusst Raum für Zeigen, Pausen und Übergänge. Zweimal laut mit Stoppuhr proben; bei Bedarf Beispiele ausführen oder kürzen. Als Ziel 7:30–8:30 Minuten halten.

| Person | Inhalt | Zielzeit | Probe 1 | Probe 2 |
| --- | --- | --- | --- | --- |
| [Person A] | Enemy-Beitrag dieses Entwurfs | 8:00 | [messen] | [messen] |
| [Person B, falls vorhanden] | eigener Department-Beitrag | ebenfalls 8:00 | [messen] | [messen] |

Wenn ausschließlich das Enemy-Thema auf mehrere Personen verteilt werden soll, müssen zusätzliche eigenständige Kapitel für je acht Minuten entstehen. Gleich viel Zeit ist wichtiger als gleich viele Folien. Wer welchen Teil implementiert hat, vor dem Vortrag offen und korrekt zuordnen.

Beim Einstieg Blickkontakt halten und die Spielsituation mit einer kurzen Pause wirken lassen. Bei Tabellen nur den relevanten Vergleich zeigen. Fachwörter wie Zustandsautomat und Windup einmal knapp erklären. Keine vollständigen Funktionen ablesen.

## Demonstration ohne Zeitrisiko

Optional einen 15–20-Sekunden-Clip aus dem echten Projekt in die 65 Sekunden der Wahrnehmungsfolie integrieren: entdecken → Ecke → letzter Ort → Suche. Kein Clip ist in dieser Vorlage enthalten. Vorher aufnehmen und gegen einen Standbild-/Textablauf absichern; kein unvorbereitetes Live-Demo-Risiko. Der Ablauf bleibt auch ohne Clip vollständig vortragbar.
