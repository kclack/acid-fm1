# Validation record

Built locally in the managed Linux environment on 2026-10-23.
No physical M-VAVE FM1 was connected or flashed.

## Completed

- Native C engine tests: every factory preset renders audible, finite and
  bounded output. Tests cover sample-block-size independence, deterministic
  seeded playback, swing timing, MIDI clock timeout, live note priority,
  arpeggiator note release, ratchets, invalid projects, parameter bounds,
  mutation, and adversarial valid combinations. Every distortion mode is
  silent at zero input. AddressSanitizer and UndefinedBehaviorSanitizer passed.
  Leak detection was disabled because this execution environment uses ptrace;
  the engine does not allocate memory dynamically.
- Full firmware application in fm1-base's native simulator: panel navigation,
  play/stop, editing, save, mode changes, note on/off, and LCD renders.
- Emscripten 4.0.20: both the standalone studio engine and full application
  emulator compile. The six-second native/WebAssembly comparison produced
  identical float samples in this environment (maximum error zero).
- Chromium: live AudioWorklet output, patch save/recall, generation/undo,
  project import/export, MIDI and WAV downloads, modulation editing, desktop
  and mobile layout, guide, and hardware-view emulator boot.
- Browser test observed zero application requests to third-party origins.
  Real Web MIDI hardware permission/communication was not tested; browser
  speech depends on OS/browser voice availability.
- JieLi pi32v2 toolchain, clang 4.0.1, r3 single-precision FPU target: actual
  `.bin`, OTA loader, and `.fwsc` package compile and link. The platform's
  checks for image/RAM budgets, RAM-only code, soft-double helpers, inline
  64-bit multiply hazards, and unintended MMIO pass.

Representative linked budget: app 91,256 bytes; `.data + .bss` 29,336 bytes
of 98,304; pool 233,800 bytes of 344,064. Exact values and hashes belong to
the particular release package. Host render speed is not a device CPU claim.

## Not validated

- Flashing, recovery, physical audio output, on-device CPU/xruns/latency.
- Real USB audio transfers, USB/TRS MIDI timing and disconnect behavior.
- Real flash power-loss behavior and compatibility with existing saved data.
- Sound equivalence to Acid V or a component-level TB-303 circuit model.
- Browser compatibility beyond the tested Chromium build.

The firmware therefore remains experimental; see HARDWARE_TEST.md before a
physical trial. The studio is useful for musical and UI testing but does not
replace that validation.
