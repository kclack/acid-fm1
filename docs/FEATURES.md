# Feature translation

The goal is to translate the useful musical ideas in Acid V's public feature
set into a small original FM1 instrument, not to port its binary or reproduce
its proprietary model. Arturia references are listed in THIRD_PARTY.md.

| Idea | Implemented here | Difference / limit |
|---|---|---|
| Acid saw and pulse oscillator | PolyBLEP saw/pulse, morph and pulse width | Original oscillator, not Arturia DSP |
| Resonant acid filter | Four-pole TPT cascade with saturated feedback | Approximation; no component-level diode/circuit model |
| Accent / slide | Accent VCA and filter lift, attack, pitch glide, ties | Needs comparison on physical hardware |
| Advanced circuit controls | Bass boost, tracking, noise, clipping, sub, tune | Sub is a sine; no full component calibration panel |
| Vibrato | Rate and depth | Single sine source |
| Advanced sequencer | 1–64 steps, swing, probability, ratchets, cutoff locks | No full desktop piano-roll editor or DAW host sync |
| Polymetric lanes | Independent note, accent, slide and mod cycle lengths | Accent/slide follow transport count, not random note order |
| Arpeggiator | Held-note ascending, descending and random traversal | Pendulum sequence mode uses ascending arp; no full arp pattern library |
| Three function generators | Six shapes, eight-point custom curves, six destinations | One destination per lane, no arbitrary graph editor or routing matrix |
| Mutation | Seeded pattern generator and bounded mutation with studio undo | Original generator, not Arturia's algorithms |
| Distortion collection | Soft/hard clip, fold, crush, asymmetric, rectify, sine fold | Seven original algorithms, not the 14 Acid V modes |
| Effects | Delay, chorus, small comb ambience | No convolution, full reverb, phaser/flanger, dynamics or complete 17-effect rack |
| Presets | Eight original patches, four device slots, local browser slots | No Arturia presets imported |
| MIDI export | Browser Standard MIDI File export | Notes/velocities/gates; no filter automation or guarantee another synth uses overlap as slide |
| Audio export | Two-cycle stereo WAV bounce at 44.1 kHz | Offline bounce, not a DAW plugin |

## Memory / sample-rate choices

No allocation occurs in the engine render. Engine state is about 106 KiB in the
FM1's pool. The delay holds 32,768 signed 16-bit mono samples with stereo taps:
about 743 ms at 44.1 kHz. Slow tempo/long divisions clamp at that capacity. The
four-comb ambience is deliberately simpler than a desktop reverb.

The firmware and hardware emulator assume 44,100 Hz; physical I2S is approximately
44,117.6 Hz. The studio follows the browser's sample rate (32–96 kHz supported by
the C engine). Buffer durations therefore vary with browser rate. This is a
signal-level emulator, not a cycle-accurate hardware emulator.

Seven distortion types and all effect stages run inside the same original C
engine; 2× oversampling applies to the oscillator/filter voice, not the entire
effect rack. Nonlinear effects can alias. Their quality and the filter response
need listening tests; do not infer an Acid V sonic match from feature names.
