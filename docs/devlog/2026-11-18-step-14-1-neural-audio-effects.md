# Step 14.1 — Offline neural audio-effect DSP reference

**Status:** Implementation prepared; local test and audio-listening evidence must be recorded separately. No neural model has been trained.

## Aim

Create a reproducible, inspectable soft-saturation reference and aligned clean/processed WAV pairs for future supervised neural-effect research. This experiment is independent of the JUCE audio callback and does not alter Step 13 functionality.

## DSP

For normalized input x[n] ∈ [-1,1] and drive g > 0:

\[s_g(x[n]) = \frac{\tanh(gx[n])}{\tanh(g)}\]

For g=0 define s_0(x)=x by continuity. Dry/wet mix m∈[0,1] and nonnegative output gain a give:

\[y[n] = a((1-m)x[n]+m s_g(x[n]))\]

For |x|≤1 and a≤1, |y|≤1. Higher output gain may exceed full scale, so WAV export rejects out-of-range results instead of silently clipping. The model is memoryless, nonlinear and may introduce harmonics above Nyquist that alias; this baseline does not claim bandlimited saturation.

## Experiment design

- WAV files use mono/stereo frame-major arrays, preserve source sample rate, and export 32-bit float PCM.
- Five deterministic fixtures: sine, harmonics, impulse, seeded noise, and silence.
- Each dry/wet pair has identical frame count and sample rate, with metadata in `manifest.json`.
- Numerical checks cover bypass, bounds, silence, channels, reproducibility, invalid parameters and roundtrip I/O.
- The manifest's relative paths make the dataset portable, but not independently immutable; future work should add content hashes and provenance.

## Candidate model for Step 14.2

Start with a tiny sample-wise MLP (input sample plus optional normalized drive; hidden tanh layers; scalar output), train with paired samples and a fixed held-out recording split. Compare against the exact tanh baseline using MAE, spectral error and listening. A memoryless MLP cannot model dynamic effects; a causal convolutional model is a later candidate. Avoid train/test leakage from adjacent windows of the same recording. Do not infer real-time suitability from offline inference speed.

## Acceptance and open work

Run `python -m pytest -q`, generate the dataset, inspect paired WAVs, and record exact Python/library versions, listening observations and test output. Still open: diverse licensed recordings, anti-aliasing comparison, trained network, ONNX export, C++ inference, latency/CPU profiling and DAW acceptance.
