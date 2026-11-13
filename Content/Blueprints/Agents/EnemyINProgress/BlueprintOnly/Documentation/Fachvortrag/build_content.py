"""Content sources for the German presentation and its evidence dossier."""
from pathlib import Path
import json

ROOT = Path(__file__).parent
SOURCES = [
 dict(id='Q1', short='Hunicke, LeBlanc & Zubek (2004)', title='MDA: A Formal Approach to Game Design and Game Research',
      url='https://www.cs.northwestern.edu/~hunicke/MDA.pdf',
      detail='AAAI Workshop on Challenges in Game AI. Originalbeitrag, 5 PDF-Seiten; insbesondere PDF-S. 2 (MDA) und 3 (Dynamiken). Metadaten: https://aaai.org/papers/ws04-04-001-mda-a-formal-approach-to-game-design-and-game-research/',
      finding='MDA unterscheidet Spielregeln, daraus entstehendes Laufzeitverhalten und angestrebte emotionale Spielerreaktionen.',
      transfer='Wir analysieren die Kette: begrenzte Taser-Schüsse → Abstand und Timing abwägen → angestrebte taktische Spannung. Die Wirkung ist eine Projekthypothese.',
      limit='Ein Analysemodell; kein experimenteller Nachweis für die Qualität dieses Enemys.'),
 dict(id='Q2', short='Butcher & Griesemer (2002)', title='Creating the Illusion of Intelligence: Where AI and Level Design Overlap in Halo’s AI',
      url='https://halo.bungie.org/misc/gdc.2002.haloai/talk.html?page=25',
      detail='GDC 2002; archivierte Originalvortragsnotizen, Zusammenfassung auf Seite 25. Sessionnachweis: https://gdcvault.com/play/1022590/Creating-the-Illusion-of-Intelligence',
      finding='Die Vortragsnotizen betonen verständliches, interaktives Gegnerverhalten und erkennbare Denkprozesse.',
      transfer='Verlust des Sichtkontakts soll Suche auslösen; nachvollziehbare Grenzen werden im Projekt durch Zustände und Ressourcen umgesetzt.',
      limit='Praxisbericht zu Halo, keine kontrollierte Studie und keine Übertragung der Halo-Implementierung. Die Webseite hat eine irreführende Audio-Kopfzeile; zitiert sind die AI-Notizen.'),
 dict(id='Q3', short='Martel (2017)', title='Can You See Me Now? Building Robust AI Sensory Systems',
      url='https://www.gdcvault.com/play/1026497/Can-You-See-Me-Now',
      detail='Eric Martel, Ubisoft, GDC 2017 AI Summit. Verwendet wurde die öffentlich lesbare Sessionbeschreibung, nicht eine vollständige Videoanalyse.',
      finding='Die Sessionbeschreibung begründet glaubwürdige Detektion mit robusten, für Designer konfigurierbaren Sensorsystemen.',
      transfer='Sichtkegel, Distanz und Sichtprüfung werden getrennt von der zeitbasierten Näheerkennung eingestellt.',
      limit='Begründet das Designprinzip; liefert keine Bestätigung für unsere 65 Grad oder 2,5 Sekunden und belegt kein bei uns implementiertes Hören.'),
 dict(id='Q4', short='Ryan, Rigby & Przybylski (2006)', title='The Motivational Pull of Video Games: A Self-Determination Theory Approach',
      url='https://selfdeterminationtheory.org/SDT/documents/2006_RyanRigbyPrzybylski_MandE.pdf',
      detail='Motivation and Emotion, 30(4), 347–363. DOI: 10.1007/s11031-006-9051-8. Vier Studien; Abstract PDF-S. 1, Diskussion/Grenzen PDF-S. 14–16.',
      finding='Erlebte Kompetenz und Autonomie stehen in den untersuchten Spielen mit Spielvergnügen in Zusammenhang.',
      transfer='Für unser Projekt folgt daraus die prüfbare Annahme, dass erkennbare Handlungsmöglichkeiten und bewältigbare Gegenwehr relevant sind.',
      limit='Kein Beleg für einen optimalen Schwierigkeitswert und kein Nachweis für unsere Zielgruppe. Genre- und Messgrenzen werden von den Autoren diskutiert.'),
 dict(id='Q5', short='Epic Games: Animation Montage (UE 5.5)', title='Animation Montage',
      url='https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-montage-in-unreal-engine?application_version=5.5',
      detail='Offizielle technische Dokumentation; Abschnitte zu Blueprint-Steuerung, Slots und Slot Groups. Ohne ausgewiesenes Veröffentlichungsdatum; Versionsauswahl 5.5.',
      finding='Montages lassen sich per Blueprint steuern und über Slots in die Animation einbinden.',
      transfer='ABP_Enemy verwendet den Referenz-Slotaufbau; TryAttack wartet auf die Dauer der ausgewählten Punch-Montage.',
      limit='Technische Referenz; belegt nicht automatisch korrektes Rigging, gutes Timing oder Multiplayer-Kompatibilität unseres Projekts.'),
 dict(id='Q6', short='Epic Games: Navigation System (UE 5.5)', title='Navigation System',
      url='https://dev.epicgames.com/documentation/en-us/unreal-engine/navigation-system-in-unreal-engine?application_version=5.5',
      detail='Offizielle technische Übersicht; Navigation Mesh und Generation Modes. Ohne ausgewiesenes Veröffentlichungsdatum; Versionsauswahl 5.5.',
      finding='Unreal nutzt ein aus Levelkollision erzeugtes Navigation Mesh zur Wegfindung und bietet verschiedene Erzeugungsmodi.',
      transfer='Patrouille und Spawner benötigen erreichbare NavMesh-Punkte; prozedurale Räume werden erst nach MapComplete verarbeitet.',
      limit='MapComplete ersetzt keinen erfolgreichen Navigationstest; die Quelle belegt keine vollständige Erreichbarkeit aller generierten Räume.')
]

from focus_slides import SLIDES

EVIDENCE = [
 dict(id='E1', title='Blueprint-Implementierung und interner Funktionsnachweis', path='../EnemyInternals.md; ../../Authoring/generate_blueprints.py; ../../Authoring/update_intelligence.py; ../GraphInventory.json', description='Dokumentation und Authoring-Quelle inklusive installierter Animationskorrektur. 23 Enemy-Funktionen und 8 unterstützende Funktionen. Parameter stammen aus dieser Implementierung, nicht aus externer Literatur.'),
 dict(id='E2', title='Blueprint-Gameplayprüfung', path='../../Verification.txt', description='Stand 12.11.2026: 55 PASS-Zeilen; RESULT failures=0. Enthält auch Kompilier-/Elternklassenprüfungen. Keine 55 unabhängigen Spielerexperimente.'),
 dict(id='E3', title='Animierte Posen und Referenzsetup', path='../../AnimationPoseVerification.txt', description='Stand 12.11.2026: 27 PASS-Zeilen; RESULT failures=0. Vergleicht Referenzsetup und ausgewertete Posen mit BP_Enemy_punch; kein vollständiger visueller Playtest.'),
 dict(id='E4', title='Arena-Smoke-Test', path='../../Authoring/intelligence_arena.log', description='Protokoll vom 12.11.2026: BLUEPRINT_ONLY_SMOKE enemies=3 controllers=3 moving=1. Ein bewegter Gegner am Messzeitpunkt, keine Aussage über dauerhaftes Verhalten aller Gegner.')
]
EVIDENCE.append({'id': 'E5', 'title': 'Alt-Neu-Vergleich: Punch-Gegner und Blueprint-Gegner', 'path': 'Alt_Neu_Vergleich.md', 'description': 'Read-only-Graphinspektion am 13.11.2026: BP_Enemy_punch, zugewiesener AIC_Enemy_Base, BTT_DefaultAttack, BTT_MoveAlongRoute und BTT_FindRandomPatrolPoint. Belegt Struktur und sichtbare Aufrufe, keine Performance- oder Spielerlebnisüberlegenheit.'})

def write():
    ROOT.mkdir(exist_ok=True, parents=True)
    (ROOT/'content.json').write_text(json.dumps(dict(slides=SLIDES,sources=SOURCES,evidence=EVIDENCE),ensure_ascii=False,indent=2),encoding='utf-8')
    lines=['# Folienlayout und Sprechtext: Enemy AI: Sensory Setup and Difficulty scaling','',
      'Vorlage für **eine Person, 8:00 Minuten**. Alle Texte sind deutsch. Folien 11–12 sind Quellenanhang ohne geplante Sprechzeit. Nicht als fertige Team-Zeitaufteilung verwenden.','',
      '## Vor Abgabe zu ergänzen','',
      '- Name, Matrikelnummer, Modul und Fachdozent:in auf der ersten Seite.','- Freigegebene Persona, verbindliche Production-Ziele und persönlicher Eigenanteil.','- Prüfungsdatum und tatsächliche Uploadfrist (am Vortag).','- Bei mehreren Vortragenden: pro Person einen eigenen 5–10-Minuten-Block mit gleichem Umfang planen; diesen 8-Minuten-Beitrag nicht einfach aufteilen.','',
      'Die Aufgabenstellung nennt sowohl ±10 % als auch 4–11 Minuten. Rechnerisch ergeben sich aus 5 Minuten minus 10 % 4:30 Minuten. Der Entwurf bleibt mit 8:00 sicher innerhalb aller genannten Grenzen; die Abweichung sollte mit der Lehrperson geklärt werden.','',
      '## Layoutsystem','',
      '16:9, 1280 × 720 px; Arial; dunkler Hintergrund (#112326), warmweiß (#F4F1E8), Mint (#A9DBC8). Titel mindestens 32 pt, Haupttext etwa 23–27 pt. Flache typografische Kompositionen, viel Freiraum, keine Screenshots langer Blueprint-Graphen. Tabellen bleiben in PowerPoint editierbar.','',
      'Deckblatt mit dem gewählten Titel. Die zweite Folie enthält ausschließlich Themen und Reihenfolge. Danach folgen Alt-Neu-Vergleich, Sensorik, ihre Vor-/Nachteile und Designersteuerung, Difficulty-Berechnung, Ansatzbegründung und Designerzugang, ihre Vor-/Nachteile sowie ein Ausblick auf beide Systeme. Genauere Funktionen, Beispiele und Trade-offs stehen in den Sprechernotizen. Implementierte Einstellungen und geplante Werkzeuge bleiben klar getrennt.','']
    elapsed=0
    for i,s in enumerate(SLIDES,1):
        start=f'{elapsed//60}:{elapsed%60:02d}';elapsed+=s['seconds'];end=f'{elapsed//60}:{elapsed%60:02d}'
        lines += [f'## Folie {i}: {s["title"]}', '', f'**Zeit:** {start}–{end} ({s["seconds"]} s)' if s['seconds'] else '**Zeit:** Quellenanhang, nicht mitsprechen.', '', '**Sichtbarer Folientext**','']
        if s.get('subtitle'): lines += [s['subtitle'],'']
        lines += ['- '+b for b in s['body']]
        if s.get('table'):
            lines += ['', '| '+' | '.join(s['table'][0])+' |','| '+' | '.join(['---']*len(s['table'][0]))+' |']+['| '+' | '.join(r)+' |' for r in s['table'][1:]]
        if s['kind']=='sources': lines += ['- '+next((q['short']+': '+q['title'] for q in SOURCES if q['id']==rid),rid) for rid in s['refs']]
        lines += ['', '**Sprechtext**', '',s['notes'],'', '**Nachweise:** '+', '.join(s['refs']) if s['refs'] else '**Nachweise:** Projekt-/Deckblattangaben.','']
    lines += ['## Probenplan und faire Zeitverteilung','',
      'Die angegebenen Zeiten sind ein Regieplan, keine gemessene Redezeit. Die Sprechtexte lassen bewusst Raum für Zeigen, Pausen und Übergänge. Zweimal laut mit Stoppuhr proben; bei Bedarf Beispiele ausführen oder kürzen. Als Ziel 7:30–8:30 Minuten halten.','',
      '| Person | Inhalt | Zielzeit | Probe 1 | Probe 2 |','| --- | --- | --- | --- | --- |','| [Person A] | Enemy-Beitrag dieses Entwurfs | 8:00 | [messen] | [messen] |','| [Person B, falls vorhanden] | eigener Department-Beitrag | ebenfalls 8:00 | [messen] | [messen] |','',
      'Wenn ausschließlich das Enemy-Thema auf mehrere Personen verteilt werden soll, müssen zusätzliche eigenständige Kapitel für je acht Minuten entstehen. Gleich viel Zeit ist wichtiger als gleich viele Folien. Wer welchen Teil implementiert hat, vor dem Vortrag offen und korrekt zuordnen.','',
      'Beim Einstieg Blickkontakt halten und die Spielsituation mit einer kurzen Pause wirken lassen. Bei Tabellen nur den relevanten Vergleich zeigen. Fachwörter wie Zustandsautomat und Windup einmal knapp erklären. Keine vollständigen Funktionen ablesen.','',
      '## Demonstration ohne Zeitrisiko','',
      'Optional einen 15–20-Sekunden-Clip aus dem echten Projekt in die 65 Sekunden der Wahrnehmungsfolie integrieren: entdecken → Ecke → letzter Ort → Suche. Kein Clip ist in dieser Vorlage enthalten. Vorher aufnehmen und gegen einen Standbild-/Textablauf absichern; kein unvorbereitetes Live-Demo-Risiko. Der Ablauf bleibt auch ohne Clip vollständig vortragbar.']
    (ROOT/'Folienlayout_und_Sprechtext.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')

if __name__=='__main__': write()
