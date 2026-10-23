# ACID FM1

An original acid bassline firmware for the **M-VAVE FM-1**, with two browser
emulators. Inspired by the public feature set of Arturia Acid V and classic
acid instruments. No Arturia code, presets, or assets are used.

**Experimental, version 0.1.0. Builds and simulations are tested; this firmware
has not been run on a physical FM1.** Do not interpret a successful compile or
browser demo as proof of device timing, audio quality, or safe recovery.

- [Play the studio](https://kclack.github.io/acid-fm1/)
- [Emulate the real FM1 controls](https://kclack.github.io/acid-fm1/hardware/)
- [Guide and hardware mapping](https://kclack.github.io/acid-fm1/guide.html)
- [Experimental firmware downloads](https://github.com/kclack/acid-fm1/releases)

## What is implemented

- Monophonic saw/pulse voice with PolyBLEP edges and 2× voice oversampling.
- Original saturated four-pole TPT feedback filter; cutoff, resonance,
  envelope, decay, accent amount/attack, tracking, bass boost, clipping.
- Legato slide, variable pulse width, sine sub, circuit noise, vibrato, tune.
- Up to 64 steps with notes, rests, accents, slides, per-step probability,
  1–4 ratchets, and cutoff locks.
- Independent note/accent/slide loop lengths, swing, four sequence directions,
  root/scale selection, seeded generation, and mutation.
- Three tempo-related modulators with independent cycle lengths; six shapes
  including eight-point curves, and cutoff/resonance/PW/drive/pitch/level routes.
- Seven distortion modes, mono feedback delay with stereo taps, chorus,
  and compact four-comb ambience. Not Arturia's complete effects suite.
- Live last-note-priority mode and an arpeggiator; MIDI notes, transport,
  24-PPQN clock and CC control. MIDI-clock silence timeout.
- Eight original factory patches; four hardware project slots with A/B-sector
  CRC-protected storage supplied by the platform.
- Browser patch save/import/export, MIDI input, two-cycle WAV bounce,
  one-cycle MIDI export, keyboard, scope, and generation undo.
- USB MIDI, TRS MIDI input, USB audio, screen mirroring, and update support
  supplied by **fm1-base / felucca-bsp**. Bluetooth and Wi-Fi are deliberately
  left out of this first build. There is no network reporting in the application.

## What it is not

This is an independent instrument, **not Acid V running on the FM1** and not
a circuit-accurate or sonically validated clone. Arturia's proprietary circuit
model, full 14-mode distortion / 17-effect collection, preset library, drag-and-
drop DAW integration and other desktop features are not reproduced. The attached
Windows installer could not be transferred into this environment because the
upload download tool has a 32 MiB limit; it was not run or extracted. Public
documentation was used instead. See [the feature matrix](docs/FEATURES.md).

The studio and hardware emulator share the same C sound engine. The hardware
emulator additionally runs the actual application, panel mapping and LCD code
against a simulated platform. It does not emulate the processor, physical codec,
flash electrical behavior, USB timing, or CPU load. Browser local slots and
hardware slots are separate; there is currently no studio-to-device patch
transfer protocol. The hardware-view emulator edits its own simulated device.

## Build here or on Linux x86-64

```sh
git clone --recurse-submodules https://github.com/kclack/acid-fm1
cd acid-fm1
python3 -m pip install pillow fonttools
sh tools/test.sh
```

The native test compiles with `cc`, then the complete app compiles against the
platform's host simulator. The device is **JieLi AC79 / pi32v2**, not ARM.

```sh
# All downloads stay in the directory you select.
sh base/tools/get_toolchain.sh "$PWD/.toolchains/jieli"
python3 tools/fetch_sdk.py "$PWD/.toolchains/ac79-sdk"
export JIELI_TOOLCHAIN="$PWD/.toolchains/jieli/toolchain"
export AC79_SDK="$PWD/.toolchains/ac79-sdk"
python3 base/tools/build.py . --release 0.1.0
```

Outputs: `build/acid-fm1.bin`, `build/loader/ota.bin`, and
`build/acid-fm1-0.1.0.fwsc`. The three SDK packaging files are hash-checked.
The build checks RAM/flash limits, RAM-only routines, unintended MMIO, target
64-bit-multiply hazards, and absence of soft-double routines.

### Browser builds

Activate [Emscripten 4.0.20](https://github.com/emscripten-core/emsdk), then:

```sh
sh tools/build_web.sh
python3 -m http.server 8000 --directory web
# http://localhost:8000/
```

`web/engine.wasm` is compiled from `dsp/acid.c` with the studio bridge.
`web/hardware/app.wasm` is the complete app built with fm1-base's simulator.
Both use `-ffp-contract=off`. No npm install is needed to play the site.
GitHub Pages serves the checked-in `web/` artifacts through `.github/workflows/pages.yml`.

## Installation and recovery

First back up the projects and patches of whichever firmware is on your FM1.
The four slot pairs and settings area used by this application can replace
existing firmware's data. Do not install on an FM1 you cannot recover.

Use the download in the experimental release with the fm1-base CLI:

```sh
python3 -m pip install mido python-rtmidi
python3 base/tools/fm1_install.py build/acid-fm1-0.1.0.fwsc
```

Check `--help` and the base's documentation before updating. The package embeds
the base update loader; this project does **not** flash a device automatically,
bundle stock firmware, or install an alternative bootloader. If the existing
firmware does not support that update path, use its documented update method.
Recovery references: [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter)
and [the official Workbench](https://open.m-vave.com/fm1-workbench).

Hardware acceptance checklist: [docs/HARDWARE_TEST.md](docs/HARDWARE_TEST.md).

## Contributing and license

Original code: AGPL-3.0-only. Platform: MPL-2.0 file-by-file; fonts: SIL OFL; three SDK
package files: Apache-2.0. See [THIRD_PARTY.md](THIRD_PARTY.md). The full-source
release includes both pinned submodules. Bugs and contributions are welcome;
no support or performance guarantee is implied.
