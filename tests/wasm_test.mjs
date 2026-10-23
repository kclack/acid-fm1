/* SPDX-License-Identifier: AGPL-3.0-only */
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { createEngine, validateProject } from '../web/engine-api.js';
const root=new URL('../',import.meta.url),bytes=fs.readFileSync(new URL('web/engine.wasm',root));
const meta=JSON.parse(fs.readFileSync(new URL('web/params.json',root)));
const a=await createEngine(bytes),b=await createEngine(bytes);
assert.equal(a.x.synth_param_count(),53);assert.equal(meta.length,53);
assert.equal(a.x.synth_project_size(),1088);
assert.doesNotThrow(()=>validateProject(a.snapshot(),meta));
const invalid=a.snapshot();invalid.steps[3][5]=0;assert.throws(()=>validateProject(invalid,meta));
invalid.steps[3][5]=1;invalid.params[16]=3.1;assert.throws(()=>validateProject(invalid,meta));
a.x.synth_play(1);b.x.synth_play(1);
for(let i=0;i<16;i++)assert.deepEqual(a.render(1024).slice(),b.render(1024).slice());
a.x.synth_generate(123,.7);const patch=a.snapshot();b.load(patch);assert.deepEqual(a.snapshot(),b.snapshot());
for(let i=0;i<8;i++){
  a.x.synth_preset(i);validateProject(a.snapshot(),meta);a.x.synth_play(1);
  if(i===7)a.x.synth_midi(0x90,57,110);
  let peak=0;for(let j=0;j<20;j++)for(const v of a.render(1024)){assert(Number.isFinite(v)&&Math.abs(v)<=1);peak=Math.max(peak,Math.abs(v));}
  assert(peak>.005,`preset ${i} must be audible`);
}
// Compare exactly the native test scenario with the checked-in Wasm build.
const nativeFile=new URL('build/demo.f32',root);
if(fs.existsSync(nativeFile)){
  const raw=fs.readFileSync(nativeFile),native=new Float32Array(raw.buffer,raw.byteOffset,raw.byteLength/4),engine=await createEngine(bytes);
  engine.x.synth_play(1);let maxError=0,sum=0;
  for(let offset=0;offset<native.length;){const frames=Math.min(1024,(native.length-offset)/2),samples=engine.render(frames);
    for(let i=0;i<samples.length;i++){const e=Math.abs(samples[i]-native[offset+i]);maxError=Math.max(maxError,e);sum+=e*e;}offset+=samples.length;
  }
  assert(maxError<.00005,`native/Wasm max error ${maxError}`);
  console.log(`Native/Wasm agreement: max=${maxError}, RMS=${Math.sqrt(sum/native.length)}`);
}
console.log('PASS Wasm presets, deterministic rendering, project validation, and native agreement');
