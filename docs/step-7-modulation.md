# Step 7 — First modulation source

This first increment begins roadmap Layer 6: an LFO and reusable routing path.
An LFO (low-frequency oscillator) changes a parameter over time, below audio
frequencies. A sine LFO moves smoothly; a triangle rises and falls at a constant
rate. Both yield a bipolar control signal between -1 and +1.

## Signal path

```text
Host parameters -> (position, pitch) -------+
                                           |
LFO (sine/triangle) -> depth -> destination+-> smoothed live controls -> grain cloud
```

For a signal `s` between -1 and +1 and a depth `d` between 0 and 1:

- Position target = clamp(basePosition + s * d * 0.25, 0, 1).
- Pitch target = clamp(basePitch + s * d * 12, -24, 24) semitones.
- Off or zero depth keeps the existing sound unchanged.

LFO phase advances one sample at a time: `phase += hertz / sampleRate`, wrapping
at one cycle. Oscillator state and routing live on the audio thread; each block
reads host values once. The modulation target is passed into the existing
20-ms real-time smoother. The signal never allocates, locks or calls JUCE in
the loop. Saved host parameters include modulation settings, but LFO phase
restarts when the host calls `prepareToPlay`.

## Listen

Load a WAV in the standalone/plugin, turn Play on, choose **Position** or
**Pitch** as LFO destination, raise **LFO depth** and select a rate and shape.
Position scans around the selected location; pitch bends around the selected
base semitone value. A newly triggered grain captures its playback parameters;
active grains keep their previous pitch and start location.

## Test

```sh
cmake -S . -B build -DDRIZEL_JUCE_PATH=../JUCE
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

`modulation` verifies wave phase, range behaviour, determinism across buffer
partitions, identity at depth zero and destination clamping.

## Remaining Layer 6 work

Envelope and seeded random-walk sources, routing to density and spread,
multiple simultaneous routes, smoother-response evaluation, long-session
listening and target-Mac real-time measurements are not included in this
increment. This is not the completion of Layer 6.
