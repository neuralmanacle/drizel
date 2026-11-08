# Prerequisites and learner confirmation

**Planning update: 8 November 2026.** The repository contains working code through
Step 9, but that does not verify personal mastery. The previous blanket "Not
started" labels did not distinguish implementation from study. All study items
below need learner confirmation; no skill is marked learned by this docs update.
Use the [curriculum](audio-dsp-ml-curriculum.md) and
[evidence guide](../docs/proof-of-work.md) before changing a study checkbox.

## Foundations already exercised by the project

| Learned | Topic | Existing code or evidence to explain | Demonstration required |
|---|---|---|---|
| [ ] | C++ values, arrays, structs and RAII | [SampleBuffer](../src/audio/SampleBuffer.h) | Explain ownership, copies, moves and borrowed sample pointers |
| [ ] | Sample rate, frames, bit depth and WAV formats | [Audio-data design](../docs/audio-data.md) | Calculate duration/memory and explain decode/rate policy |
| [ ] | File I/O and validation | [WAV adapter](../src/io/WavFile.cpp) | Trace success, rejection and cleanup paths |
| [ ] | Windowing and overlap | [Window](../src/audio/Window.h) | Derive a window and compare repeated-grain behaviour |
| [ ] | Interpolation and pitch ratio | [Interpolation](../src/audio/Interpolation.h) | Calculate fractional reads and explain aliasing limitations |
| [ ] | Grain scheduling and mixing | [GrainCloud](../src/audio/GrainCloud.h) | Explain density, voice capacity, randomness and gain policy |
| [ ] | Threads, atomics and memory lifetime | [RealtimeEngine](../src/audio/RealtimeEngine.h) | Trace active/pending/retired ownership and stale-rate handling |
| [ ] | LFO routing and smoothing | [Modulation](../src/audio/Modulation.h) | Explain units, ranges and the currently limited destinations |
| [ ] | MIDI notes, envelopes and controllers | [MidiInstrument](../src/audio/MidiInstrument.h) | Explain last-note priority and event timing versus audible response |
| [ ] | Plugin lifecycle, UI and state | [Step 9 guide](../docs/step-9-instrument.md) | Build/play and explain host state, missing samples and reset behaviour |

## New study linked to delivery

| Learned | Topic | Step | Required study evidence |
|---|---|---|---|
| [ ] | Profiling and callback deadlines | 10 | Measured timing distribution and buffer deadline calculation |
| [ ] | General modulation architecture | 11 | Source/route model, units, bounded work and reset examples |
| [ ] | Complex numbers, DFT/FFT and STFT | 12–13 | Known-signal spectra and frame/hop/time-resolution explanation |
| [ ] | FIR/IIR, convolution and resampling | 12 | Response/aliasing and listening comparison with cost measurements |
| [ ] | Python/NumPy/SciPy and reproducible analysis | 13 | A script that regenerates annotated feature plots |
| [ ] | Audio characteristics and pitch uncertainty | 13 | Attack/sustain/noise examples and documented failure cases |
| [ ] | Python/C++ feature parity and analysis ownership | 14 | Matching fixtures plus stale/cancelled job handling |
| [ ] | Linear algebra, derivatives and probability | 15 | Worked forward pass, loss and gradient example |
| [ ] | Dataset provenance, splits and leakage | 15 | Recording-level split manifest and training-only preprocessing |
| [ ] | Baselines, PyTorch training and evaluation | 15 | Held-out comparison, confusion matrix and error analysis |
| [ ] | Model export, C++ inference and recovery | 16 | Golden-input parity, resource costs and fallback demo |
| [ ] | Prompt interpretation and deterministic policy execution | 17 | Schema validation, replay and late/stale response experiments |
| [ ] | Causal neural audio effects | 18, optional | Offline DSP/neural comparison and deployment feasibility report |
| [ ] | Embedded C/C++, codec/I²S, DMA and memory budgets | 19 | Actual board audio, wiring and target resource report |
| [ ] | Fixed point and numerical scaling | 19 study; implementation if justified | Overflow/saturation example and float/reference comparison |
| [ ] | Quantisation and embedded neural runtimes | 20, optional | Float/quantised comparison and measured on-device inference |
| [ ] | Adaptive filtering and speech DSP | Optional side study | Controlled LMS/NLMS experiment and explanation of AEC/ANC/AFC |

Record completion dates and links to actual notes/reports when confirming a
row. Existing layer implementation status remains in [the roadmap](../docs/roadmap.md);
future deliverable acceptance remains in [the step plan](../docs/post-step-9-plan.md).
