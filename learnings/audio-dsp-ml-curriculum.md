# Audio DSP, ML and embedded learning curriculum

**Status: planned study, 8 November 2026.** Existing code is a practical starting
point, not proof that its author has mastered every prerequisite. Confirm study
progress separately in [prerequisites](prerequisites.md).

Follow the [post-Step-9 plan](../docs/post-step-9-plan.md). Each block combines
study, a worked experiment and an explanation tied to Drizel. Advance when the
matching acceptance criteria are met; these blocks are not a fixed-week course.

## A. Real-time C++ and audio engineering — Steps 10–11

**Study:** values/references, RAII, stack/heap and object lifetime, fixed-capacity
storage, atomics and data races, single producer/consumer ownership, sample-rate
and block-size arithmetic, parameter smoothing, modulation and MIDI event timing.

**Practice:** trace a WAV from loader to `PreparedSample`, callback and retirement.
Explain what happens while loading another sample. Measure callback distributions
and write the buffer deadline calculation for three host settings. Draw the
combination order for base parameters, MIDI, modulation and smoothing.

**Explain before marking learned:** why lock-free operations alone do not prove
real-time safety; why destruction can be expensive; how bounded queues handle
full/empty states; why scheduling a MIDI event differs from immediate audible onset.

**Evidence:** Step 10 baseline and Step 11 routing report, with code references
in `src/audio/RealtimeEngine.h`, `Modulation.h`, `MidiInstrument.h` and the processor.

## B. DSP mathematics and sound quality — Step 12

**Study:** sinusoids, radians, complex numbers, Euler's relation, linear systems,
impulses, convolution, frequency response, decibels, sampling/Nyquist, aliasing,
DFT/FFT, leakage, window functions, overlap-add, FIR/IIR and resampling.

**Practice:** plot a sine, impulse, sweep and repeated grain; compare window and
interpolator choices. Compute `rate = 2^(semitones/12) * sourceRate/outputRate`.
Show why interpolation smoothness does not by itself guarantee alias rejection.
Prototype a filter and inspect both frequency response and transient behaviour.

**Explain:** amplitude versus power dB, time versus frequency resolution, phase
versus magnitude, overlap gain, downsampling versus filtering, quality/CPU/delay.

**Evidence:** numerical checks plus measured and listened A/B comparisons. Use
synthetic references as well as guitar; a prettier spectrogram is not a metric.

## C. Audio signal characteristics and Python — Steps 13–14

**Study:** NumPy shapes/dtypes/broadcasting, SciPy signal tools, plotting, STFT
frame/hop timing, channel policies, RMS/envelopes, onsets, centroid, roll-off,
flatness, zero crossings, harmonic/percussive content and fundamental frequency.

**Practice:** annotate a guitar attack and sustained tail, a noisy texture and a
quiet passage. Explain corresponding feature tracks. Compare one C++ feature
with the Python reference using identical frame origins and padding.

**Explain:** RMS versus perceived loudness; centroid as a brightness proxy;
flatness versus semantic sound identity; pitch confidence on monophonic audio
versus chords; normalised source position versus frame index and seconds.

**Evidence:** versioned sample map, known-signal tests and a working sample-aware
selection comparison. Save settings so plots can be regenerated.

## D. ML foundations and experimental design — Step 15

**Study:** vectors, dot products and matrix multiplication; derivatives/chain
rule; distributions and conditional probability; feature scaling; regression and
classification; logistic regression; dense networks, activations, loss,
backpropagation, optimisers, regularisation, class balance and early stopping.

**Practice:** implement a tiny forward pass by hand, calculate one simple loss,
then train a baseline and small PyTorch classifier on the region task. Plot
learning curves and inspect the recordings behind misclassifications.

**Explain:** train/validation/test roles; why adjacent audio crops can leak across
splits; why preprocessing is fit only on training data; precision/recall/F1;
why a confident prediction can be wrong; why a model can lose to a simple rule.

**Evidence:** data card, grouped splits, reproducible training, baseline comparison
and final held-out report. Learn CNNs on spectrograms and GRU/LSTM temporal models
only when a measured limitation motivates their additional complexity.

## E. Local inference and hybrid DSP/ML — Steps 16–18

**Study:** preprocessing/export contracts, runtime/operator compatibility,
causality, streaming state, feature-rate versus control-rate versus audio-rate
execution, model resets, numerical parity, CPU/RAM, failure recovery and versioning.

**Practice:** run the same golden features in Python and C++, compare predictions,
and simulate missing or delayed results during playback. For Step 17, manually
execute a policy before introducing a language interpreter. For optional Step 18,
compare a small neural effect with a conventional DSP baseline offline first.

**Explain:** training versus inference; audio classification versus language
interpretation; why background analysis can tolerate latency while a synchronous
effect must meet each audio deadline; why model output needs explicit validation.

**Research reading:** RNNoise demonstrates hybrid DSP/neural speech enhancement;
DDSP combines differentiable DSP components and learning. Neither is a drop-in
solution for Drizel, and the current grain scheduler is not a differentiable model.

**Evidence:** export/parity report, integrated fallback demo and measured runtime.
A language-model call is not evidence of training an audio model.

## F. Embedded audio and edge ML — Steps 19–20

**Study:** C/C++ cross compilation and linking, processor architecture, flash/RAM,
stack/heap, interrupts, DMA, I²S/codecs, double buffering, cache coherency, FPU,
fixed-point scaling/saturation, profiling and basic RTOS scheduling.
Then study weight/activation quantisation, representative calibration data,
operator support and runtime memory planning.

**Practice:** budget a source sample and grain pool before selecting a board;
bring up audio first, port a reduced core, compare with a desktop render, then
schedule a tiny classifier alongside audio. Compare float/quantised predictions.

**Explain:** why a dedicated AI accelerator is optional; feature extraction and
activations in addition to weights; interrupt timing versus background work;
why a simulator/cross-build does not prove that physical audio works.

**Evidence:** firmware, wiring/build instructions, actual hardware demo, resource
report and desktop/device comparison. Learn FPGA/HDL and NPU-specific tooling
only as a later target-driven specialisation.

## G. Optional adaptive and speech DSP studies

For roles involving Arun's areas of expertise, study correlation, adaptive FIR,
LMS/NLMS and convergence, then distinguish acoustic echo cancellation, active
noise cancellation and acoustic feedback cancellation. Prototype system
identification in simulation before pursuing a live acoustic loop. Beamforming,
wake-word detection and speech enhancement are separate possible experiments.
They are not mandatory features for a granular instrument.

## Study session format

1. State one question and read one focused reference section.
2. Derive a small numeric example with units and predict what will happen.
3. Run an experiment and compare prediction, measurement and listening.
4. Explain the relevant Drizel code and one failure case in original words.
5. Save the result using the [evidence template](../docs/proof-of-work.md).

## Primary references

Use these for the matching study block; choose and record dependency versions
when implementing, rather than assuming a tutorial's environment is current.

- [JUCE documentation](https://juce.com/learn/documentation/) — framework and plugin APIs.
- [Sean Luke, Computational Music Synthesis](https://people.cs.gmu.edu/~sean/book/synthesis/) — synthesis foundations.
- [SciPy signal documentation](https://docs.scipy.org/doc/scipy/reference/signal.html) — analysis/filtering reference.
- [librosa feature documentation](https://librosa.org/doc/0.11.0/feature.html) — a versioned Python feature reference.
- [PyTorch Learn the Basics](https://docs.pytorch.org/tutorials/beginner/basics/intro.html) — tensors, models and optimisation.
- [scikit-learn common pitfalls](https://scikit-learn.org/stable/common_pitfalls.html) — preprocessing and leakage.
- [RTNeural](https://github.com/jatinchowdhury18/RTNeural) — candidate small-network C++ inference library.
- [RNNoise paper](https://arxiv.org/abs/1709.08243) — hybrid DSP/deep-learning speech enhancement.
- [DDSP introduction](https://magenta.tensorflow.org/ddsp) — differentiable DSP research.
- [CMSIS-DSP](https://arm-software.github.io/CMSIS-DSP/latest/) — Arm DSP functions.
- [CMSIS-NN](https://arm-software.github.io/CMSIS-NN/latest/) — Cortex-M neural kernels.
- [LiteRT for Microcontrollers](https://developers.google.com/edge/litert/microcontrollers/overview) — a candidate microcontroller inference runtime.
