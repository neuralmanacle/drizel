# Development Roadmap

Drizel grows from a custom granular engine into a sample-aware instrument, then
into applied audio ML and embedded audio. This is the single capability roadmap:
each layer connects theory, an experiment, implementation, validation and proof
of work. Study can proceed offline while integration waits for its prerequisites.

> **Status — 8 November 2026:** Implementation exists through the Step 9 MIDI,
> interface and preset milestone (Layers 7–8), merged into `main`. Earlier DSP,
> modulation and host acceptance items remain open. Layers 9–16 are planned;
> this document does not claim completed analysis, ML or hardware deployment.

## How to read this roadmap

- **Layers** describe capabilities and dependencies. **Steps** identify historical
  delivery increments and devlogs: Step 9 is the MIDI/interface milestone, not
  Layer 9. Future delivery increments can implement part of a layer.
- **Checkboxes** distinguish verified implementation from unfinished work.
  Learner understanding, numerical tests, listening, host checks and release
  acceptance require their own evidence.
- **Study → experiment → build → validate → explain** is the flow within each
  layer. Use the [curriculum](../learnings/audio-dsp-ml-curriculum.md),
  [prerequisites](../learnings/prerequisites.md) and
  [evidence template](proof-of-work.md).
- **Historical devlogs** retain their original dates and numbering. The earlier
  prompt proposal occupied Layer 9; it now follows analysis and deployment at
  Layer 13. Current guides use the numbering below.
- **Optional layers** are research branches, not blockers for the usable core.
  A measured negative result is evidence, but does not complete an unbuilt feature.

## Layer overview

| Layer | Capability | Foundation |
|---|---|---|
| 0 | C++, mathematics and toolchain | Starting point |
| 1 | Audio data and file I/O | 0 |
| 2 | Signal primitives, filtering and resampling quality | 0–1 |
| 3 | Single grain | 1–2 |
| 4 | Grain cloud and scheduling | 3 |
| 5 | Validated real-time engine | 4 |
| 6 | General modulation | 5 |
| 7 | MIDI instrument behaviour | 5–6 |
| 8 | Interface, presets and packaging | 5–7 |
| 9 | Audio characteristics and analysis | DSP study in 0–2; offline work is independent of UI |
| 10 | Sample-aware granular playback | 5–6 and 9 |
| 11 | Audio ML and region classification | 9 dataset and 10 baseline |
| 12 | Local C++ model deployment | 10–11 |
| 13 | Editable promptable modulation | 5–6; 10 for sample-aware prompts; 12 labels optional |
| 14 | Neural audio-effect research (optional) | 2, 11–12 and measured callback headroom |
| 15 | Reduced embedded granular instrument | Portable core in 0–6 and target feasibility |
| 16 | Embedded audio inference (optional) | 11–12 and stable device audio in 15 |

Layers 0–8 build the instrument and establish its quality and reliability.
Layers 9–12 add sample understanding through DSP, then evaluated ML. Layer 13
connects that foundation to language-based control. Layers 14–16 investigate
neural effects and hardware without making them prerequisites for the instrument.

## Guiding rules

1. **Offline first, real-time second.** Render and measure before live integration.
2. **Test the maths and listen.** Record both numerical and audible results.
3. **Keep the callback bounded.** No allocation, locks, file I/O, logging, parsing
   or network work in audio processing. Background analysis never blocks playback.
4. **Separate DSP from adapters.** Keep the core independent of JUCE, ML runtimes
   and device drivers wherever practical.
5. **Write down decisions.** Explain units, alternatives, ownership and limitations.
6. **Compare before adding ML.** Keep DSP/simple learned baselines, recording-level
   data splits, held-out evaluation and a manual/DSP fallback.
7. **Measure on the target.** Report CPU, memory, observed timing and conditions;
   desktop builds and cross-compilation do not prove hardware playback.
8. **Earn progress claims with evidence.** Use the shared report format, but keep
   study, code, CI, listening, host acceptance and distribution checks separate.

---

## Layer 0: Foundations

**Goal:** A working toolchain and enough C++ and DSP knowledge to start without guessing.

**Study** (notes go in `learnings/`)
- [ ] C++ essentials: value semantics, references, RAII, `std::vector`, `std::array`, `const` correctness
- [ ] Memory and lifetime: stack vs heap, ownership, smart pointers, why allocation matters in real-time code
- [ ] Digital audio basics: sample rate, bit depth, buffers, aliasing, Nyquist
- [ ] Concurrency basics: threads, data races, `std::atomic`, memory ordering at an introductory level

**Build**
- [x] Repository layout in place (`src/`, `tests/`, `experiments/`, `learnings/`, `docs/`)
- [x] JUCE and Projucer starter project builds and opens on the target machine (maintainer report; no audio engine yet)
- [x] A test runner is chosen and automated tests pass (C++ executable registered with CTest)
- [x] Strict compiler warnings and warnings-as-errors configured for core tests and the WAV adapter
- [ ] Continuous formatting policy and automated formatting checks agreed

**Proof of work:** A clean build, ownership walkthrough and worked sample/memory calculations.

**Exit criteria**
- A clean checkout builds from the README steps with no manual fixes.
- One automated test runs from the command line or IDE.

---

## Layer 1: Audio data and file I/O

**Goal:** Get audio into memory and out again, with no synthesis yet.

- [x] Define the internal sample buffer type (channel layout, sample type, length, sample rate)
- [x] Load WAV files into the buffer (mono and stereo, 16/24-bit and 32-bit float)
- [x] Handle load failures explicitly (missing file, unsupported format, empty data)
- [x] Write a buffer back out to WAV for offline inspection
- [x] Decide and document the policy for sample-rate mismatches (preserve source rate; resample at playback later)

**Experiments**
- [x] `experiments/` program that loads a WAV, prints its properties, and writes a copy with identical decoded samples (float32 output; metadata is not copied)

**Tests**
- [x] Round trip: load, write, reload, and compare sample data
- [x] Edge cases: zero-length data is rejected and one-frame files round-trip
- [x] Excessive claimed lengths and decoded-memory limits are checked before allocation
- [x] Real-file round trip on the target Mac: stereo, 48 kHz, 2,273,072 frames (47.356 seconds)

**Study:** PCM/float audio, sample/frame/channel layout, codec ownership and validation.

**Proof of work:** WAV roundtrip comparisons, rejected-input cases and source/export listening.

**Exit criteria**
- Supported-format fixtures and the recorded real file load, write, and reload with identical decoded samples and audio properties.
- Loading never happens on the audio thread (design noted in `docs/`).

**Additional validation**
- [ ] Listen to the source and exported file on the target machine
- [ ] Measure memory use and runtime with long files near the configured limit

The initial offline milestone is complete within this scope. Extra validation
remains tracked here; this does not claim a finished sample-loading UI or engine.

---

## Layer 2: Signal primitives

**Goal:** The small, independently testable building blocks every grain will use.

### 2.1 Windows
- [x] Symmetric Hann window (offline table preparation; explicit short-length policy)
- [ ] At least one alternative (for example Tukey or triangular) behind the same interface
- [x] Precomputed Hann window tables; future grains can reuse them without per-sample cosine calls

### 2.2 Interpolation
- [x] Linear interpolation at fractional read positions
- [x] Cubic Hermite interpolation behind the same `readSample` interface, with numerical and boundary tests
- [x] Zero-padded source boundaries, including fractional blends with silence

### 2.3 Utilities
- [x] Fractional-semitone to playback-rate conversion in `GrainCloud` (`2^(semitones/12)` plus sample-rate ratio)
- [ ] Extract reusable pitch helpers, including an explicit cents interface
- [ ] Gain and decibel helpers
- [x] Seeded deterministic random source in `GrainCloud`, checked by reproducible-render tests

**Experiments**
- [ ] Render a sine wave at fractional rates with each interpolator and compare spectra to hear and see aliasing and high-frequency loss
- [ ] Compare window shapes on a repeated short grain

**Tests**
- [x] Hann: correct length, endpoints, symmetry, expected overlap-sum behaviour
- [x] Linear interpolation: exact at integer positions, correct at midpoints, safe at boundaries

Hann, linear and Hermite primitives are documented in [signal primitives](signal-primitives.md).
Pitch conversion and seeded randomness are implemented in the cloud. Alternative
windows, remaining standalone utility helpers, and listening/spectral experiments
remain open; Layer 2 is not complete.

### Quality studies and proof of work

**Study:** complex numbers, Fourier analysis, leakage, window trade-offs,
interpolation versus band-limited resampling, convolution, FIR/IIR response and
stability. Apply these to the grain reader as Layers 3–4 become available.

- [ ] Compare Hann and an alternative window on repeated grains; measure overlap
  behaviour and listen to their articulation.
- [ ] Compare linear/Hermite interpolation using tones, sweeps, impulses and
  musical material at fractional rates and different source/output rates.
- [ ] Plot spectra with a documented FFT/window method; distinguish leakage,
  interpolation error and aliasing.
- [ ] Prototype a justified anti-alias/resampling approach; measure quality,
  delay, CPU and memory. Interpolation smoothness alone is not alias rejection.
- [ ] State the supported transposition range and integrate a measured quality
  option, or record a rejected candidate as deferred rather than implemented.

**Proof of work:** reproducible A/B renders with gain treatment stated, spectral
figures and quality/cost comparisons. Re-run the relevant Layer 5 timing checks
after live integration. Layer 9 will reuse this spectral-analysis foundation.

**Exit criteria**
- Each primitive has tests and a short note in `docs/` on its trade-offs.
- Interpolators are swappable without changing grain code.

---

## Layer 3: A single grain

> Implementation and numerical checks are complete; see [single grain](single-grain.md)
> and the [Step 4 verification log](devlog/2026-10-16-step-4-single-grain.md).
> Target-machine listening remains open.

**Goal:** One grain, played correctly, offline.

- [x] Define grain state: start position, current position, playback rate, length, window, amplitude, active flag
- [x] Render one windowed grain from the sample buffer into an output buffer
- [x] Support variable playback rate (pitch) via interpolation
- [x] Handle grains that run past the end of the buffer
- [x] Add per-grain pan and gain
- [x] Ensure a grain can be reset and reused without allocation

**Experiments**
- [ ] Render the same grain at several lengths and rates to a file and listen for clicks and artefacts

**Tests**
- [x] Grain stays active for exactly its configured frame count, including silent Hann endpoints and zero-padded reads
- [x] Output starts and ends at zero with the chosen window
- [x] Rate of 1.0 reproduces the source slice (within window shaping)

**Study:** Fractional playback rate, envelope age, source boundaries and pan/gain.

**Proof of work:** Known-signal checks and clean single-grain renders at several rates/lengths.

**Exit criteria**
- A single grain renders to a WAV that sounds clean at multiple pitches.
- Grain state is small, fixed-size, and trivially copyable.

---

## Layer 4: Many grains

> Step 5 adds an offline implementation: see [grain cloud](grain-cloud.md).
> Target-machine listening and worst-case timing validation remain open.

**Goal:** A scheduler and mixer that turn single grains into a texture, still offline.

- [x] Fixed-capacity grain pool (no allocation after startup)
- [x] Grain scheduler: trigger by density (grains per second) and by interval
- [x] Timing variation (jitter) around the nominal interval
- [x] Per-grain parameter variation: position spread, pitch spread, length, pan
- [x] Overlap-add mixing of all active grains
- [x] Defined behaviour when the pool is full (drop oldest, drop new, or steal quietest)
- [x] Gain compensation so output level stays sensible as overlap changes

**Experiments**
- [x] Render generated-source textures at 5, 20, 80 and 1000 grains/sec; finite/bounded output recorded in the [Step 5 log](devlog/2026-10-19-step-5-grain-cloud.md)
- [ ] Listen to low-, medium- and high-density textures on the target machine
- [ ] Measure CPU cost per grain offline to estimate a safe maximum

**Tests**
- [x] Seeded runs are bit-for-bit reproducible within the same build
- [x] Active grain count never exceeds pool capacity
- [x] Output never exceeds a defined peak at maximum density

**Study:** Scheduling, density/overlap, seeded variation, voice capacity and gain.

**Proof of work:** Repeatable textures, bounded-voice tests and per-grain/per-block cost.

**Exit criteria**
- The engine renders a controllable granular texture to a file from a single function call.
- Maximum grain count and per-block cost are measured and written down.

---

## Layer 5: Real-time engine

> Step 6 implements the live callback, asynchronous loading, sample handoff,
> smoothed controls and output safety. See [real-time engine](realtime-engine.md).
> Host/device listening and worst-case callback timing remain open; the exit
> criteria below are not yet claimed complete.

**Goal:** The same engine running live inside an audio callback.

- [x] Wrap the engine in a JUCE audio processor (`prepareToPlay`, `processBlock`)
- [x] Handle arbitrary block sizes and sample rates
- [x] Audit `processBlock` for allocation, locks, and blocking calls
- [x] Load samples on a background thread and hand them to the audio thread safely
- [x] Expose engine parameters through thread-safe, smoothed values (atomics plus per-block or per-sample smoothing)
- [x] Add playback position control and pitch modulation as live parameters
- [x] Add denormal protection and output limiting or clipping safety

**Validation**
- [ ] Run for an extended session with no dropouts at typical buffer sizes (64-512 samples)
- [ ] Stress test at maximum grain density and small buffer sizes
- [ ] Verify the parameter-smoothing path produces no zipper noise
- [x] Automated sample-swap continuity, stale-rate rejection and concurrent handoff checks
- [ ] Confirm sample-swap behaviour by listening during host playback

### Timing and host acceptance evidence

**Study:** buffer deadlines, callback/worker ownership, profiling overhead,
sample-rate conversion, MIDI timing and reproducible host state.

- [ ] Record the primary machine, OS/compiler/JUCE revision, format and host
  versions. Exercise 44.1/48/96 kHz and 64/128/512-frame buffers where supported.
- [ ] Stress maximum live density, notes/controllers, automation, sample swaps
  and editor interaction. Export measurements outside the callback using bounded
  instrumentation; record its overhead and unsupported configurations.
- [ ] Report median, p95, p99, observed maximum, dropouts and session duration.
  Compare with `block frames / sample rate` and a declared headroom target;
  128/48000 gives about 2.67 ms for the whole callback.
- [ ] Connect the listening and state results to Layer 7 MIDI and Layer 8's
  two-host, validator and packaging checks; close only actually verified items.

**Proof of work:** timing data, conditions table, listening examples, validation
logs and reproducible failure reports. An observed maximum is not a mathematical
guarantee of worst-case execution time.

**Exit criteria**
- Drizel plays a loaded sample live with adjustable position, pitch, and density, and no audible glitches.
- Measured worst-case callback time is well under the available buffer time, with margin documented.

---

## Layer 6: Modulation system

**Goal:** A single, general way to move parameters over time, so later features plug in rather than special-case.

- [x] Per-sample bipolar LFO output and scalar-source router input
- [ ] General interchangeable source interface for LFO, envelope and random walk
- [x] LFO source (sine/triangle shape, rate, depth)
- [x] MIDI velocity attack/release amplitude envelope (Step 9 final-output gate)
- [ ] Routable envelope source for arbitrary modulation destinations
- [ ] Random-walk source for slow organic drift
- [x] Single LFO route with Off/Position/Pitch destination and amount
- [ ] General multi-source routing, including density and spread destinations
- [x] Current LFO position/pitch targets and MIDI pitch/position/density controls use the engine smoothing path (note amplitude has its own envelope)

**Tests**
- [x] Current LFO range/rate and MIDI envelope/controller behaviour tested
- [ ] Range/rate tests for future general envelope and random-walk sources
- [x] Routing with zero amount leaves a parameter unchanged

### General routing contract and proof of work

**Study:** bipolar/unipolar sources, envelopes, seeded random walks, modulation
sums, control/audio-rate updates, explicit parameter units and smoothing.

- [ ] Specify source count, route capacity, destination units and overflow before
  implementation. Name the exact spread parameter and expose it deliberately.
- [ ] Define the order of base knobs, MIDI overrides, summed routes and smoothing;
  document state migration and preserve existing sessions.
- [ ] Verify source rates, zero-depth identity, destination bounds, seeded/reset
  behaviour and block-partition independence at maximum route capacity.
- [ ] Measure routing cost and listen to transitions under Layer 5 stress cases;
  do not allocate routes or resize containers in the callback.

**Proof of work:** source plots, routing table/diagram, tests, A/B renders and
measured cost. Keep the routable envelope separate from the final MIDI gate.

**Exit criteria**
- Position, pitch, density, and spread can each be driven by any source.
- Adding a new source requires no changes to the engine.

> Layer 6 remains partial: a general source interface, routable envelopes, random
> walk and density/spread modulation routes are still open. Step 9 uses the existing
> LFO route and a dedicated MIDI amplitude envelope; it does not complete Layer 6.
>
> This layer is the attachment point for MIDI (Layer 7) and for local execution of prompt-derived policies (Layer 13).

---

## Layer 7: Instrument behaviour (MIDI)

> Step 9 combines Layers 7 and 8: see [instrument guide](step-9-instrument.md).
> MIDI and preset implementation is present; target-host listening, reproducible
> sound recall and distribution acceptance remain open.

**Goal:** Make the engine playable as an instrument.

- [x] MIDI note input mapped to pitch
- [x] Note on/off with an amplitude envelope
- [x] Decide the voicing model: one shared grain cloud, or a cloud per note
- [x] MIDI CC mapping to parameters
- [x] Pitch bend and mod wheel
- [x] Sample-accurate MIDI event timing within a block
- [x] Presets: save and recall parameter sets

**Study:** MIDI event offsets, key priority, velocity envelopes, pitch bend and CCs.

**Proof of work:** Keyboard demo, event tests and listened note/release/controller behaviour.

**Exit criteria**
- A MIDI keyboard plays the instrument with correct pitch and clean note starts and releases.
- A saved preset restores its parameter set and unchanged source reference;
  held notes, scheduler/random history and LFO phase are not serialised, so
  sample-identical performance recall is not claimed.

---

## Layer 8: Interface and packaging

> Step 8 introduced host-bound controls, waveform/base-position marker,
> active-grain telemetry, WAV drag-and-drop and output display. Step 9 extends
> this with MIDI/envelope/root controls and preset dialogs. See the
> [Step 9 guide](step-9-instrument.md). Primary-platform and host acceptance
> remain open; this layer is not complete.

**Goal:** A usable, shippable instrument.

**Interface**
- [x] Parameter controls bound to engine parameters
- [x] Waveform display with a position marker and grain activity
- [x] Sample loading by file chooser and drag-and-drop
- [x] Modulation assignment and amount controls
- [x] Output meter
- [x] Interface updates never block or allocate on the audio thread

**Packaging and testing**
- [ ] Standalone build on the primary platform
- [x] Linux Standalone and VST3 Debug builds compiled with JUCE 8.0.15
- [ ] Primary-Mac VST3/AU builds verified for Step 9
- [x] CI configured to build macOS formats, run tests and upload unsigned development artifacts (run result tracked separately)
- [ ] Test in at least two plugin hosts
- [x] Processor integration tests for preset/host-state recall, invalid preset rejection and held-note reset
- [ ] State save and restore inside a real host session
- [ ] Verify AGPL-3.0 and third-party (including JUCE) license obligations for distribution

**Study:** UI/processor lifecycle, host parameters, external sample paths and state.

**Proof of work:** Two-host project/preset recall, validator logs and installation/build evidence.

**Exit criteria**
- A new user can install a build, load a sample, and make sound without reading the source.
- The plugin passes a host validation tool and survives project save/reload.

---

## Layer 9: Audio characteristics and sample analysis

**Status:** Planned.

**Goal:** Describe a source recording through reproducible, time-aligned measurements.

**Prerequisite:** DSP foundations from Layers 0–2. Offline study can begin before host acceptance; live use waits for Layer 5.

**Study:** NumPy arrays, plotting, FFT/STFT, frame/hop sizes, phase, harmonics,
RMS, onset strength, centroid, flatness, autocorrelation and pitch confidence.

**Build and experiments**

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

**Proof of work:** runnable analysis, example map, annotated guitar case study and plots
that connect what is heard to what is measured. Initial feature scripts are not
neural networks or a trained classifier.

**Exit criteria:** a fresh run reproduces the map within declared tolerances and every
region maps back to the correct source interval. No plugin integration yet.

---

## Layer 10: Sample-aware granular playback

**Status:** Planned.

**Goal:** Let musicians bias grain selection using measured source characteristics.

**Prerequisite:** Validated engine and general modulation (Layers 5–6), plus the Layer 9 analysis reference.

**Study:** Python/C++ equivalence, immutable analysis data, weighted sampling,
background jobs, cache invalidation and confidence-aware UI.

**Build and experiments**

- [ ] Port a minimal useful feature subset to framework-independent C++; compare
  against Layer 9 using shared fixtures and stated numerical tolerances.
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

**Proof of work:** C++/Python comparison, sample-map UI demo, same-source A/B textures and
cost measurements with analysis enabled, disabled and rebuilding.

**Exit criteria:** useful sample-aware playback without inference, stale map use, callback
blocking or loss of ordinary manual controls. This becomes the ML baseline.

---

## Layer 11: Audio ML and region classification

**Status:** Planned.

**Goal:** Learn useful region labels and evaluate them against simpler baselines.

**Prerequisite:** Layer 9 dataset/feature contract and Layer 10 DSP selection baseline.

**Study:** vectors/matrices, scaling, probability, labels, gradient descent,
losses, validation/test separation, class imbalance and overfitting.

**Build and experiments**

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
- [ ] Compare the resulting grain choices with Layer 10 in listening examples;
  report if the neural model adds no practical value.

**Proof of work:** data card, model card, training/evaluation commands, baseline table,
learning curves, confusion matrix and representative error examples.

**Exit criteria:** a reproducible held-out evaluation and explicit deploy/revise/defer
verdict. A stronger metric on one small dataset does not establish generality.

---

## Layer 12: Local C++ model deployment

**Status:** Planned.

**Goal:** Use a justified audio model locally without disrupting ordinary playback.

**Prerequisite:** Evaluated model (Layer 11) and safe sample-map ownership/selection (Layer 10).

**Study:** export formats, supported operators, preprocessing contracts, floating
point differences, memory budgets, worker scheduling and failure recovery.

**Build and experiments**

- [ ] Select a runtime only after checking the actual trained architecture,
  supported operators, platform support, binary size and licence. RTNeural is a
  candidate for compatible small networks, not an automatic runtime choice.
- [ ] Validate C++ preprocessing and predictions against Python golden inputs,
  including silence, extreme values, unknown labels and malformed models.
- [ ] Load/validate weights and run region inference on a worker; publish bounded
  immutable results using Layer 10 ownership/version rules. Keep Python and
  training dependencies out of the normal plugin runtime/build.
- [ ] Use confidence/abstention and a manual/DSP fallback; demonstrate playback
  during missing weights, invalid model data, cancellation and worker overload.
- [ ] Measure end-to-end analysis latency, model size, peak worker RAM and audio
  timing under load; persist model/feature versions with relevant state.

**Proof of work:** standalone C++ inference example, Python/C++ error table, integrated
selection demo and an updated model card with target measurements.

**Exit criteria:** compatible model results improve or usefully complement the baseline,
with documented cost and reliable fallback. No training or general-purpose
inference runtime is introduced into `processBlock`.

---

## Layer 13: Promptable modulation policies (experimental)

**Goal:** Let a musician describe how a parameter should move, translate that
intent into an editable policy, and execute the accepted behaviour locally.
Model inference stays outside the audio callback.

**Status:** Planned research. **Prerequisite:** validated Layer 5 and general
Layer 6 interfaces, plus the local policy handoff/executor below. Layer 10 supplies
optional sample context; Layer 12 learned labels are not required for movement
prompts. These prerequisites are not yet complete.

**Study:** language interpretation versus audio classification, schema validation,
control units, deterministic reset/transport, local execution and lookahead.

The first target is grain scan position. Example prompts are "very chaotic",
"go around in circles", and "one step forward two step back". These map to
proposed irregular, cyclic, and directional-step patterns; their meanings and
defaults must be tested rather than assumed. See the
[promptable modulation proposal](promptable-modulation.md). No model or provider
has been selected, and the core instrument remains independent of that choice.

### 13.1 Studies (in `experiments/llm-control/`)
- [ ] Define the initial grain-scan policy vocabulary, parameter units, region bounds, and repeat behaviour
- [ ] Compare direct pattern controls with prompt interpretation using the three seed examples and paraphrases
- [ ] Test prompt formats and output schemas for validity and consistency
- [ ] Show the interpreted behaviour as a preview with editable settings
- [ ] Record failure modes (malformed output, out-of-range values, unmusical choices)
- [ ] Measure inference latency and jitter from the actual deployment location
- [ ] Compare hosted and local models for quality, latency, and licensing fit

### 13.2 Sample context

- [ ] Reference the versioned map from Layers 9–10 instead of duplicating analysis.
- [ ] Evaluate requests such as "scan sustained parts, then become brighter";
  define behaviour when there are no matching regions or confidence is low.
- [ ] Use Layer 12 classifier output only if it improves the measured task;
  ordinary movement policies must work without it.

### 13.3 Plumbing
- [ ] Versioned policy schema with supported pattern names, explicit units, bounds, timing, and random seeds
- [ ] Validate and prepare accepted policies outside the audio thread; execute only supported numeric operations
- [ ] Local execution of cyclic scans, directional steps, and seeded irregular movement
- [ ] Trajectory format: timestamped breakpoints for position, density, spread, and pitch, several seconds ahead
- [ ] Lock-free single-producer, single-consumer queue from control worker to audio thread
- [ ] Interpolation of breakpoints inside the engine as a Layer 6 modulation source
- [ ] Inference worker with timeouts and cancellation; network requests only if a hosted model is chosen
- [ ] Persist the accepted policy, schema version, edits, and seed alongside the prompt
- [ ] Invalidate stale results when the selected sample or prompt changes

### 13.4 Resilience
- [ ] Keep the current local policy running when inference is late or fails; use a documented continuation for finite trajectories
- [ ] Reject invalid policies and enforce all numeric limits before handoff to the engine
- [ ] Smooth policy changes at a defined activation point
- [ ] Clear user-facing state: interpreting, ready, or unable to interpret; preserve access to manual controls
- [ ] The instrument is fully usable with this feature disabled

**Proof of work:** manual-pattern versus prompt comparisons, prompt/policy/render
examples, validity/latency results, saved-policy replay and failure-injection demo.
Build/test manual cyclic, stepped and irregular patterns before adding a model.

**Exit criteria**
- Each seed prompt produces a valid, visible, editable policy whose offline render matches the documented behaviour.
- The same saved policy, seed, source, and render settings reproduce the same result in the tested engine build, without a new model call.
- Disabling inference or disconnecting a hosted model during playback does not interrupt audio or the active local policy.
- Malformed, late, and stale results leave the active policy intact, and applying a valid update satisfies the real-time engine's callback budget.

---

## Layer 14: Neural audio effects (optional research)

**Status:** Planned.

**Goal:** Determine whether a small neural effect adds useful sound within a measured budget.

**Prerequisite:** Layers 2 and 11–12, plus measured callback headroom from Layer 5.

**Study:** nonlinear systems, waveshaping, temporal state, causal CNN/GRU models,
spectral/time-domain losses, aliasing and sample-rate-dependent model behaviour.

**Build and experiments**

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

**Proof of work:** effect comparison, model/runtime report and measured acceptance or
rejection. Rejected deployment stays an offline experiment, not a plugin feature.

**Exit criteria:** a documented feasibility verdict; any shipped path passes its stated
real-time and sound-quality criteria. This is not required for Layers 13 or 15.

---

## Layer 15: Embedded granular instrument

**Status:** Planned.

**Goal:** Play a reduced Drizel engine on a real embedded audio device.

**Prerequisite:** Portable core in Layers 0–6, measured desktop reference and a device feasibility study. No language model or neural effect is required.

**Study:** embedded C/C++, cross compilation, memory maps, flash/RAM, codec/I²S,
DMA double buffering, interrupts, FPU/fixed point, scheduling and debugging.

**Build and experiments**

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

**Proof of work:** firmware/build instructions, wiring/pin map, memory/timing report,
desktop/device audio comparison and video of the actual device making sound.

**Exit criteria:** the documented reduced instrument runs on physical hardware within its
budgets. A desktop simulator or successful cross-build alone is not hardware proof.

---

## Layer 16: Embedded audio inference (optional)

**Status:** Planned.

**Goal:** Run useful audio inference alongside stable audio on the device.

**Prerequisite:** Evaluated/deployed model in Layers 11–12 and stable physical-device audio from Layer 15.

**Study:** integer quantisation, representative calibration data, operator
support, activation memory, feature parity and inference scheduling.

**Build and experiments**

- [ ] Choose a small classification/control task from Layer 11; budget features,
  weights, activations and runtime together. Train on a computer, infer locally.
- [ ] Compare float and quantised models on the same held-out recordings. Use
  representative training-side calibration data, never the final test set.
- [ ] Evaluate CMSIS-NN or LiteRT for Microcontrollers against the selected
  architecture/SDK; check the export route and supported operations first.
- [ ] Schedule feature extraction and inference with a bounded audio handoff;
  handle overload/low confidence using a documented DSP/manual fallback.
- [ ] Measure actual device accuracy, end-to-end latency, cycles, peak RAM/flash,
  sustained audio headroom and quality loss versus desktop results.

**Proof of work:** deployable model, reproducible firmware, quantisation comparison and
on-device demo/report. Include measurement conditions and unsupported inputs.

**Exit criteria:** model plus preprocessing fits the real device and remains useful
without disrupting audio. Buying an AI board is not completion evidence.

---

## Additional studies and scope boundaries

These are optional experiments, not prerequisites for a granular instrument:

| Study | Concepts | Suitable evidence and boundary |
|---|---|---|
| Adaptive filtering | Correlation, FIR, LMS/NLMS, convergence and stability | Simulated system identification/noise cancellation, then a measured experiment; no claim of a production AEC |
| Speech enhancement | STFT, noise estimates, spectral gains and small recurrent models | Reproduce a bounded DSP/neural comparison inspired by RNNoise; document dataset and metric limits |
| Microphone arrays | Spatial phase, delay estimation and beamforming | Controlled multi-channel recordings; requires appropriate capture hardware |
| DDSP | Differentiable oscillators/filters, losses and timbre models | Separate notebook/offline study; the current grain scheduler is not automatically differentiable |
| FPGA or NPU | Hardware parallelism, supported operators, transfer and scheduling overhead | A target-specific experiment only after profiling establishes a reason |
| Audio similarity | Feature distances, embeddings and retrieval evaluation | Region retrieval study after Layer 11; avoid claiming semantic understanding from proximity alone |

AEC removes a playback reference's echo from a microphone path; ANC uses a
physical anti-noise path; AFC addresses acoustic feedback loops. They require
different problem setups and are not interchangeable "noise removal" features.
Automotive qualification, compliance and production readiness are outside this
instrument roadmap. Study relevant role requirements when pursuing that domain.

## Dependency and next-work guidance

The overview above gives prerequisite relationships, not a requirement to finish
every research branch in numerical order. Layer 9 analysis can be studied offline;
Layer 13's basic movement policies need Layers 5–6 but not a classifier. Layer 15
can start from the portable core without Layers 11–14. Layers 14 and 16 are optional.

The next implementation work is to close the existing Layer 5/7/8 acceptance gaps,
finish Layer 6 routing and run Layer 2 quality experiments. Record that evidence
before integrating new live analysis or inference. Continue with Layer 9's sample
map, Layer 10 selection and Layer 11's baseline/model comparison.

## Checkpoints

| Milestone | Reached when |
|---|---|
| Load and verify a WAV | Initial Layer 1 scope met; recorded 10 October 2026 |
| Hear a grain and texture | Layers 3–4 listening and numerical criteria met |
| Play reliably | Layer 5 measured playback and Layer 7 MIDI acceptance met |
| Share the instrument | Layer 8 host/install/state acceptance met |
| Understand the source | Layer 9 reproducible analysis and Layer 10 selection met |
| Demonstrate audio ML | Layer 11 held-out comparison and Layer 12 C++ deployment met |
| Describe movement | Layer 13 editable, reproducible policy acceptance met |
| Demonstrate neural audio | Optional Layer 14 feasibility or integrated-effect evidence recorded |
| Play on hardware | Layer 15 real-device audio, resources and firmware evidence met |
| Run ML on hardware | Optional Layer 16 accuracy/cost and uninterrupted-audio evidence met |
