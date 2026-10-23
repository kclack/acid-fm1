First experimental release of ACID FM1: an original acid instrument for the M-VAVE FM-1, inspired by the public feature set of Acid V and classic acid synthesis.

**This firmware has not been tested on a physical FM1.** Native and browser tests and a successful target build do not establish device CPU headroom, physical audio/USB operation, flashing, or recovery. Back up existing projects and confirm your recovery procedure before a hardware trial; application storage can replace another firmware's data. See HARDWARE_TEST.md and VALIDATION.md below.

- Up to 64 steps, accents, slides, polymetric lengths, probability, ratchets and cutoff locks.
- Original saw/pulse synth, saturated four-pole filter, three modulation lanes, seven distortions, delay, chorus and compact ambience.
- Eight original presets, live/arp modes, MIDI notes/CC/clock, four hardware project slots.
- Studio and panel emulators share the original C engine; the panel emulator runs the actual app and LCD code. They do not simulate device timing or CPU load.

Try the [studio](https://kclack.github.io/acid-fm1/) or [hardware controls](https://kclack.github.io/acid-fm1/hardware/), and read the [guide](https://kclack.github.io/acid-fm1/guide.html).

Files:
- `.fwsc`: experimental installation package for the supported fm1-base update path. Read the repository installation/recovery instructions first.
- `-app.bin`: raw application image, not a standalone installation package.
- `-source.tar.gz`: full source, including pinned fm1-base and felucca-bsp dependencies.
- `-browser.zip`: static emulator site; serve over localhost or HTTPS for audio/MIDI support.
- `SHA256SUMS`: SHA-256 checksums of the other attached files.

Built locally with the JieLi pi32v2 toolchain and Emscripten 4.0.20. Native engine tests passed 197,114 assertions, full-app panel checks passed, and the six-second native/WebAssembly render matched exactly. Chromium checks covered live audio, patch save/import/export, MIDI/WAV downloads and emulator startup. On-device validation remains outstanding.

Original project code: AGPL-3.0-only, preserving the repository's license choice. Platform code: MPL-2.0; fonts: OFL; SDK packaging files: Apache-2.0. Full source, credits, license texts and the build report are attached. No Arturia code, presets, samples or artwork are included; no sonic equivalence is claimed. The uploaded Windows installer could not be transferred through the environment's 32 MiB download limit, so public documentation was used instead.
