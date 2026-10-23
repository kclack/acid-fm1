/* SPDX-License-Identifier: AGPL-3.0-only */
import { createEngine, validateProject } from './engine-api.js';
const $ = id => document.getElementById(id);
const presets = ['Foundation','Warehouse','Liquid Chrome','Broken Circuit','Dub Corrosion','Polymetric','Slow Burn','Init Live'];
const descriptions = [
  'The essential acid voice. Sawtooth, resonance, slide, and accent.',
  'Driven and resonant. A fast, raw warehouse bassline.',
  'Pulse waves and moving width, polished with chorus and delay.',
  'Crushed edges, swung steps, and a few extra ratchets.',
  'Slow echoes and spacious tails. Give the notes some room.',
  'Accent, slide, and modulation lanes fall out of step, then meet again.',
  'A low, patient burn. Sub bass, long decay, and a dark filter.',
  'A clean starting point for playing the keyboard or a MIDI controller.',
];
const options = {
  11:['Soft clip','Hard clip','Wavefold','Bit crush','Asymmetric','Rectify','Sine fold'],
  17:['Forward','Reverse','Pendulum','Random'],
  19:['Chromatic','Minor','Minor pentatonic','Dorian','Major'],
  22:['1/16','1/8 triplet','1/8','Dotted 1/8','1/4','Dotted 1/4'],
  30:['Sequencer','Live / legato','Arpeggiator'],31:['Internal','MIDI input'],
};
for(let i=0;i<3;i++){options[34+i*5]=['Off','Cutoff','Resonance','Pulse width','Drive','Pitch','Amplitude'];options[35+i*5]=['Sine','Triangle','Ramp','Square','Random hold','Custom curve'];}
const noteNames=['C','C♯','D','D♯','E','F','F♯','G','G♯','A','A♯','B'];
const noteName=n=>`${noteNames[((n%12)+12)%12]}${Math.floor(n/12)-1}`;
let meta,bytes,model,state,ctx,node,analyser,ready=false,playing=false,bank=0,selectedStep=0,octave=36,undo=null;
let toastTimer,scopeData,scopeCtx=$('scope').getContext('2d'),midiAccess,audioStarting;
const held=new Map(),sounding=new Set(),pointers=new Map(),computerKeys=new Map();
function toast(message){$('toast').textContent=message;$('toast').classList.add('show');clearTimeout(toastTimer);toastTimer=setTimeout(()=>$('toast').classList.remove('show'),3500);}
function send(data){node?.port.postMessage(data);}
function randomSeed(){return crypto.getRandomValues(new Uint32Array(1))[0]||1;}
function fmt(id,v){
  if(options[id])return options[id][Math.round(v)];
  if(id===0)return `${Math.round(35*2**(v*8.2))} Hz`;
  if(id===3)return `${Math.round(25+v*v*2400)} ms`;
  if(id===5)return `${Math.round(2+v*v*350)} ms`;
  if(id===18)return noteName(Math.round(v));
  if(id===28)return `${v.toFixed(1)} Hz`;
  if(meta[id].discrete||id===13)return String(Math.round(v));
  if(id===8)return `${v>=0?'+':''}${v.toFixed(1)} st`;
  return `${Math.round(v*100)}%`;
}
function setParam(id,value){
  if(!ready||!Number.isFinite(value))return;
  model.x.synth_set(id,value);state.params[id]=model.x.synth_get(id);
  send({type:'set',id,value:state.params[id]});syncParam(id);
  if([16,18,19].includes(id))renderSteps();
}
function setStep(step,field,value){model.x.synth_step(step,field,value);state.steps[step][field]=model.x.synth_step_get(step,field);send({type:'step',step,field,value:state.steps[step][field]});}
function setCurve(mod,point,value){model.x.synth_curve(mod,point,value);state.curves[mod][point]=value;send({type:'curve',mod,point,value});}
function syncParam(id){
  const v=state.params[id];
  document.querySelectorAll(`[data-param="${id}"]`).forEach(el=>{
    if(el.classList.contains('knob')){
      const t=(v-meta[id].min)/(meta[id].max-meta[id].min);
      el.style.setProperty('--angle',`${-135+t*270}deg`);el.parentElement.style.setProperty('--pct',t);
      el.setAttribute('aria-valuenow',v);el.setAttribute('aria-valuetext',fmt(id,v));
    }else el.value=v;
  });
  document.querySelectorAll(`[data-value="${id}"]`).forEach(el=>{el.textContent=fmt(id,v);});
  if(id===6){$('saw').classList.toggle('selected',v<.5);$('pulse').classList.toggle('selected',v>=.5);}
  if(id===25){$('volume').value=v;$('volume-readout').textContent=fmt(id,v);}
  if(id===13)$('tempo').value=Math.round(v);
  if(id===30)$('mode').value=v;
  if(id===31)$('clock-status').textContent=v?'MIDI INPUT':'INTERNAL';
}
function refresh(){for(let id=0;id<meta.length;id++)syncParam(id);for(let m=0;m<3;m++)for(let i=0;i<8;i++)$(`curve-${m}-${i}`).value=state.curves[m][i];renderSteps();}
function applyProject(p,restorePlay=playing){validateProject(p,meta);model.load(p);state=model.snapshot();send({type:'project',project:state,playing:restorePlay});refresh();}
function remember(){undo=structuredClone(state);$('undo').disabled=false;}
function setPlaying(on){playing=!!on;model.x.synth_play(playing?1:0);send({type:'play',on:playing});$('play').classList.toggle('playing',playing);$('play').innerHTML=playing?'■ <span>Stop sequence</span>':'▶ <span>Start sequence</span>';}
async function enableAudio(){
  if(audioStarting)return audioStarting;
  audioStarting=enableAudioImpl();
  try{return await audioStarting;}finally{audioStarting=null;}
}
async function enableAudioImpl(){
  if(ctx){await ctx.resume();return;}
  if(!ready)throw new Error('The engine is still loading.');
  const Audio=window.AudioContext||window.webkitAudioContext;
  if(!Audio)throw new Error('Web Audio is unavailable in this browser.');
  ctx=new Audio({latencyHint:'interactive'});
  try{
    await ctx.audioWorklet.addModule(new URL('./worklet.js',import.meta.url));
    node=new AudioWorkletNode(ctx,'acid-fm1',{numberOfInputs:0,numberOfOutputs:1,outputChannelCount:[2]});
    analyser=ctx.createAnalyser();analyser.fftSize=1024;scopeData=new Float32Array(1024);
    node.connect(analyser);analyser.connect(ctx.destination);
    await new Promise((resolve,reject)=>{
      const timer=setTimeout(()=>reject(new Error('The audio engine did not start.')),12000);
      node.port.onmessage=({data:d})=>{
        if(d.type==='ready'){clearTimeout(timer);resolve();}
        if(d.type==='error'){clearTimeout(timer);reject(new Error(d.message));toast(d.message);}
        if(d.type==='status')updateStatus(d.values);
      };
      node.port.postMessage({type:'init',bytes:bytes.slice(0),project:state});
    });
    await ctx.resume();$('engine-state').textContent=`LIVE ENGINE · ${Math.round(ctx.sampleRate/100)/10} kHz`;
    ctx.onstatechange=()=>{if(ctx?.state==='suspended')$('engine-state').textContent='AUDIO PAUSED · CLICK PLAY TO RESUME';};
  }catch(error){node?.disconnect();await ctx.close();ctx=null;node=null;throw error;}
}
function updateStatus(values){
  const [position,isPlaying,peak,lost]=values;
  if(!!isPlaying!==playing){playing=!!isPlaying;$('play').classList.toggle('playing',playing);$('play').innerHTML=playing?'■ <span>Stop sequence</span>':'▶ <span>Start sequence</span>';}
  document.querySelectorAll('.step-no.current').forEach(el=>el.classList.remove('current'));
  if(isPlaying&&position>=0)document.querySelector(`[data-position="${position}"]`)?.classList.add('current');
  $('meter').style.width=`${Math.min(100,Math.max(0,(20*Math.log10(Math.max(peak,.00001))+60)/60*100))}%`;
  $('clock-status').textContent=lost?'WAITING FOR CLOCK':state.params[31]?'MIDI INPUT':'INTERNAL';
}
function rangeControl(id,parent,{compact=false}={}){
  const m=meta[id],wrap=document.createElement('div');wrap.className=compact?'compact-control':'range-control';
  const label=document.createElement('label');label.htmlFor=`param-${id}-${document.querySelectorAll(`[data-param="${id}"]`).length}`;label.textContent=m.name;
  const out=document.createElement('output');out.dataset.value=id;if(!compact)label.append(out);
  let input;
  if(options[id]||id===18){input=document.createElement('select');const choices=options[id]||Array.from({length:61},(_,i)=>noteName(i+24));choices.forEach((name,i)=>{const o=document.createElement('option');o.value=id===18?i+24:i;o.textContent=name;input.append(o);});}
  else {input=document.createElement('input');input.type=compact&&m.discrete?'number':'range';input.min=m.min;input.max=m.max;input.step=m.discrete?1:(m.max-m.min)/100;}
  input.id=label.htmlFor;input.dataset.param=id;input.setAttribute('aria-label',m.name);input.addEventListener('input',()=>setParam(id,+input.value));
  wrap.append(label,input);parent.append(wrap);return wrap;
}
function createKnob(id){
  const group=document.createElement('div');group.className='knob-group';const wrap=document.createElement('div');wrap.className='knob-wrap';
  const knob=document.createElement('button');knob.className='knob';knob.dataset.param=id;knob.setAttribute('role','slider');knob.setAttribute('aria-label',meta[id].name);knob.setAttribute('aria-valuemin',meta[id].min);knob.setAttribute('aria-valuemax',meta[id].max);
  const label=document.createElement('span');label.className='knob-label';label.textContent=meta[id].name;
  const value=document.createElement('output');value.className='knob-value';value.dataset.value=id;
  let drag;
  knob.addEventListener('pointerdown',e=>{drag={y:e.clientY,v:state.params[id]};knob.setPointerCapture(e.pointerId);});
  knob.addEventListener('pointermove',e=>{if(drag)setParam(id,drag.v+(drag.y-e.clientY)*(meta[id].max-meta[id].min)/(e.shiftKey?1200:160));});
  const end=()=>drag=null;knob.addEventListener('pointerup',end);knob.addEventListener('pointercancel',end);knob.addEventListener('lostpointercapture',end);
  knob.addEventListener('dblclick',()=>setParam(id,meta[id].default));
  knob.addEventListener('keydown',e=>{if(['ArrowUp','ArrowRight','ArrowDown','ArrowLeft','Home','End'].includes(e.key)){e.preventDefault();const sign=['ArrowUp','ArrowRight'].includes(e.key)?1:-1;setParam(id,e.key==='Home'?meta[id].min:e.key==='End'?meta[id].max:state.params[id]+sign*(meta[id].max-meta[id].min)/(e.shiftKey?1000:100));}});
  wrap.append(knob);group.append(wrap,label,value);$('main-knobs').append(group);
}
function group(title,ids,parent){const g=document.createElement('section');g.className='control-group';const h=document.createElement('h3');h.textContent=title;g.append(h);ids.forEach(id=>rangeControl(id,g));parent.append(g);}
function renderSteps(){
  const grid=$('step-grid');grid.replaceChildren();
  const label=text=>{const el=document.createElement('span');el.className='row-label';el.textContent=text;grid.append(el);};
  label('STEP');for(let i=0;i<16;i++){const s=bank*16+i,b=document.createElement('button');b.className='step-no'+(s===selectedStep?' selected':'');b.textContent=String(s+1).padStart(2,'0');b.dataset.position=s;b.setAttribute('aria-label',`Edit step ${s+1}`);b.onclick=()=>{selectedStep=s;renderSteps();};grid.append(b);}
  label('NOTE');for(let i=0;i<16;i++){
    const s=bank*16+i,input=document.createElement('input');input.type='number';input.min=-24;input.max=36;input.step=1;input.value=state.steps[s][0];input.className='step-note'+(s>=state.params[16]?' outside':'');input.setAttribute('aria-label',`Step ${s+1} note offset`);
    input.onchange=()=>{setStep(s,0,+input.value);input.value=state.steps[s][0];selectedStep=s;updateStepDetail();};grid.append(input);
  }
  for(const [field,name,cls]of [[1,'GATE',''],[2,'ACCENT','accent'],[3,'SLIDE','slide']]){
    label(name);for(let i=0;i<16;i++){const s=bank*16+i,b=document.createElement('button');b.className=`step-toggle ${cls}${state.steps[s][field]?' on':''}${s>=state.params[16]?' outside':''}`;b.setAttribute('aria-label',`Step ${s+1} ${name.toLowerCase()}`);b.setAttribute('aria-pressed',!!state.steps[s][field]);b.onclick=()=>{setStep(s,field,state.steps[s][field]?0:1);selectedStep=s;renderSteps();};grid.append(b);}
  }
  updateStepDetail();
}
function updateStepDetail(){const s=state.steps[selectedStep];$('selected-step').textContent=String(selectedStep+1).padStart(2,'0');$('step-chance').value=s[4];$('chance-value').textContent=`${s[4]}%`;$('step-ratchet').value=s[5];$('step-lock').value=s[6];$('lock-value').textContent=s[6]<0?'OFF':`${Math.round(s[6]*100)}%`;}
function buildControls(){
  for(let id=0;id<6;id++)createKnob(id);
  [18,19,16,17,14,50,51].forEach(id=>rangeControl(id,$('seq-settings'),{compact:true}));
  group('01 / DISTORTION',[10,11,12,49],$('fx-controls'));group('02 / ECHO',[20,21,22],$('fx-controls'));group('03 / SPACE',[23,24],$('fx-controls'));
  group('OSCILLATOR',[6,7,8,9],$('circuit-controls'));group('CIRCUIT',[26,27,47,48],$('circuit-controls'));group('PERFORMANCE',[15,28,29,31,52],$('circuit-controls'));
  for(let m=0;m<3;m++){
    const lane=document.createElement('section');lane.className='mod-lane';const title=document.createElement('h3');title.textContent=`MOD 0${m+1}`;lane.append(title);
    const controls=document.createElement('div');controls.className='mod-controls';[32,33,34,35,36].forEach(id=>rangeControl(id+m*5,controls));lane.append(controls);
    const curve=document.createElement('div');curve.className='curve-editor';curve.title='Custom curve: eight points, interpolated cyclically';
    for(let i=0;i<8;i++){const input=document.createElement('input');input.type='range';input.min=-1;input.max=1;input.step=.01;input.id=`curve-${m}-${i}`;input.setAttribute('aria-label',`Modulation ${m+1} curve point ${i+1}`);input.oninput=()=>setCurve(m,i,+input.value);curve.append(input);}lane.append(curve);$('mod-lanes').append(lane);
  }
}
function download(data,name,type){const u=URL.createObjectURL(new Blob([data],{type})),a=document.createElement('a');a.href=u;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(u),2000);}
function sendMidi(st,note=0,velocity=0){model.x.synth_midi(st,note,velocity);send({type:'midi',bytes:[st,note,velocity]});if([0x90,0xb0,0xe0].includes(st&0xf0))for(let id=0;id<meta.length;id++){state.params[id]=model.x.synth_get(id);syncParam(id);}}
async function noteDown(note){
  const count=held.get(note)||0;held.set(note,count+1);
  try{await enableAudio();if(!held.has(note))return;if(!sounding.has(note)){sounding.add(note);sendMidi(0x90,note,110);}document.querySelector(`[data-note="${note}"]`)?.classList.add('pressed');}
  catch(e){toast(e.message);}
}
function noteUp(note){const count=held.get(note)||0;if(count>1){held.set(note,count-1);return;}held.delete(note);if(ready&&sounding.has(note)){sendMidi(0x80,note,0);sounding.delete(note);}document.querySelector(`[data-note="${note}"]`)?.classList.remove('pressed');}
function releaseKeys(){for(const note of sounding){if(ready)sendMidi(0x80,note,0);}held.clear();sounding.clear();pointers.clear();computerKeys.clear();document.querySelectorAll('.key.pressed').forEach(el=>el.classList.remove('pressed'));}
function buildKeyboard(){
  releaseKeys();const board=$('keyboard');board.replaceChildren();let white=0;const whites=15;
  for(let i=0;i<25;i++){
    const note=octave+i,black=[1,3,6,8,10].includes(note%12),b=document.createElement('button');b.className=`key ${black?'black-key':'white-key'}`;b.dataset.note=note;b.setAttribute('aria-label',noteName(note));
    if(black)b.style.left=`${white/whites*100-1.95}%`;else white++;
    const label=document.createElement('span');label.textContent=note%12===0?noteName(note):'';b.append(label);
    b.onpointerdown=e=>{e.preventDefault();b.setPointerCapture(e.pointerId);pointers.set(e.pointerId,note);noteDown(note);};
    const up=e=>{if(pointers.has(e.pointerId)){noteUp(pointers.get(e.pointerId));pointers.delete(e.pointerId);}};b.onpointerup=up;b.onpointercancel=up;b.onlostpointercapture=up;
    b.onkeydown=e=>{if(e.key==='Enter'&&!e.repeat){e.preventDefault();noteDown(note);}};b.onkeyup=e=>{if(e.key==='Enter')noteUp(note);};board.append(b);
  }
  $('octave').textContent=`${noteName(octave)} — ${noteName(octave+24)}`;
}
function scope(){
  requestAnimationFrame(scope);const canvas=$('scope'),c=scopeCtx,w=canvas.width,h=canvas.height;c.clearRect(0,0,w,h);c.strokeStyle='#60744e';c.lineWidth=1;c.beginPath();c.moveTo(0,h/2);c.lineTo(w,h/2);c.stroke();
  if(!analyser)return;analyser.getFloatTimeDomainData(scopeData);c.strokeStyle='#c5f477';c.lineWidth=1.5;c.beginPath();for(let i=0;i<scopeData.length;i++){const x=i/scopeData.length*w,y=h/2-scopeData[i]*h*.8;i?c.lineTo(x,y):c.moveTo(x,y);}c.stroke();
}

function bind(){
  $('play').onclick=async()=>{try{await enableAudio();setPlaying(!playing);}catch(e){toast(e.message);}};
  $('panic').onclick=()=>{releaseKeys();setPlaying(false);model.x.synth_panic();send({type:'panic'});toast('All notes and effect tails silenced.');};
  $('volume').oninput=()=>setParam(25,+$('volume').value);$('tempo').onchange=()=>setParam(13,+$('tempo').value);$('mode').onchange=()=>{releaseKeys();setParam(30,+$('mode').value);};
  $('saw').onclick=()=>setParam(6,0);$('pulse').onclick=()=>setParam(6,1);
  presets.forEach((name,i)=>{const o=document.createElement('option');o.value=i;o.textContent=`${String(i+1).padStart(2,'0')} / ${name.toUpperCase()}`;$('preset').append(o);});
  $('preset').onchange=()=>{remember();const p=+$('preset').value;model.x.synth_preset(p);state=model.snapshot();$('patch-description').textContent=descriptions[p];send({type:'project',project:state,playing});refresh();};
  document.querySelectorAll('[data-tab]').forEach(b=>b.onclick=()=>{document.querySelectorAll('[data-tab]').forEach(t=>{t.classList.toggle('active',t===b);t.setAttribute('aria-selected',t===b);});document.querySelectorAll('.tab-content').forEach(c=>c.hidden=c.id!==b.dataset.tab);});
  document.querySelectorAll('[data-bank]').forEach(b=>b.onclick=()=>{bank=+b.dataset.bank;selectedStep=bank*16;document.querySelectorAll('[data-bank]').forEach(t=>t.classList.toggle('active',t===b));renderSteps();});
  $('step-chance').oninput=()=>{setStep(selectedStep,4,+$('step-chance').value);updateStepDetail();};$('step-ratchet').onchange=()=>setStep(selectedStep,5,+$('step-ratchet').value);$('step-lock').oninput=()=>{setStep(selectedStep,6,+$('step-lock').value);updateStepDetail();};$('clear-lock').onclick=()=>{setStep(selectedStep,6,-1);updateStepDetail();};
  $('generate').onclick=()=>{remember();model.x.synth_generate(randomSeed(),.7);state=model.snapshot();send({type:'project',project:state,playing});refresh();toast('New seeded pattern. Undo keeps your last one.');};
  $('mutate').onclick=()=>{remember();model.x.synth_mutate(randomSeed(),state.params[52]);state=model.snapshot();send({type:'project',project:state,playing});refresh();toast('Pattern mutated.');};
  $('undo').onclick=()=>{if(undo){const old=undo;undo=structuredClone(state);applyProject(old);toast('Previous pattern restored.');}};
  $('save').onclick=()=>{try{localStorage.setItem(`acid-fm1-slot-${$('slot').value}`,JSON.stringify(state));toast('Patch saved in this browser. Export a file for backup.');}catch{toast('Browser storage is unavailable. Export your patch instead.');}};
  $('load').onclick=()=>{try{const p=localStorage.getItem(`acid-fm1-slot-${$('slot').value}`);if(!p){toast('This local slot is empty.');return;}remember();applyProject(JSON.parse(p));toast('Local patch recalled.');}catch(e){toast(e.message);}};
  $('export-project').onclick=()=>download(JSON.stringify(state,null,2),'acid-fm1-patch.json','application/json');$('import-project').onclick=()=>$('project-file').click();
  $('project-file').onchange=async()=>{const f=$('project-file').files[0];if(!f)return;try{if(f.size>100000)throw new Error('Patch files must be smaller than 100 KB.');const p=validateProject(JSON.parse(await f.text()),meta);remember();releaseKeys();applyProject(p);toast('Patch imported.');}catch(e){toast(e.message);}finally{$('project-file').value='';}};
  $('oct-down').onclick=()=>{octave=Math.max(24,octave-12);buildKeyboard();};$('oct-up').onclick=()=>{octave=Math.min(72,octave+12);buildKeyboard();};
  $('midi').onclick=async()=>{
    try{if(!navigator.requestMIDIAccess)throw new Error('Web MIDI requires a supported browser such as Chrome or Edge.');await enableAudio();midiAccess=await navigator.requestMIDIAccess({sysex:false});
      const connect=()=>{let count=0;for(const input of midiAccess.inputs.values()){if(input.state==='connected')count++;input.onmidimessage=e=>{const [st,d1=0,d2=0]=e.data;sendMidi(st,d1,d2);};}$('midi-status').textContent=count?`${count} MIDI input${count===1?'':'s'} connected`:'No MIDI inputs found. Connect a device.';};midiAccess.onstatechange=connect;connect();$('midi').textContent='Refresh MIDI inputs';
    }catch(e){toast(e.message);}
  };
  const mapping={'a':0,'w':1,'s':2,'e':3,'d':4,'f':5,'t':6,'g':7,'y':8,'h':9,'u':10,'j':11,'k':12,'o':13,'l':14,'p':15,';':16};
  document.addEventListener('keydown',e=>{if(['INPUT','SELECT','TEXTAREA','BUTTON'].includes(e.target.tagName))return;if(e.key==='Escape'){$('panic').click();return;}if(e.code==='Space'){e.preventDefault();if(!e.repeat)$('play').click();return;}const key=e.key.toLowerCase();if(mapping[key]!==undefined&&!e.repeat&&!e.metaKey&&!e.ctrlKey){e.preventDefault();const note=octave+mapping[key];computerKeys.set(key,note);noteDown(note);}});
  document.addEventListener('keyup',e=>{const key=e.key.toLowerCase();if(computerKeys.has(key)){noteUp(computerKeys.get(key));computerKeys.delete(key);}});window.addEventListener('blur',releaseKeys);
  $('wav-export').onclick=bounceWav;$('midi-export').onclick=exportMidi;
}

async function bounceWav(){
  const button=$('wav-export');button.disabled=true;button.textContent='Rendering…';
  try{
    if(state.params[30]!==0)throw new Error('Switch to Sequencer mode to bounce a pattern.');
    const engine=await createEngine(bytes,44100),patch=structuredClone(state);patch.params[31]=0;engine.load(patch);engine.x.synth_play(1);
    const cycles=2,frames=Math.ceil(state.params[16]*15/state.params[13]*44100*cycles),tail=44100,total=frames+tail,data=new ArrayBuffer(44+total*4),v=new DataView(data);
    const text=(offset,s)=>[...s].forEach((c,i)=>v.setUint8(offset+i,c.charCodeAt(0)));
    text(0,'RIFF');v.setUint32(4,36+total*4,true);text(8,'WAVEfmt ');v.setUint32(16,16,true);v.setUint16(20,1,true);v.setUint16(22,2,true);v.setUint32(24,44100,true);v.setUint32(28,176400,true);v.setUint16(32,4,true);v.setUint16(34,16,true);text(36,'data');v.setUint32(40,total*4,true);
    for(let offset=0;offset<total;){
      if(offset===frames)engine.x.synth_play(0);
      const n=Math.min(1024,total-offset,offset<frames?frames-offset:total-offset),samples=engine.render(n);
      for(let i=0;i<n*2;i++)v.setInt16(44+offset*4+i*2,Math.max(-32768,Math.min(32767,Math.round(samples[i]*32767))),true);
      offset+=n;if(offset%32768===0)await new Promise(r=>setTimeout(r,0));
    }
    download(data,'acid-fm1-pattern.wav','audio/wav');toast('Two pattern cycles bounced at 44.1 kHz, with a one-second tail.');
  }catch(e){toast(e.message);}finally{button.disabled=false;button.textContent='Bounce WAV ↓';}
}
function exportMidi(){
  const ppq=480,events=[],p=state.params,len=Math.round(p[16]),mask=[0xfff,0x5ad,0x4a9,0x6ad,0xab5][Math.round(p[19])];let t=0,seed=state.seed||1,pos=p[17]===1?len-1:0,direction=1;
  const rand=()=>{seed^=seed<<13;seed^=seed>>>17;seed^=seed<<5;return seed>>>0;};
  for(let step=0;step<len;step++){
    const s=state.steps[pos],ticks=120*(1+(step%2?-p[14]:p[14])),accent=state.steps[step%p[50]][2],slide=state.steps[step%p[51]][3];
    const chance=(rand()>>>8)/16777216*100;
    if(s[1]&&chance<s[4]){
      let note=s[0];while(!(mask&(1<<((note%12+12)%12))))note--;
      note=Math.max(0,Math.min(127,Math.round(p[18])+note));
      for(let r=0;r<s[5];r++){const start=t+r*ticks/s[5],dur=ticks/s[5]*(slide?1.02:p[15]);events.push({t:Math.round(start),b:[0x90,note,accent?120:86]},{t:Math.round(start+dur),b:[0x80,note,0]});}
    }
    t+=ticks;
    if(p[17]===1)pos=(pos+len-1)%len;else if(p[17]===2){pos+=direction;if(pos>=len){direction=-1;pos=Math.max(0,len-2);}if(pos<0){direction=1;pos=Math.min(1,len-1);}}else if(p[17]===3)pos=rand()%len;else pos=(pos+1)%len;
  }
  const tempo=Math.round(60000000/p[13]),track=[0,0xff,0x51,3,tempo>>16&255,tempo>>8&255,tempo&255];let prev=0;
  const vlq=n=>{const a=[n&127];while((n>>>=7))a.unshift((n&127)|128);return a;};
  events.sort((a,b)=>a.t-b.t||a.b[0]-b.b[0]);for(const e of events){track.push(...vlq(Math.max(0,e.t-prev)),...e.b);prev=e.t;}track.push(0,0xff,0x2f,0);
  const size=track.length,header=[77,84,104,100,0,0,0,6,0,0,0,1,ppq>>8,ppq&255,77,84,114,107,size>>>24&255,size>>>16&255,size>>>8&255,size&255];
  download(new Uint8Array([...header,...track]),'acid-fm1-pattern.mid','audio/midi');toast('One cycle exported. MIDI preserves notes and overlaps, not the synth’s filter or glide sound.');
}

async function boot(){
  try{
    const results=await Promise.all([fetch(new URL('./engine.wasm',import.meta.url)),fetch(new URL('./params.json',import.meta.url))]);
    if(results.some(r=>!r.ok))throw new Error('Could not load the firmware engine.');
    bytes=await results[0].arrayBuffer();meta=await results[1].json();model=await createEngine(bytes);state=model.snapshot();ready=true;
    buildControls();bind();refresh();buildKeyboard();scope();$('play').disabled=false;$('engine-led').classList.add('ready');$('engine-state').textContent='ENGINE READY · CLICK PLAY';
    window.acidLab={get project(){return structuredClone(state);},get playing(){return playing;},get context(){return ctx;},get model(){return model;},get analyser(){return analyser;}};
  }catch(e){$('engine-state').textContent='ENGINE UNAVAILABLE';$('patch-description').textContent=e.message;toast(e.message);console.error(e);}
}
boot();
