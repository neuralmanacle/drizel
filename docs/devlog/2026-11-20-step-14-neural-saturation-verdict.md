# Drizel Step 14 — Can a neural network earn its place in an audio effect?

Drizel’s grain engine is already a real-time workload. I wanted to know what a
small neural effect could add before giving it any of the audio callback’s
budget. The first target was soft saturation, a useful test because the DSP
reference is simple and its output is easy to measure.

## The experiment

Step 14.1 generated dry and saturated WAV pairs with a normalized tanh
waveshaper. Step 14.2 added a sample-wise PyTorch model for a fixed setting.
For the final feasibility probe, I used a separate NumPy model that takes
drive into account. It applies tanh hidden units to the driven sample and
fits the output weights by regularized least squares. The hidden features are
odd-symmetric, which keeps a silent input silent. The model selected on the
rain recording uses 16 hidden units and 384 bytes of float64 coefficients.

I fitted the model on guitar E2 and tenor sax, then evaluated it on an
independent guitar chord and stereo claves recording. Each test recording ran
at input peaks of 0.08, 0.4, and 0.85 and drive values of 1.5, 3, and 4. The
18 cases used full wet mix and unity output gain. For listening, the script
exports dry, DSP and neural WAVs at the same RMS within each source triplet.
A listening session has yet to be logged. The numeric scores below use the
original signal levels.

## What the measurements showed

The neural model scored **76.0–115.4 dB signal-to-error ratio** against the tanh
reference across the held-out grid. Dry audio scored **2.5–10.7 dB** against that
same reference. The largest neural-to-reference RMS difference was below
**0.00005 dB**. The small corpus and reference-derived labels support a claim
about reproducing this curve on these recordings. A different effect or a
physical device would need its own evaluation.

A 10 kHz tone exposed the same limitation in both paths. With drive 3, the
folded third harmonic appeared at 14.1 kHz at 44.1 kHz and at 18 kHz at 48 kHz.
It landed near **−13.56 dBc** for both the conventional and neural paths.
Neither experiment uses oversampling. The model has zero samples of
algorithmic delay because every output depends only on its corresponding
input sample. Plugin and host latency would need separate measurement.

| Stereo block | Conventional median | Neural median |
| --- | ---: | ---: |
| 64 frames | 9.2 µs | 24.3 µs |
| 128 frames | 10.0 µs | 38.4 µs |
| 256 frames | 11.2 µs | 65.7 µs |

These are 1,000 timed calls per case after warm-up in one single-threaded
NumPy process on Linux. They measure offline block compute. DAW callback
headroom, allocator behavior, and performance of a native implementation
remain unmeasured.

## The decision

**Keep the model offline.** It closely reproduced the target, including its
aliasing, and the measured implementation took roughly 2.6–5.8 times the
median compute time of the direct DSP reference. The study found no
measured benefit that would justify adding this model to Drizel’s audio
callback. A future experiment would need a target with a behavior worth
learning, such as a repeatable recording of a nonlinear device with memory.

[Reproduction commands](../../experiments/neural-audio-effects/README.md#step-143--offline-feasibility-decision) · [Corpus and recording credits](../../experiments/sample-analysis/corpus/manifest.json)
