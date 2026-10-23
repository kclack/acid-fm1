/* SPDX-License-Identifier: MIT */
export async function createEngine(bytes, sampleRate = 44100) {
  const { instance } = await WebAssembly.instantiate(bytes, {
    env: { emscripten_notify_memory_growth() {} },
    wasi_snapshot_preview1: { proc_exit(code) { throw new Error(`Engine exit ${code}`); } },
  });
  const x = instance.exports;
  x._initialize?.();
  x.synth_init(sampleRate);
  const api = {
    x,
    render(frames) { const p = x.synth_render(frames); return new Float32Array(x.memory.buffer, p, frames * 2); },
    snapshot() {
      return {
        format: 'acid-fm1', version: 1, seed: x.synth_seed(),
        params: Array.from({ length: x.synth_param_count() }, (_, i) => x.synth_get(i)),
        steps: Array.from({ length: 64 }, (_, s) => Array.from({ length: 7 }, (_, f) => x.synth_step_get(s, f))),
        curves: Array.from({ length: 3 }, (_, m) => Array.from({ length: 8 }, (_, i) => x.synth_curve_get(m, i))),
      };
    },
    load(p) {
      x.synth_seed_set(p.seed || 1);
      p.params.forEach((v, i) => x.synth_set(i, v));
      p.steps.forEach((s, i) => s.forEach((v, f) => x.synth_step(i, f, v)));
      p.curves.forEach((c, m) => c.forEach((v, i) => x.synth_curve(m, i, v)));
    },
  };
  return api;
}

export function validateProject(p, meta) {
  if (!p || p.format !== 'acid-fm1' || p.version !== 1) throw new Error('Choose an ACID FM1 version 1 project.');
  if (!Number.isInteger(p.seed) || p.seed < 1 || p.seed > 4294967295) throw new Error('Invalid pattern seed.');
  const valid = (v, lo, hi, integer = false) => typeof v === 'number' && Number.isFinite(v) && v >= lo && v <= hi && (!integer || Number.isInteger(v));
  if (!Array.isArray(p.params) || p.params.length !== meta.length || !p.params.every((v, i) => valid(v, meta[i].min, meta[i].max, meta[i].discrete))) throw new Error('Invalid synth parameters.');
  const limits = [[-24, 36, true], [0, 1, true], [0, 1, true], [0, 1, true], [0, 100, true], [1, 4, true], [-1, 1]];
  if (!Array.isArray(p.steps) || p.steps.length !== 64 || !p.steps.every(s => Array.isArray(s) && s.length === 7 && s.every((v, f) => valid(v, ...limits[f])))) throw new Error('Invalid sequencer steps.');
  if (!Array.isArray(p.curves) || p.curves.length !== 3 || !p.curves.every(c => Array.isArray(c) && c.length === 8 && c.every(v => valid(v, -1, 1)))) throw new Error('Invalid modulation curves.');
  return p;
}
