# ACID FM1 controls

## Start

PLAY starts/stops the pattern. PRESET chooses one of eight original factory
patches and stops playback. Press PLAY again. HOME + PLAY is panic. Sequencer
mode is the default; keys transpose its root. In live mode they play a monophonic
last-note-priority voice. ARP switches between sequencer and held-note arp.

## Pages and encoders

SELECT turns through all 16 pages. On the SEQUENCER page it selects a step
instead; press HOME or another page button to leave. K1–K4 edit the four values
on screen. FX cycles effects / distortion / space. ENV opens envelope. LFO
cycles the three modulators; ALGO then chooses that modulator's destination.
SEQ switches sequencer / pattern settings. GLO switches clock / utility. EDIT
opens the sequencer. HOME returns to voice. SEL cycles scale.

| Page | K1 | K2 | K3 | K4 |
|---|---|---|---|---|
| Voice | Cutoff | Resonance | Envelope | Drive |
| Envelope | Decay | Accent | Slide | Gate |
| Effects | Delay mix | Feedback | Ambience | Chorus |
| Mod 1/2/3 | Rate | Depth | Shape | Cycle length |
| Sequencer | Note offset | Gate | Accent | Slide |
| Circuit | Pulse width | Sub | Bass boost | Noise |
| Clock | BPM | Swing | Clock source | Mode |
| Distortion | Type | Mix | Clipping | Volume |
| Pattern | Length | Direction | Accent length | Slide length |
| FX space | Delay division | Vibrato rate | Vibrato depth | Tracking |
| Mod route 1/2/3 | Destination | Rate | Depth | Length |
| Utility | Root | Scale | Accent attack | Mutation amount |

ALGO on other pages switches saw/pulse. PRESET chooses a factory patch.
OCT−/OCT+ shift the key range; held notes are released on octave changes.
REC toggles step recording: each key writes its offset and gate at the selected
step and advances the cursor. It does not record velocity or realtime timing.

On the sequencer page, **hold HOME then turn** K1 for probability, K2 for ratchet,
K3 for cutoff lock, K4 to clear the lock. A short HOME press returns to voice
on release; using HOME with another control acts as a modifier instead.

## Save / load

SAVE writes the complete patch/pattern into the current slot. Hold HOME and
turn ALGO to select slot 1–4. HOME + SAVE recalls it. Slot 1 is loaded at boot.
Loading any patch or slot stops playback. Presets and saves replace state;
back up before installing another firmware. Browser studio slots are separate.

## MIDI

All channels are accepted (no channel filter in this version). USB and TRS MIDI
input work through the platform. Note velocity 100 or above triggers accent.
Pitch bend maps ±2 semitones into Tune. Mod wheel controls vibrato depth.
CC 7 volume, 71 resonance, 74 cutoff, 75 decay, 76 envelope, 77 accent, 78 drive,
91 ambience, 93 delay; CC 120 panic, 123 all notes off. External MIDI clock is
24 PPQN; clock loss after two seconds releases the voice and waits for clock.
The sequencer follows start/continue/stop. No sequence MIDI output is emitted
by the firmware; the studio can export a MIDI file.

## Browser

Click Start sequence to enable audio. Drag knobs vertically; Shift gives fine
adjustment, arrows also work, double-click resets. Edit notes and gate/accent/
slide cells; use bank buttons for all 64 steps. Step number selects chance,
ratchet, and cutoff lock. Custom modulation curves use eight vertical sliders.

Generate / Mutate keep one undo state. Save/Recall uses this browser's storage;
export JSON for a durable backup. Import validates dimensions and ranges before
applying. MIDI input is explicitly connected by the user. WAV bounce renders
two cycles plus one second of tail; MIDI export renders one cycle and cannot
carry this synth's filter/glide sound. Arp/Live WAV bounce is not implemented.

Computer keys A W S E D F T G Y H U J K play semitones. Space toggles playback
when no form control is focused; Escape panics. Mobile supports pointer keys.
The hardware-view emulator runs the actual application and its LCD; it keeps
separate simulated storage. Its optional speech uses the browser's speech API.
