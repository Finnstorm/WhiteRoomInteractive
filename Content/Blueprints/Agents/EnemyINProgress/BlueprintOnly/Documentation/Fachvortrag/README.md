# Enemy AI: Sensory Setup and Difficulty scaling

Deutschsprachiger Entwurf V3 vom 13.11.2026 mit dem gewählten englischen Titel. Der Inhalt ist auf **eine Person mit acht Minuten** ausgelegt: Deckblatt, reine Themenagenda, Alt-Neu-Vergleich, Sensorik mit Vor-/Nachteilen und Designerzugang, Difficulty Scaling mit Berechnung, Begründung, Designerzugang und Vor-/Nachteilen sowie Ausbauoptionen beider Systeme. Es gibt zehn vorgetragene Folien und zwei Quellenfolien im Anhang. Die Sprechernotizen vertiefen das Warum und Wie.

## Arbeitsdateien und PDFs

- [Folien V3 als PDF](Ausgabe/VORLAGE_Matrikelnummer_EnemyAI_Sensory_Difficulty_V3.pdf)
- [Bearbeitbare PowerPoint V3 mit Sprechernotizen](Ausgabe/EnemyAI_Sensory_Difficulty_V3_bearbeitbar.pptx)
- [Begleitdossier V3 als PDF](Ausgabe/VORLAGE_Matrikelnummer_EnemyAI_Sensory_Difficulty_Begleitdossier_V3.pdf): zwölf Seiten mit Mechanik, Formeln, Designerparametern, Abwägungen und Quellenkritik.
- [Vergleich mit dem ursprünglichen Punch-Gegner](Alt_Neu_Vergleich.md): überprüfte Gemeinsamkeiten, Unterschiede und Aussagegrenzen.
- [Folienlayout und Sprechtext](Folienlayout_und_Sprechtext.md): sichtbarer Text, Zeitfenster, Sprechtext und Quellen pro Folie sowie Probenplan.
- [Bearbeitbares Begleitdossier](EnemyAI_Hintergrund_und_Quellen.md)
- [Erweiterte Funktionsdokumentation](../EnemyInternals.md): ergänzt um wissenschaftliche/praktische Einordnung und Quellen.

## Noch zu ergänzen

Name, Matrikelnummer, Modul, Fachdozent:in, bestätigte Persona, verbindliche Production-Ziele und individueller Eigenanteil. Die Teamgröße wurde nicht angegeben; bei mehreren Personen erhält jede einen eigenen gleich langen Beitrag. Die acht Minuten nicht einfach aufteilen.

Die Namen mit `VORLAGE` erfüllen noch nicht die geforderte individuelle Dateibenennung. Vor Upload Pflichtangaben eintragen und nach `Matrikelnummer_NameDerAbgabe.pdf` umbenennen. Prüfungsdatum und Uploadfrist am Vortag festlegen; nichts wurde hochgeladen.

## Quellen und Aussagegrenzen

Für V3 ausgewählt sind MDA, Halo-Entwicklervortragsnotizen, die öffentliche GDC-Sensorikbeschreibung und Motivationsforschung (Q1–Q4). Die früher recherchierten Epic-Quellen Q5–Q6 bleiben in der Inhaltsbasis erhalten. Projektnachweise sind als E1–E5 markiert; E5 ist die Graphinspektion des ursprünglichen Punch-Gegners. Die Quellenkritik nennt jeweils Fundstelle, zulässige Übertragung und Grenzen. Der Designerzugang meint editierbare Blueprint-Parameter und Funktionen; ein eigenes Preset- oder Debug-Tool existiert noch nicht. Wahrnehmungsbedingungen und die anschließende Entscheidungsgeschwindigkeit werden getrennt erklärt.

Technische PASS-Zahlen werden nicht als Nutzerstudie oder Beweis für Spielspaß dargestellt. Die Persona-Passung ist offen. Keine neuen Gameplayänderungen gehören zu diesem Dokumentationsauftrag.

## Bearbeitung

Folieninhalte und Sprechtexte werden in `focus_slides.py` gepflegt; `build_content.py` erzeugt daraus `content.json` und den Markdown-Regieplan. `focus_dossier.py` enthält die V3-Dossierinhalte, die über `build_dossier.py` erzeugt werden; `build_pdf.py` setzt sie als PDF. `build_deck.mjs` erzeugt die nativen PowerPoint-Objekte und Folienvorschauen mit Artifact Tool. Der Deck-PDF-Export verwendet die hochauflösenden gerenderten Folien; Inhalte werden in der PPTX bearbeitet. Die Ausgaben V1 und V2 bleiben erhalten. `revise_slides_v2.py` ist ein historischer Einmalschritt und gehört nicht zum V3-Build.

Die PPTX wurde technisch validiert und alle Folien/Dossierseiten wurden als Renderings geprüft. Sie wurde nicht in der Desktop-Anwendung PowerPoint geöffnet. Vor der Abgabe Sprechzeit, Zielgerät und aktualisierten PDF-Export prüfen.
