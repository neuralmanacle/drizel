# After Step 9: audio DSP, ML and embedded proof of work

**Planning revision: 8 November 2026. All Steps 10–20 are planned, not implemented.**
Step 9 is merged into `main`; its host, listening and release acceptance items
remain open. This plan adds a learning and delivery sequence without rewriting
that implementation history or marking study as complete.

Drizel remains a playable granular instrument. Each addition must improve its
sound, control, reliability or portability, or answer a documented experiment.
The engineering progression is audio software → deeper DSP → audio analysis →
applied audio ML → C++ deployment → embedded audio and edge ML.

## Navigation and numbering

- [Existing implementation layers and status](roadmap.md)
- [Learning curriculum](../learnings/audio-dsp-ml-curriculum.md)
- [Prerequisite self-assessment](../learnings/prerequisites.md)
- [Evidence requirements and report template](proof-of-work.md)
- [Experiment organisation](../experiments/README.md)

**Steps** are deliverable increments; **layers** are architectural capabilities.
Step 9 is not Layer 9. Preserve Layers 0–9 and their original status; the new
steps close earlier gaps and then extend the instrument. A larger step can be
split into smaller PRs while retaining its evidence and acceptance checklist.
There are no fixed completion dates or promises of mastery.

| Step | Deliverable | Prerequisite | Existing layer or extension |
|---|---|---|---|
| 10 | Measured, validated Step 9 baseline | Step 9 implementation | Close validation gaps in Layers 2–5 and 7–8 |
| 11 | General modulation sources and routing | Step 10 playback/performance baseline | Complete Layer 6 scope |
| 12 | Measured grain and transposition quality | Step 10; integrate after Step 11 | Close Layer 2 gaps and extend DSP quality |
| 13 | Python sample-analysis reference | Step 12 DSP study; offline study can start earlier | Begin Layer 9.2 without a language model |
| 14 | Sample-aware granular playback in C++ | Steps 11–13 | Analysis and selection extension |
| 15 | Evaluated audio-region classifier | Step 13 dataset; Step 14 supplies DSP baseline | Audio ML experiment |
| 16 | Local C++ model deployment | Steps 14–15 | Audio ML integration |
| 17 | Editable promptable modulation | Steps 10–11; Step 14 for sample-aware prompts | Layer 9; Step 16 classification is optional |
| 18 | Small neural audio-effect study (optional) | Steps 12, 15–16 | Audio-rate inference study |
| 19 | Reduced embedded granular instrument | Steps 10–12 and target feasibility study | Embedded audio extension; no ML required |
| 20 | Embedded audio inference (optional) | Steps 15–16 and 19 | Edge ML deployment |

Steps 18 and 20 are optional specialisations. Step 19 does not depend on prompts
or a neural effect. Offline study can proceed while target-machine checks are
pending; live integration waits for the relevant reliability and ownership gates.

## Shared completion contract

Every step needs an explainable hypothesis, a runnable experiment, an appropriate
comparison, measured results, a listening example when sound changes, and a short
technical report. Use [the evidence template](proof-of-work.md). Record negative
results too; rejecting an unsuitable model is valid research evidence, but does
not complete a proposed integration that was never built.

- [ ] Study: explain the concepts and the chosen trade-off in original notes.
- [ ] Experiment: reproduce the result from documented commands and inputs.
- [ ] Implementation: integrate only the parts justified by the experiment.
- [ ] Validation: record actual checks, failures and target limitations.
- [ ] Evidence: link code, report, measurements and demo to an exact commit.

The checkboxes in each step below are independent acceptance items. Do not mark
a whole step complete because a blog, demo, build or unit test exists alone.

## Step 10 — Validate and measure the playable instrument

**Learn:** buffer deadlines, callback versus UI/worker ownership, MIDI timing,
profiling overhead, sample-rate conversion, host state and reproducibility.

**Build and investigate**

- [ ] Reproduce the Step 9 primary-Mac build and test Standalone plus two named
  plugin hosts, recording OS, compiler, JUCE revision, format and host versions.
- [ ] Exercise 44.1/48/96 kHz and 64/128/512-frame buffers; record unsupported
  combinations explicitly. Include silence, rapid notes, maximum live density,
  overlapping held notes, sample loading, automation and editor interaction.
- [ ] Check note/release behaviour, CCs, bend, panic, sample-swap continuity,
  smoothing, preset recall and host project save/reload by listening.
- [ ] Measure callback timing with bounded instrumentation; export/format data
  outside the callback. Report median, p95, p99, observed maximum, dropout count,
  session duration and measurement overhead, rather than average CPU alone.
- [ ] Compare the observed maximum to `block frames / sample rate`; document a
  chosen headroom target before evaluating it. At 128/48000, the interval is
  about 2.67 ms. This is the whole callback interval, not an ML-only allowance.
- [ ] Run the applicable plugin validators, retain results, and close or track
  the outstanding packaging checks from the Step 9 guide.
- [ ] Render/listen to the outstanding single-grain and cloud examples from
  Layers 2–4; retain their limitations for the Step 12 comparison.

**Proof:** baseline report, machine/settings table, timing data, validator logs,
short keyboard demo and reproducible bug reports for unresolved failures.

**Exit:** a repeatable measured baseline with the targeted acceptance failures
resolved. Mark only the original roadmap items actually verified. An observed
maximum is not a mathematical guarantee of worst-case execution time.

## Step 11 — Complete general modulation

**Learn:** bipolar/unipolar signals, envelopes, seeded random walks, parameter
units, modulation sums, smoothing and control-rate versus audio-rate updates.

- [ ] Define a bounded source/route interface with documented source count,
  route capacity, destination units and overflow behaviour before coding it.
- [ ] Make LFO, a routable envelope and seeded random walk usable as sources;
  distinguish this envelope from Step 9's final MIDI amplitude gate.
- [ ] Support position, pitch, density and spread destinations. Name the exact
  spread parameter and units; expose missing engine controls deliberately.
- [ ] Define how base knobs, MIDI overrides, multiple routes and smoothing
  combine; preserve existing sessions and document any state migration.
- [ ] Verify zero-depth identity, destination bounds, source rates, repeatable
  seeds, block-partition behaviour and bounded execution at maximum routes.
- [ ] Listen to transitions and repeat Step 10 stress scenarios after integration.

**Proof:** routing diagram/table, source plots, before/after WAVs, focused tests
and measured routing cost.

**Exit:** the Layer 6 exit criteria are met; a new source uses the agreed
interface without special cases in grain rendering. No dynamic route allocation
or container resizing occurs in the callback.

## Step 12 — Grain quality, filtering and resampling

**Learn:** Fourier analysis, leakage, window trade-offs, interpolation versus
band-limited resampling, aliasing, convolution, FIR/IIR response and stability.

- [ ] Compare Hann with at least one alternative window on repeated grains;
  measure endpoints, overlap behaviour and perceived articulation.
- [ ] Compare linear and Hermite interpolation using tones, sweeps, impulses
  and musical material at fractional rates and different source/output rates.
- [ ] Plot spectra and measure unwanted components with a documented FFT/window
  method; distinguish leakage, interpolation error and aliasing.
- [ ] Prototype an appropriate anti-alias/resampling approach, such as a
  band-limited reader or prefiltered source levels. Record quality, delay,
  memory and CPU trade-offs; changing interpolator alone is not proof of an
  anti-alias solution. Define treatment of reverse playback if introduced.
- [ ] Extract reusable pitch/cents and gain/dB helpers with explicit units and
  finish the applicable Layer 2 items without inventing redundant utilities.
- [ ] Integrate a justified quality option or document why a candidate is
  deferred; measure it at the supported pitch/density extremes.

**Proof:** aligned A/B renders with gain treatment stated, spectral figures,
quality/cost comparison and a written supported transposition range.

**Exit:** the original primitives and listening gaps are closed, and the chosen
quality policy is supported by measurements. A rejected anti-alias candidate
must remain recorded as deferred, not described as implemented.

## Step 13 — Characterise audio in Python

**Learn:** NumPy arrays, plotting, FFT/STFT, frame/hop sizes, phase, harmonics,
RMS, onset strength, centroid, flatness, autocorrelation and pitch confidence.

- [ ] Establish a small reproducible Python environment with recorded versions
  and a CLI or script entry point; a notebook may supplement that entry point.
- [ ] Assemble a rights-documented set of guitar, saxophone/voice where available,
  percussion, noise, silence and field recordings. Include synthetic fixtures.
- [ ] Generate waveform/spectrogram plots and time-aligned RMS, onset, centroid
  and flatness tracks; study pitch only for suitable monophonic regions.
- [ ] Specify channel/downmix policy, source sample rate, FFT/window/hop,
  centering/padding, feature units, time origin and silence handling. RMS is an
  energy measure; perceptual loudness needs a separately defined method.
- [ ] Export a versioned map with source identity/hash, analysis settings,
  timestamps or frame indices, feature values and unavailable/confidence flags.
- [ ] Compare measurements against annotated events and known tones/impulses;
  explain failures on chords, noise and quiet passages.

**Proof:** runnable analysis, example map, annotated guitar case study and plots
that connect what is heard to what is measured. Initial feature scripts are not
neural networks or a trained classifier.

**Exit:** a fresh run reproduces the map within declared tolerances and every
region maps back to the correct source interval. No plugin integration yet.

## Step 14 — Make grain selection sample-aware

**Learn:** Python/C++ equivalence, immutable analysis data, weighted sampling,
background jobs, cache invalidation and confidence-aware UI.

- [ ] Port a minimal useful feature subset to framework-independent C++; compare
  against Step 13 using shared fixtures and stated numerical tolerances.
- [ ] Prepare analysis on a worker. Keep source ownership valid for the job,
  reject stale results after sample/rate/settings changes and reclaim retired
  maps outside the callback, including cancellation and shutdown paths.
- [ ] Precompute bounded region/candidate tables outside playback; avoid scanning
  a whole sample or rebuilding a probability distribution for every grain.
- [ ] Add attack/sustain preference, silence avoidance and brightness selection
  incrementally. Define empty-candidate and low-confidence fallback to manual
  selection, seed/reset behaviour and the selected-region boundary policy.
- [ ] Keep feature-based selection distinct from claims of semantic recognition.
  Pitch/root suggestions, if explored, need confidence and manual override.
- [ ] Version analysis/selection state and define cache regeneration on recall;
  recheck host-state, loading, concurrent handoff and callback timing.

**Proof:** C++/Python comparison, sample-map UI demo, same-source A/B textures and
cost measurements with analysis enabled, disabled and rebuilding.

**Exit:** useful sample-aware playback without inference, stale map use, callback
blocking or loss of ordinary manual controls. This becomes the ML baseline.

## Step 15 — Train and evaluate one small audio model

**Learn:** vectors/matrices, scaling, probability, labels, gradient descent,
losses, validation/test separation, class imbalance and overfitting.

- [ ] Define a region-labelling guide for transient, sustained tonal and
  noise-like material, including ambiguous/mixed regions and abstention.
  Labels are operational categories, not universal truths about timbre.
- [ ] Freeze a provenance/licence manifest and group splits by original recording
  and recording session where applicable. Keep crops/augmentations of the same
  source in one split; fit scaling and feature selection on training data only.
- [ ] Compare majority/rule-based and a simple learned baseline (for example
  logistic regression) before a small PyTorch dense network on the same inputs.
- [ ] Choose metrics and acceptable regression/cost limits before the final test.
  Report per-class precision/recall/F1, macro-F1, confusion matrix and per-source
  failures; reserve the test set for final evaluation, not model selection.
- [ ] Record seeds, hyperparameters, feature schema, normalisation, data splits,
  model weights/hash, inference cost and uncertainty behaviour. Do not claim
  calibrated confidence from raw softmax scores alone.
- [ ] Compare the resulting grain choices with Step 14 in listening examples;
  report if the neural model adds no practical value.

**Proof:** data card, model card, training/evaluation commands, baseline table,
learning curves, confusion matrix and representative error examples.

**Exit:** a reproducible held-out evaluation and explicit deploy/revise/defer
verdict. A stronger metric on one small dataset does not establish generality.

## Step 16 — Deploy the audio model in local C++

**Learn:** export formats, supported operators, preprocessing contracts, floating
point differences, memory budgets, worker scheduling and failure recovery.

- [ ] Select a runtime only after checking the actual trained architecture,
  supported operators, platform support, binary size and licence. RTNeural is a
  candidate for compatible small networks, not an automatic runtime choice.
- [ ] Validate C++ preprocessing and predictions against Python golden inputs,
  including silence, extreme values, unknown labels and malformed models.
- [ ] Load/validate weights and run region inference on a worker; publish bounded
  immutable results using Step 14 ownership/version rules. Keep Python and
  training dependencies out of the normal plugin runtime/build.
- [ ] Use confidence/abstention and a manual/DSP fallback; demonstrate playback
  during missing weights, invalid model data, cancellation and worker overload.
- [ ] Measure end-to-end analysis latency, model size, peak worker RAM and audio
  timing under load; persist model/feature versions with relevant state.

**Proof:** standalone C++ inference example, Python/C++ error table, integrated
selection demo and an updated model card with target measurements.

**Exit:** compatible model results improve or usefully complement the baseline,
with documented cost and reliable fallback. No training or general-purpose
inference runtime is introduced into `processBlock`.

## Step 17 — Add editable promptable modulation

**Learn:** language interpretation versus audio classification, schema validation,
local policy execution, lookahead, deterministic resets and user evaluation.

- [ ] Build and test manual cyclic, stepped and seeded irregular policies first;
  define units, bounds, reset/transport behaviour and safe transition timing.
- [ ] Implement the validated worker-to-engine handoff and local executor from
  the [prompt proposal](promptable-modulation.md). Inference never runs in the
  audio callback; the active policy survives a late or failed response.
- [ ] Compare direct controls with prompt interpretation on the three existing
  seed prompts and paraphrases. Choose a model/provider only after evaluation.
- [ ] Show and edit the interpreted policy before applying it; reject malformed,
  out-of-range, stale and cancelled responses without changing active controls.
- [ ] Use Step 14 analysis for optional sample-aware requests such as "scan the
  sustained parts, then become brighter". Step 16 labels are optional context,
  not a prerequisite for ordinary movement prompts.
- [ ] Save the numeric policy, schema, seed, source identity and edits. Verify
  restoration without another inference call and document render determinism.

**Proof:** prompt/policy/render examples, validity and latency report, failure
injection demo, saved-policy replay and a comparison with manual controls.

**Exit:** satisfy Layer 9 acceptance criteria. Keep model/feature quality claims
separate from the deterministic behaviour of the local executor.

## Step 18 — Study a neural audio effect (optional)

**Learn:** nonlinear systems, waveshaping, temporal state, causal CNN/GRU models,
spectral/time-domain losses, aliasing and sample-rate-dependent model behaviour.

- [ ] Start with a small causal saturation/effect model and an explicit target,
  dataset and simple DSP baseline; avoid making general timbre transfer the
  first scope. Use held-out source recordings and level ranges.
- [ ] Compare dry, conventional DSP and neural outputs at matched declared
  levels; report error, listening results, sample-rate assumptions and latency.
- [ ] Evaluate offline first. Only consider synchronous audio-rate inference
  after proving fixed/bounded memory and execution, supported block/rate cases,
  reset behaviour and callback headroom. Model loading remains off-thread.
- [ ] If using an asynchronous audio path, specify buffering, added latency,
  underrun behaviour and host latency compensation before integration.
- [ ] Add safe bypass and repeat the host stress suite if an effect is integrated.

**Proof:** effect comparison, model/runtime report and measured acceptance or
rejection. Rejected deployment stays an offline experiment, not a plugin feature.

**Exit:** a documented feasibility verdict; any shipped path passes its stated
real-time and sound-quality criteria. This is not required for Steps 17 or 19.

## Step 19 — Port a reduced granular engine to embedded audio

**Learn:** embedded C/C++, cross compilation, memory maps, flash/RAM, codec/I²S,
DMA double buffering, interrupts, FPU/fixed point, scheduling and debugging.

- [ ] Estimate source storage, grain/window state, stack, audio buffers and CPU
  before choosing hardware. Record board, processor, codec, toolchain, costs
  and SDK licences; FPGA/NPU support is not required for the first port.
- [ ] Start with codec loopback/test tone and dropout instrumentation, then port
  a minimal core without JUCE, desktop file/UI dependencies or ML.
- [ ] Use a bounded sample and reduced grain count; define storage access and
  what must be resident in RAM. Keep slow storage and control I/O off the audio
  deadline path; account for DMA/cache coherency on the chosen processor.
- [ ] Compare output with desktop golden renders within justified tolerances;
  document float/fixed-point saturation and scaling if fixed point is used.
- [ ] Add a few physical controls or MIDI only after stable audio; measure cycles
  per block, observed maximum, headroom, RAM/flash and extended-run dropouts.

**Proof:** firmware/build instructions, wiring/pin map, memory/timing report,
desktop/device audio comparison and video of the actual device making sound.

**Exit:** the documented reduced instrument runs on physical hardware within its
budgets. A desktop simulator or successful cross-build alone is not hardware proof.

## Step 20 — Deploy a tiny audio model on the device (optional)

**Learn:** integer quantisation, representative calibration data, operator
support, activation memory, feature parity and inference scheduling.

- [ ] Choose a small classification/control task from Step 15; budget features,
  weights, activations and runtime together. Train on a computer, infer locally.
- [ ] Compare float and quantised models on the same held-out recordings. Use
  representative training-side calibration data, never the final test set.
- [ ] Evaluate CMSIS-NN or LiteRT for Microcontrollers against the selected
  architecture/SDK; check the export route and supported operations first.
- [ ] Schedule feature extraction and inference with a bounded audio handoff;
  handle overload/low confidence using a documented DSP/manual fallback.
- [ ] Measure actual device accuracy, end-to-end latency, cycles, peak RAM/flash,
  sustained audio headroom and quality loss versus desktop results.

**Proof:** deployable model, reproducible firmware, quantisation comparison and
on-device demo/report. Include measurement conditions and unsupported inputs.

**Exit:** model plus preprocessing fits the real device and remains useful
without disrupting audio. Buying an AI board is not completion evidence.

## Additional studies and scope boundaries

These are optional experiments, not prerequisites for a granular instrument:

| Study | Concepts | Suitable evidence and boundary |
|---|---|---|
| Adaptive filtering | Correlation, FIR, LMS/NLMS, convergence and stability | Simulated system identification/noise cancellation, then a measured experiment; no claim of a production AEC |
| Speech enhancement | STFT, noise estimates, spectral gains and small recurrent models | Reproduce a bounded DSP/neural comparison inspired by RNNoise; document dataset and metric limits |
| Microphone arrays | Spatial phase, delay estimation and beamforming | Controlled multi-channel recordings; requires appropriate capture hardware |
| DDSP | Differentiable oscillators/filters, losses and timbre models | Separate notebook/offline study; the current grain scheduler is not automatically differentiable |
| FPGA or NPU | Hardware parallelism, supported operators, transfer and scheduling overhead | A target-specific experiment only after profiling establishes a reason |
| Audio similarity | Feature distances, embeddings and retrieval evaluation | Region retrieval study after Step 15; avoid claiming semantic understanding from proximity alone |

AEC removes a playback reference's echo from a microphone path; ANC uses a
physical anti-noise path; AFC addresses acoustic feedback loops. They require
different problem setups and are not interchangeable "noise removal" features.
Automotive qualification, compliance and production readiness are outside this
instrument roadmap. Study relevant role requirements when pursuing that domain.

## Start here

Begin **Step 10**, using the acceptance checklist in the [Step 9 guide](step-9-instrument.md).
Record a reproducible primary-machine baseline before changing audio behaviour.
Use the curriculum for the matching study block, and attach evidence as each
step is completed. Consult the [proof-of-work guide](proof-of-work.md) before
claiming a new DSP, ML or embedded capability in the README or portfolio.
