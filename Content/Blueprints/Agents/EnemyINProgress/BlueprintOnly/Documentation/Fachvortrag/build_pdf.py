"""Typeset the German source/evidence dossier and create page previews."""
import json,re,html
from pathlib import Path
from reportlab.platypus import SimpleDocTemplate,Paragraph,Spacer,Table,TableStyle,PageBreak,KeepTogether
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet,ParagraphStyle
from reportlab.lib.enums import TA_LEFT
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas

ROOT=Path(__file__).parent
OUT=ROOT/'Ausgabe';OUT.mkdir(exist_ok=True)
pdfmetrics.registerFont(TTFont('Arial','C:/Windows/Fonts/arial.ttf'))
pdfmetrics.registerFont(TTFont('Arial-Bold','C:/Windows/Fonts/arialbd.ttf'))
pdfmetrics.registerFontFamily('Arial',normal='Arial',bold='Arial-Bold',italic='Arial',boldItalic='Arial-Bold')
ink=colors.HexColor('#112326');muted=colors.HexColor('#4E625E');mint=colors.HexColor('#DCEEE6')
style=getSampleStyleSheet()
style.add(ParagraphStyle(name='BodyDE',fontName='Arial',fontSize=10.3,leading=14.2,spaceAfter=8,textColor=ink,wordWrap='LTR'))
style.add(ParagraphStyle(name='SmallDE',parent=style['BodyDE'],fontSize=8.9,leading=12,spaceAfter=4))
style.add(ParagraphStyle(name='TitleDE',fontName='Arial-Bold',fontSize=25,leading=30,spaceAfter=22,textColor=ink))
style.add(ParagraphStyle(name='H2DE',fontName='Arial-Bold',fontSize=13,leading=17,spaceBefore=9,spaceAfter=7,textColor=ink,keepWithNext=True))

def mark(s):
    s=html.escape(s)
    s=re.sub(r'\*\*(.*?)\*\*',r'<b>\1</b>',s)
    s=re.sub(r'(https?://[^\s<>]+)',lambda m:'<link href="'+html.unescape(m[1])+'" color="#267B65">'+m[1]+'</link>',s)
    return s

def table(lines):
    rows=[[x.strip() for x in l.strip().strip('|').split('|')] for l in lines if not re.match(r'^\|[\s:|-]+\|$',l)]
    cols=len(rows[0]);widths=[(481/cols)]*cols
    if cols==3: widths=[116,182,183]
    if cols==2: widths=[165,316]
    content=[[Paragraph(mark(c),style['SmallDE']) for c in row] for row in rows]
    t=Table(content,colWidths=widths,repeatRows=1,hAlign='LEFT')
    t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),mint),('FONTNAME',(0,0),(-1,0),'Arial-Bold'),('VALIGN',(0,0),(-1,-1),'TOP'),('TOPPADDING',(0,0),(-1,-1),7),('BOTTOMPADDING',(0,0),(-1,-1),7),('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),('LINEBELOW',(0,0),(-1,0),.7,muted),('LINEBELOW',(0,1),(-1,-1),.25,colors.HexColor('#D8DFDA'))]))
    return t

def footer(canvas,doc):
    canvas.setFillColor(muted);canvas.setFont('Arial',8)
    canvas.drawString(57,32,'WHITE ROOM INTERACTIVE  /  BLUEPRINT ENEMY AI  /  13.11.2026')
    canvas.drawRightString(538,32,str(doc.page))

pages=json.loads((ROOT/'dossier_pages.json').read_text(encoding='utf-8'))
story=[]
for i,page in enumerate(pages):
    if i: story.append(PageBreak())
    story.append(Paragraph(mark(page['title']),style['TitleDE']))
    normalized=re.sub(r'(?m)^(## [^\n]+)\n(?!\n)',r'\1\n\n',page['content'])
    blocks=normalized.split('\n\n')
    body_style=style['SmallDE'] if any(word in page['title'] for word in ('Quellenkritik','Projektnachweise')) else style['BodyDE']
    for b in blocks:
        if not b.strip():continue
        if b.startswith('## '):story.append(Paragraph(mark(b[3:]),style['H2DE']))
        elif b.startswith('|'):story.extend([table(b.splitlines()),Spacer(1,9)])
        else:
            for line in b.splitlines():
                story.append(Paragraph(mark(line),body_style))
    if i==0:story.append(Spacer(1,22))
destination=OUT/'VORLAGE_Matrikelnummer_EnemyAI_Sensory_Difficulty_Begleitdossier_V3.pdf'
doc=SimpleDocTemplate(str(destination),pagesize=(595.28,841.89),rightMargin=57,leftMargin=57,topMargin=54,bottomMargin=56,title='Enemy AI: Sensory Setup and Difficulty scaling - Begleitdossier',author='Vorlage - Angaben ergänzen')
doc.build(story,onFirstPage=footer,onLaterPages=footer)
print(destination)

# Use the slide renderer's high-resolution output for an identical PDF layout.
# The native PPTX remains editable; no bundled rendering dependency is modified.
previews=sorted((ROOT/'.build').glob('slide-*.png'))
if len(previews)==12:
    deck=OUT/'VORLAGE_Matrikelnummer_EnemyAI_Sensory_Difficulty_V3.pdf'
    c=canvas.Canvas(str(deck),pagesize=(960,540))
    c.setTitle('Enemy AI: Sensory Setup and Difficulty scaling');c.setAuthor('Vorlage - Angaben ergänzen')
    for preview in previews:
        c.drawImage(str(preview),0,0,width=960,height=540)
        c.showPage()
    c.save(); print(deck)
