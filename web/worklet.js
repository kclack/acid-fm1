/* SPDX-License-Identifier: MIT */
import { createEngine } from './engine-api.js';
class AcidProcessor extends AudioWorkletProcessor {
  constructor() {
    super(); this.engine = null; this.frames = 0;
    this.port.onmessage = async ({ data: d }) => {
      try {
        if (d.type === 'init') {
          this.engine = await createEngine(d.bytes, sampleRate);
          this.engine.load(d.project);
          this.port.postMessage({ type: 'ready' });
        } else if (this.engine) {
          const x = this.engine.x;
          if (d.type === 'set') x.synth_set(d.id, d.value);
          if (d.type === 'step') x.synth_step(d.step, d.field, d.value);
          if (d.type === 'curve') x.synth_curve(d.mod, d.point, d.value);
          if (d.type === 'play') x.synth_play(d.on ? 1 : 0);
          if (d.type === 'panic') x.synth_panic();
          if (d.type === 'midi') x.synth_midi(...d.bytes);
          if (d.type === 'project') { x.synth_panic(); this.engine.load(d.project); if (d.playing) x.synth_play(1); }
        }
      } catch (error) { this.port.postMessage({ type: 'error', message: error.message }); }
    };
  }
  process(_inputs, outputs) {
    const out = outputs[0]; if (!out?.length) return true;
    if (!this.engine) { out.forEach(c => c.fill(0)); return true; }
    const n = out[0].length, audio = this.engine.render(n);
    for (let i = 0; i < n; i++) { out[0][i] = audio[i * 2]; if (out[1]) out[1][i] = audio[i * 2 + 1]; }
    this.frames += n;
    if (this.frames >= 2048) {
      this.frames = 0;
      this.port.postMessage({ type: 'status', values: Array.from({ length: 8 }, (_, i) => this.engine.x.synth_status(i)) });
    }
    return true;
  }
}
registerProcessor('acid-fm1', AcidProcessor);
