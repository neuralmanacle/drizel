# 24 October 2026 — Step 7: first modulation source

Added a framework-independent bipolar LFO with sine and triangle waveforms,
sample-rate-aware phase advancement, rate limiting (0.05–10 Hz), and a simple
single-destination modulation router. The LFO modulates normalized grain-start
position (up to ±0.25) or pitch (up to ±12 semitones) at configurable depth.
Modulation is off by default, preserving existing playback behaviour.

Wired four host parameters through JUCE APVTS: LFO rate, depth, waveform, and
destination. The processor snapshots settings per block and generates a control
value per sample; targets continue through Step 6's 20 ms smoothing path.
The hot path adds no allocations, locks, file I/O, or thread creation.
Registered the new header, test source, and guide in `drizel.jucer`; the
test source is non-compiling in Projucer and instead builds with CMake.

Added `tests/ModulationTests.cpp` and CTest entry `modulation` for oscillator
phase/shape, deterministic progression, zero-depth identity, disabled routing,
and output bounds. See [Step 7 guide](../step-7-modulation.md) for equations,
routing and listening instructions.

**Validation status:** Tests and plugin builds have not yet been executed on the
target Mac. Remaining Layer 6 work includes envelope and seeded random-walk
sources, routing to density and spread, multiple routes, and listening/callback
performance measurements. This milestone is the first increment of Layer 6,
not its full completion.
