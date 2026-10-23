# Physical FM1 acceptance checklist

Status: **not performed**. The environment has no connected FM1. Host timings
are not device CPU measurements. The compiled package is experimental.

Before testing: back up current projects and patches, confirm recovery tools,
and verify the release SHA-256. Keep output volume low while checking sound.

1. Install using the update method supported by the current firmware. Record
   exact hardware revision, prior firmware, installer version, and package hash.
2. Confirm cold boot, LCD, all buttons/encoders, keyboard, volume, headphones,
   speaker/output, and USB recognition. Reboot repeatedly, then cold boot again.
3. Play all eight presets; sweep cutoff and resonance at full drive. Check for
   unstable output, clicks, missing notes, unexpected DC, or sustained overload.
4. Run the worst case: all three modulators, ratchets, noise, sub, drive, delay,
   chorus and ambience, at long and short sequence lengths. Record on-device CPU
   and xruns for at least ten minutes, including simultaneous USB audio playback
   and capture. CPU must stay below the audio deadline with adequate margin.
5. Verify slide ties versus non-ties, accents, independent lane lengths,
   probability, ratchets, direction, step editing and step recording.
6. Test live last-note priority, held notes across octave/mode/preset changes,
   MIDI note-off, CC 120/123, stop, panic, and disconnected external MIDI clock.
7. Test USB and TRS MIDI at dense message rates, start/continue/stop, and clock
   at 30/120/300 BPM. MIDI-clock swing is not applied; the incoming timing wins.
8. Save distinct projects to all four slots. Reload and cold boot (slot 1 loads
   at boot), checking that panel calibration still works and no neighboring slot
   is overwritten. Test corruption/recovery only on backed-up data.
9. Verify updating away and restoring stock using the documented recovery path.
   Do not market a recovery guarantee based on the emulator.

Only after these checks should a hardware-validated release be considered.
