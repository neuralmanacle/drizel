# Drizel Step 14 — Can a neural network earn its place in an audio effect?

Drizel’s grain engine is already a real-time workload. I wanted to know what a
small neural effect could add before giving it any of the audio callback’s
budget. The first target was soft saturation, a useful test because the DSP
reference is simple and its output is easy to measure.

## What the waveshaper does

A waveshaper maps each input sample to an output amplitude. With input
sample `x` in [-1, 1] and positive drive `g`, the wet output is
`tanh(g*x) / tanh(g)`. The division pins the endpoints at -1 and +1.
Near zero, increasing drive increases the slope. At larger amplitudes,
the curve flattens and rounds peaks. The dry/wet mix interpolates between
the original sample and the shaped sample, and output gain scales the
result. At zero drive the reference returns the input before applying
output gain.

The mapping is memoryless. Each channel is processed independently and
the result for a sample depends only on that sample and the controls.
There is no envelope follower, feedback loop, or time-dependent device
behavior in the target. The experiment asks how well a learned curve can
reproduce this defined behavior and what it costs to run.

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

## How the model learns the curve

Step 14.2 used a synthetic signal-kind holdout to check the training
pipeline at one setting. The final probe uses a separate model so it can
sweep drive and evaluate on held-out recordings. Drive multiplies the
input before shaping and appears again in the normalization. For hidden
unit `i`, the feature is:

```text
phi_i(x, g) = [tanh(a_i*g*x + b_i) - tanh(-a_i*g*x + b_i)] / [2*tanh(g)]
wet_neural(x, g) = sum_i(w_i * phi_i(x, g))
```

The two evaluations at opposite input signs enforce
`phi_i(-x, g) = -phi_i(x, g)` and `phi_i(0, g) = 0`. The model draws
slopes `a_i` and biases `b_i` from seeded uniform distributions and
holds them fixed. Only the output weights `w_i` are fitted. Each row of
the least-squares system contains all hidden features for one sampled
input and drive. Its target is the conventional wet output for the same
input and drive. The fit minimizes squared sample error plus
`1e-6 * sum_i(w_i^2)`, which discourages large coefficients.

Mix and output gain use the same explicit arithmetic as the reference.
Zero drive uses an identity path before output gain. At width 16, the
model stores 16 slopes, 16 biases and 16 fitted weights, or 48 float64
coefficients. The 384-byte figure covers those coefficients alone.
Temporary arrays and the NumPy runtime also consume memory.

## Recording split and level sweep

The experiment verifies each WAV against the corpus manifest before
reading it. It divides each recording by its own absolute peak, then
multiplies it by the chosen input level. This preserves the recording's
internal dynamics while making the input peak explicit.

| Split | Recordings | Input peak | Drive | Purpose |
| --- | --- | --- | --- | --- |
| Fit | Guitar E2 and tenor sax C4 | 0.2, 0.6, 1.0 | 1, 2, 4 | Fit output weights using up to 2,048 sampled frames per recording and setting. |
| Validation | Rain | 0.7 | 2.5 | Select the hidden width using mean squared error. |
| Test | Guitar chord and stereo claves | 0.08, 0.4, 0.85 | 1.5, 3, 4 | Score full recordings after selecting the model. |

The fit uses seed 14. Validation MSE was `2.41e-10` at width 8,
`1.40e-13` at width 16, and `6.04e-13` at width 32. The test
recordings played no role in choosing the width. All recording
evaluations use their original 44.1 kHz sample rate. The 48 kHz evidence
comes from the separate synthetic tone probe. The drive grid includes
two unseen values between training settings and one previously seen
value. It tests interpolation within the chosen range.

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

### Reading the waveform scores

For each case, the script compares the conventional output `y` with the
candidate output `p`. MSE is `mean((p - y)^2)`. MAE is
`mean(abs(p - y))`. Signal-to-error ratio is
`10*log10((mean(y^2) + 1e-12) / (MSE + 1e-12))`. Higher values mean
closer agreement with the reference. The stabilizing term also limits
how large the score can get as the residual approaches zero. The RMS
difference reports the level difference separately.

| Input peak | Drive | Guitar chord, dB | Stereo claves, dB |
| ---: | ---: | ---: | ---: |
| 0.08 | 1.5 | 93.71 | 76.00 |
| 0.08 | 3 | 98.77 | 81.17 |
| 0.08 | 4 | 101.10 | 83.60 |
| 0.40 | 1.5 | 106.85 | 89.75 |
| 0.40 | 3 | 110.89 | 94.46 |
| 0.40 | 4 | 112.49 | 96.55 |
| 0.85 | 1.5 | 111.97 | 95.73 |
| 0.85 | 3 | 114.42 | 99.75 |
| 0.85 | 4 | 115.39 | 101.50 |

Dry audio uses the same reference and shows how much the target changes
the input. The separate listening exports use drive 3 and input peak 0.4.
They bring all three versions of each source to a common RMS with a
peak ceiling of 0.95. The guitar triplet landed at -20.00 dBFS RMS and
the more transient claves triplet at -26.88 dBFS RMS. The metrics above
were computed before this adjustment. Level matching lets a listener
focus on the shape of the sound instead of a gain difference.

### Why the third harmonic folds back

The odd waveshaping curve generates odd harmonics. The third harmonic
of 10 kHz is 30 kHz, above Nyquist at both tested sample rates. It folds
to `abs(sample_rate - 30000)`, which gives 14.1 kHz and 18 kHz. The
probe uses a one-second, peak-0.85 tone and measures the FFT-bin
amplitude there relative to the 10 kHz carrier. The measured agreement
shows the network reproduced this alias component along with the
transfer curve. A target with oversampling or antialiasing would
require a separate comparison.

### What the block timings measure

The benchmark creates a seeded stereo block for each tested size,
runs 100 warm-up calls per processor, then times 1,000 calls with a
monotonic counter. It verifies that rendering a block in smaller chunks
produces the same samples within numerical tolerance. The report also
records the 99th-percentile and maximum observed call times.

A 256-frame block at 48 kHz arrives every 5.33 ms. That interval is the
budget for the entire callback, including the grain engine and any
other work on the audio thread. The NumPy probe measures this
standalone processing function and its array overhead on one Linux
machine. It cannot establish how much headroom Drizel has in a DAW or
how a native C++ port would compare.

## The decision

**Keep the model offline.** It closely reproduced the target, including its
aliasing, and the measured implementation took roughly 2.6–5.8 times the
median compute time of the direct DSP reference. The study found no
measured benefit that would justify adding this model to Drizel’s audio
callback. A future experiment would need a target with a behavior worth
learning, such as a repeatable recording of a nonlinear device with memory.

## Reproduce the study

Run from the repository root with Python 3.12 or a compatible Python 3
environment. The reported run used Python 3.12.14 and NumPy 2.5.5 on
x86-64 Linux.

```bash
cd experiments/neural-audio-effects
python -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
OPENBLAS_NUM_THREADS=1 python feasibility.py output/feasibility
```

`report.json` contains the split, settings, per-case metrics, alias
probe, and block timings. `model.npz` contains the three coefficient
arrays. The six WAVs let you compare the dry, conventional, and neural
renders for each held-out source. The command runs the NumPy experiment.
The earlier fixed-setting PyTorch model has its own reproduction
commands in the experiment README.

[Reproduction commands](../../experiments/neural-audio-effects/README.md#step-143--offline-feasibility-decision) · [Corpus and recording credits](../../experiments/sample-analysis/corpus/manifest.json)
