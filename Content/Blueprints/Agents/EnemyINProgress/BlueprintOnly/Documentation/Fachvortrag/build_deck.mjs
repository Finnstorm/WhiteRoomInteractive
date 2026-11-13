import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';

const root=path.dirname(fileURLToPath(import.meta.url));
const runtime='C:/Program Files/WindowsApps/OpenAI.CodexPrimaryRuntime.v26-1111-570-0_26.1111.570.0_x64__3k8sg7r9htsxt/dependencies';
process.env.RUNTIME_NODE_MODULES=path.join(runtime,'node/node_modules');
process.env.RUNTIME_NODE=path.join(runtime,'node/bin/node.exe');
const {Presentation,PresentationFile}=await import(pathToFileURL(path.join(runtime,'node/node_modules/@oai/artifact-tool/dist/artifact_tool.mjs')).href);
const skill='C:/Users/Kruse/.codex/plugins/cache/openai-primary-runtime/presentations/26.1111.10930/skills/presentations';
const data=JSON.parse(await fs.readFile(path.join(root,'content.json'),'utf8'));
const build=path.join(root,'.build'),out=path.join(root,'Ausgabe');
await fs.mkdir(build,{recursive:true});await fs.mkdir(out,{recursive:true});
const p=Presentation.create({slideSize:{width:1280,height:720}});
const C={bg:'#112326',paper:'#F4F1E8',mint:'#A9DBC8',muted:'#C2CEC8',ink:'#112326'};
function text(s,t,x,y,w,h,size=32,color=C.paper,bold=false){
 const sh=s.shapes.add({geometry:'textbox',name:`text-${s.shapes.items.length}`,position:{left:x,top:y,width:w,height:h},fill:'none',line:{fill:'none',width:0}});
 sh.text=t;sh.text.style={fontFamily:'Arial',fontSize:size,color,bold,verticalAlignment:'top',alignment:'left',wrap:'word',insets:0};
 return sh;
}
function table(s,values,x,y,w,h){
 const t=s.tables.add({rows:values.length,columns:values[0].length,left:x,top:y,width:w,height:h,values,columnWidths:[w*.20,w*.39,w*.41]});
 t.borders.assign({fill:C.bg,width:0});
 for(let r=0;r<values.length;r++) for(let c=0;c<values[r].length;c++){
   const cell=t.getCell(r,c);cell.fill=r===0?'#23423F':C.bg;
   cell.text.style={fontFamily:'Arial',fontSize:r===0?23:27,color:r===0?C.mint:C.paper,bold:r===0};
 }
 return t;
}
for(let i=0;i<data.slides.length;i++){
 const d=data.slides[i],s=p.slides.add();s.background.fill=C.bg;
 const refs=d.refs.join(' · ');
 text(s,`${String(i+1).padStart(2,'0')}  /  ENEMY AI`,72,36,500,30,18,C.muted);
 if(d.kind!=='cover') text(s,d.title,72,105,1136,112,d.kind==='closing'?53:47,C.paper,true);
 text(s,refs,72,665,1136,28,18,C.muted);
 if(d.kind==='cover'){
   text(s,'Enemy AI:',72,105,1100,125,96,C.paper,true);
   text(s,d.subtitle,76,255,1100,160,58,C.mint);
   text(s,d.body[0],78,455,1100,45,26,C.paper);
   text(s,d.body.slice(1).join('\n'),78,530,1100,85,23,C.muted);
   text(s,'VORLAGE · Pflichtangaben und Persona vor Abgabe ergänzen',78,635,1100,36,19,C.mint);
 }else if(d.kind==='agenda'){
   for(let j=0;j<d.body.length;j++)text(s,d.body[j],72,235+j*62,1136,66,31,j===0?C.mint:C.paper,false);
 }else if(d.kind==='sensory'){
   for(let j=0;j<3;j++){
     const x=72+j*390;
     text(s,d.body[j*3].toUpperCase(),x,245,350,40,23,C.mint,true);
     text(s,d.body[j*3+1],x,310,350,82,j===0?40:52,C.paper,true);
     text(s,d.body[j*3+2],x,420,350,110,28,C.paper);
   }
   text(s,d.body[9],72,580,1136,64,31,C.muted);
 }else if(d.kind==='proscons'){
   text(s,'VORTEILE',72,240,530,42,24,C.mint,true);
   text(s,'NACHTEILE',660,240,530,42,24,C.mint,true);
   for(let j=0;j<2;j++){
     text(s,d.body[j],72,320+j*115,530,105,32,C.paper,j===0);
     text(s,d.body[j+2],660,320+j*115,530,105,32,C.paper,j===0);
   }
   text(s,d.body[4],72,600,1136,55,26,C.muted);
 }else if(d.kind==='designer_senses'){
   text(s,d.body[0],72,217,1136,60,31,C.mint,true);
   table(s,d.table,72,305,1136,285);
   text(s,d.body[1],72,615,1136,42,25,C.muted);
 }else if(d.kind==='difficulty_values'){
   text(s,d.body[0],72,217,1136,60,30,C.mint,true);
   table(s,d.table,72,305,1136,275);
   text(s,d.body[1],72,605,1136,50,27,C.muted);
 }else if(d.kind==='designer_difficulty'){
   for(let j=0;j<3;j++){
     text(s,d.labels[j],72,230+j*130,1136,35,20,C.mint,true);
     text(s,d.body[j],72,275+j*130,1136,88,j===2?29:37,C.paper,j===0);
   }
 }else if(d.kind==='expansion'){
   text(s,'SENSORIK',72,235,530,48,25,C.mint,true);
   text(s,'DIFFICULTY SCALING',660,235,530,48,25,C.mint,true);
   for(let j=0;j<3;j++){
     text(s,d.body[j],72,315+j*95,530,85,30,C.paper,false);
     text(s,d.body[j+3],660,315+j*95,530,85,30,C.paper,false);
   }
   text(s,d.body[6],72,615,1136,40,24,C.muted);
 }else if(d.kind==='comparison'){
   text(s,d.body[0],72,210,1136,55,27,C.muted);
   table(s,d.table,72,285,1136,265);
   text(s,d.body[1],72,590,1136,60,28,C.mint);
 }else if(d.kind==='whyhow'){
   text(s,d.labels[0],72,226,1136,30,19,C.mint,true);
   text(s,d.body[0],72,266,1100,95,39,C.paper,true);
   text(s,d.labels[1],72,380,1136,30,19,C.mint,true);
   text(s,d.body[1],72,425,1136,100,38,C.paper);
   text(s,d.labels[2],72,549,1136,30,19,C.mint,true);
   text(s,d.body[2],72,587,1136,62,27,C.muted);
 }else if(d.kind==='tempo'){
   text(s,d.body[0],72,230,1136,70,35,C.mint,true);
   text(s,d.body[1],72,340,1136,85,31,C.paper);
   text(s,d.body[2],72,450,1136,130,83,C.paper,true);
   text(s,d.body[3],72,600,1136,48,28,C.muted);
 }else if(d.kind==='evidence_simple'){
   text(s,d.body[0],72,240,1136,100,42,C.mint,true);
   text(s,d.body[1],72,390,1136,90,35,C.paper);
   text(s,d.body[2],72,550,1136,90,32,C.muted);
 }else if(d.kind==='next'){
   for(let j=0;j<3;j++)text(s,d.body[j],72,245+j*130,1136,95,42,j===0?C.mint:C.paper,j===0);
 }else if(d.kind==='hook'){
   text(s,'Sichtkontakt verloren.',72,235,1100,90,60,C.mint,true);
   text(s,d.body[1],72,365,1000,130,39,C.paper);
   text(s,d.body[2],72,555,1050,65,31,C.muted);
 }else if(d.kind==='states'){
   text(s,d.body[0],72,230,1136,85,38,C.mint,true);
   text(s,d.body[1],72,355,1100,55,32);
   text(s,d.body[2],72,433,1100,55,32);
   text(s,d.body[3],72,535,1090,90,29,C.muted);
 }else if(d.kind==='combat'){
   const positions=[[72,230],[660,230],[72,385],[660,385]];
   for(let j=0;j<4;j++){const [x,y]=positions[j]; const [a,b]=d.body[j].split(': ');
     text(s,a,x,y,525,45,28,C.mint,true);text(s,b,x,y+53,525,88,36,C.paper);
   }
   text(s,d.body[4],72,565,1100,65,28,C.muted);
 }else if(d.kind==='intelligence'){
   text(s,d.body[0],72,225,510,115,36,C.mint,true);
   text(s,d.body[1],72,365,505,85,28,C.paper);
   text(s,d.body[2]+'\n'+d.body[3],72,485,510,145,27,C.muted);
   table(s,d.table,650,255,558,290);
   text(s,'Berechnete Sollintervalle · kein Lernen',650,575,558,52,23,C.muted);
 }else if(d.kind==='evidence'){
   text(s,'55',72,220,330,125,104,C.mint,true);
   text(s,'Blueprint-Prüfungen',72,355,490,60,32);
   text(s,'27',660,220,330,125,104,C.mint,true);
   text(s,'Animationsprüfungen',660,355,500,60,32);
   text(s,d.body[2],72,455,1100,65,31);
   text(s,d.body[3],72,555,1100,75,30,C.muted);
 }else if(d.kind==='closing'){
   text(s,d.body[0],72,275,1100,120,43,C.mint,true);
   text(s,d.body[1],72,445,1060,150,35,C.paper);
 }else if(d.kind==='sources'){
   const step=d.refs.length>6?59:d.refs.length>4?68:101;
   for(let j=0;j<d.refs.length;j++){
     const id=d.refs[j],q=data.sources.find(q=>q.id===id),e=data.evidence.find(e=>e.id===id);
     const label=q?`${id}  ${q.short}`:`${id}  ${e.title}`;
     const shortEvidence={E1:'Funktionsdokumentation und Blueprint-Authoring',E2:'Verification.txt · 55 PASS',E3:'AnimationPoseVerification.txt · 27 PASS',E4:'Arena-Protokoll · 3 Gegner, 3 Controller',E5:'Alt_Neu_Vergleich.md · Graphinspektion 13.11.2026'};
     const detail=q?q.title:shortEvidence[id];
     text(s,label,72,225+j*step,1130,34,d.refs.length>6?23:25,C.mint,true);
     const sh=text(s,detail,72,260+j*step,1130,42,d.refs.length>4?19:23,C.paper);
     if(q)sh.hyperlink={uri:q.url,isExternal:true};
   }
 }else{
   let y=230;
   for(let j=0;j<d.body.length;j++){
     const small=d.kind==='persona'&&j===3;
     text(s,d.body[j],72,y,1100,110,small?25:34,small?C.muted:(j===0?C.mint:C.paper),j===0);
     y+=d.kind==='goals'?125:99;
   }
 }
 const sourceNotes=d.refs.map(id=>{const q=data.sources.find(q=>q.id===id);const e=data.evidence.find(e=>e.id===id);return q?`${id}: ${q.short}. ${q.title}. ${q.url}. ${q.detail} Abruf 13.11.2026.`:`${id}: ${e.title}. ${e.path}. ${e.description}`;}).join('\n');
 s.speakerNotes.text=`Sprechzeit: ${d.seconds} Sekunden.\n\n${d.notes}\n\nQUELLEN\n${sourceNotes}`;
}
for(let i=0;i<p.slides.items.length;i++){
 const blob=await p.export({slide:p.slides.items[i],format:'png',scale:2});
 await fs.writeFile(path.join(build,`slide-${String(i+1).padStart(2,'0')}.png`),new Uint8Array(await blob.arrayBuffer()));
}
const candidate=path.join(build,'candidate.pptx');
await (await PresentationFile.exportPptx(p)).save(candidate);
const {finalizePresentation}=await import(pathToFileURL(path.join(skill,'container_tools/artifact_tool_utils.mjs')).href);
const result=await finalizePresentation({workspaceDir:root,candidatePath:candidate,finalPath:path.join(out,'EnemyAI_Sensory_Difficulty_V3_bearbeitbar.pptx'),pythonExecutable:path.join(runtime,'python/python.exe'),integrityValidatorPath:path.join(skill,'container_tools/inspect_presentation_package_integrity.py'),layoutValidatorPath:path.join(skill,'container_tools/inspect_presentation_layout_geometry.py'),layoutArgs:['--expected-slide-size-emu','12192000,6858000','--validate-heading-fit','--require-native-table-slide','3','--require-native-table-slide','6','--require-native-table-slide','7'],explicitTotalSlideCount:12,requiredNativeTableOwnerSlides:[3,6,7],requiredNativeChartOwnerSlides:[],fontPolicy:{basis:'design',families:['Arial']},verifyArtifactToolImport:true,receiptPath:path.join(build,'validation_v3.json')});
await fs.writeFile(path.join(build,'finalization-result.json'),JSON.stringify(result,null,2));
console.log('DECK_COMPLETE',out);
