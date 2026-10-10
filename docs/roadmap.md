# Development Roadmap

This roadmap builds Drizel from the bottom up. Each layer depends only on the layers beneath it, and each ends with a concrete, testable result before the next begins. Nothing in a higher layer is started until the exit criteria of the layer below are met.

> **Status — 10 October 2026:** The first offline WAV milestone is reached:
> the maintainer reported passing CMake/CTest checks on macOS and an exact
> decoded-sample round trip of a real stereo file. The starter plugin also runs;
> its editor still shows "Hello World!". Listening and large-file stress checks
> remain open. See the [development log](devlog/2026-10-10.md) and
> [audio-data design](audio-data.md). Promptable modulation is a documented
> research direction, with implementation still planned.

## How to read this roadmap

- **Layers** are ordered by dependency, not by difficulty or visibility. The user interface sits near the top because it needs everything beneath it.
- **Exit criteria** define "done" for each layer. If they cannot be checked, the layer is not finished.
- **Experiments** (`experiments/`) come before production code (`src/`). Prototype an idea, record what was learned, then implement it properly.
- **Mapping to the README:** Layers 0-3 correspond to *Core DSP*, Layers 4-5 to *Real-time engine*, Layers 6-8 to *Instrument and interface*, and Layer 9 to *AI-assisted modulation*.

## Guiding rules

1. **Offline first, real-time second.** Prove each algorithm by rendering to a file before running it in a live audio callback.
2. **Test the maths, listen to the result.** Automated tests catch regressions. Listening catches everything else. Do both.
3. **Keep the audio thread clean.** No allocation, locks, file I/O, logging, or network calls inside the audio callback. This rule applies from the first real-time code onward.
4. **Separate DSP from the framework.** Core engine code should depend on the standard library, not on JUCE types, wherever practical. JUCE wraps the engine; it does not contain it.
5. **Write down decisions.** Record each non-obvious choice in `docs/` with the reasoning and the alternatives considered.

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
- [ ] Continuous formatting and warning settings agreed (warnings as errors where practical)

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
- [ ] Hann window
- [ ] At least one alternative (for example Tukey or triangular) behind the same interface
- [ ] Precomputed window tables, so grains never compute windows per sample

### 2.2 Interpolation
- [ ] Linear interpolation at fractional read positions
- [ ] Higher-order interpolation (for example cubic Hermite) behind the same interface
- [ ] Defined behaviour at buffer boundaries (clamp, wrap, or zero, chosen and documented)

### 2.3 Utilities
- [ ] Semitone/cents to playback-rate conversion
- [ ] Gain and decibel helpers
- [ ] A simple deterministic random source for reproducible tests

**Experiments**
- [ ] Render a sine wave at fractional rates with each interpolator and compare spectra to hear and see aliasing and high-frequency loss
- [ ] Compare window shapes on a repeated short grain

**Tests**
- [ ] Windows: correct length, endpoints, symmetry, expected overlap-sum behaviour
- [ ] Interpolation: exact at integer positions, correct at midpoints, safe at boundaries

**Exit criteria**
- Each primitive has tests and a short note in `docs/` on its trade-offs.
- Interpolators are swappable without changing grain code.

---

## Layer 3: A single grain

**Goal:** One grain, played correctly, offline.

- [ ] Define grain state: start position, current position, playback rate, length, window, amplitude, active flag
- [ ] Render one windowed grain from the sample buffer into an output buffer
- [ ] Support variable playback rate (pitch) via interpolation
- [ ] Handle grains that run past the end of the buffer
- [ ] Add per-grain pan and gain
- [ ] Ensure a grain can be reset and reused without allocation

**Experiments**
- [ ] Render the same grain at several lengths and rates to a file and listen for clicks and artefacts

**Tests**
- [ ] Grain produces exactly its configured length of non-zero output
- [ ] Output starts and ends at zero with the chosen window
- [ ] Rate of 1.0 reproduces the source slice (within window shaping)

**Exit criteria**
- A single grain renders to a WAV that sounds clean at multiple pitches.
- Grain state is small, fixed-size, and trivially copyable.

---

## Layer 4: Many grains

**Goal:** A scheduler and mixer that turn single grains into a texture, still offline.

- [ ] Fixed-capacity grain pool (no allocation after startup)
- [ ] Grain scheduler: trigger by density (grains per second) and by interval
- [ ] Timing variation (jitter) around the nominal interval
- [ ] Per-grain parameter variation: position spread, pitch spread, length, pan
- [ ] Overlap-add mixing of all active grains
- [ ] Defined behaviour when the pool is full (drop oldest, drop new, or steal quietest)
- [ ] Gain compensation so output level stays sensible as overlap changes

**Experiments**
- [ ] Render textures from the same sample at low, medium, and high density
- [ ] Measure CPU cost per grain offline to estimate a safe maximum

**Tests**
- [ ] Seeded runs are bit-for-bit reproducible
- [ ] Active grain count never exceeds pool capacity
- [ ] Output never exceeds a defined peak at maximum density

**Exit criteria**
- The engine renders a controllable granular texture to a file from a single function call.
- Maximum grain count and per-block cost are measured and written down.

---

## Layer 5: Real-time engine

**Goal:** The same engine running live inside an audio callback.

- [ ] Wrap the engine in a JUCE audio processor (`prepareToPlay`, `processBlock`)
- [ ] Handle arbitrary block sizes and sample rates
- [ ] Audit `processBlock` for allocation, locks, and blocking calls
- [ ] Load samples on a background thread and hand them to the audio thread safely
- [ ] Expose engine parameters through thread-safe, smoothed values (atomics plus per-block or per-sample smoothing)
- [ ] Add playback position control and pitch modulation as live parameters
- [ ] Add denormal protection and output limiting or clipping safety

**Validation**
- [ ] Run for an extended session with no dropouts at typical buffer sizes (64-512 samples)
- [ ] Stress test at maximum grain density and small buffer sizes
- [ ] Verify the parameter-smoothing path produces no zipper noise
- [ ] Confirm behaviour when the sample is swapped during playback

**Exit criteria**
- Drizel plays a loaded sample live with adjustable position, pitch, and density, and no audible glitches.
- Measured worst-case callback time is well under the available buffer time, with margin documented.

---

## Layer 6: Modulation system

**Goal:** A single, general way to move parameters over time, so later features plug in rather than special-case.

- [ ] Define a modulation source interface that outputs a value per block or per sample
- [ ] LFO source (shape, rate, depth)
- [ ] Envelope source
- [ ] Random-walk source for slow organic drift
- [ ] Modulation routing: source, destination, amount
- [ ] All modulated parameters pass through the same smoothing path

**Tests**
- [ ] Sources produce expected ranges and rates
- [ ] Routing with zero amount leaves a parameter unchanged

**Exit criteria**
- Position, pitch, density, and spread can each be driven by any source.
- Adding a new source requires no changes to the engine.

> This layer is the attachment point for MIDI (Layer 7) and for local execution of prompt-derived policies (Layer 9).

---

## Layer 7: Instrument behaviour (MIDI)

**Goal:** Make the engine playable as an instrument.

- [ ] MIDI note input mapped to pitch
- [ ] Note on/off with an amplitude envelope
- [ ] Decide the voicing model: one shared grain cloud, or a cloud per note
- [ ] MIDI CC mapping to parameters
- [ ] Pitch bend and mod wheel
- [ ] Sample-accurate MIDI event timing within a block
- [ ] Presets: save and recall parameter sets

**Exit criteria**
- A MIDI keyboard plays the instrument with correct pitch and clean note starts and releases.
- A saved preset reloads to an identical sound.

---

## Layer 8: Interface and packaging

**Goal:** A usable, shippable instrument.

**Interface**
- [ ] Parameter controls bound to engine parameters
- [ ] Waveform display with a position marker and grain activity
- [ ] Sample loading by file chooser and drag-and-drop
- [ ] Modulation assignment and amount controls
- [ ] Output meter
- [ ] Interface updates never block or allocate on the audio thread

**Packaging and testing**
- [ ] Standalone build on the primary platform
- [ ] Audio plugin build (VST3 and/or AU, as chosen)
- [ ] Test in at least two plugin hosts
- [ ] State save and restore inside a host session
- [ ] Verify AGPL-3.0 and third-party (including JUCE) license obligations for distribution

**Exit criteria**
- A new user can install a build, load a sample, and make sound without reading the source.
- The plugin passes a host validation tool and survives project save/reload.

---

## Layer 9: Promptable modulation policies (experimental)

**Goal:** Let a musician describe how a parameter should move, translate that
intent into an editable policy, and execute the accepted behaviour locally.
Model inference stays outside the audio callback.

**Prerequisite:** Layers 5 and 6 are complete. The engine already accepts smoothed, timestamped parameter changes.

The first target is grain scan position. Example prompts are "very chaotic",
"go around in circles", and "one step forward two step back". These map to
proposed irregular, cyclic, and directional-step patterns; their meanings and
defaults must be tested rather than assumed. See the
[promptable modulation proposal](promptable-modulation.md). No model or provider
has been selected, and the core instrument remains independent of that choice.

### 9.1 Studies (in `experiments/llm-control/`)
- [ ] Define the initial grain-scan policy vocabulary, parameter units, region bounds, and repeat behaviour
- [ ] Compare direct pattern controls with prompt interpretation using the three seed examples and paraphrases
- [ ] Test prompt formats and output schemas for validity and consistency
- [ ] Show the interpreted behaviour as a preview with editable settings
- [ ] Record failure modes (malformed output, out-of-range values, unmusical choices)
- [ ] Measure inference latency and jitter from the actual deployment location
- [ ] Compare hosted and local models for quality, latency, and licensing fit

### 9.2 Sample analysis
- [ ] Offline analysis of onsets, loudness, silence, and spectral features
- [ ] Convert the analysis into a labelled map of regions for the model to reference

### 9.3 Plumbing
- [ ] Versioned policy schema with supported pattern names, explicit units, bounds, timing, and random seeds
- [ ] Validate and prepare accepted policies outside the audio thread; execute only supported numeric operations
- [ ] Local execution of cyclic scans, directional steps, and seeded irregular movement
- [ ] Trajectory format: timestamped breakpoints for position, density, spread, and pitch, several seconds ahead
- [ ] Lock-free single-producer, single-consumer queue from control worker to audio thread
- [ ] Interpolation of breakpoints inside the engine as a Layer 6 modulation source
- [ ] Inference worker with timeouts and cancellation; network requests only if a hosted model is chosen
- [ ] Persist the accepted policy, schema version, edits, and seed alongside the prompt
- [ ] Invalidate stale results when the selected sample or prompt changes

### 9.4 Resilience
- [ ] Keep the current local policy running when inference is late or fails; use a documented continuation for finite trajectories
- [ ] Reject invalid policies and enforce all numeric limits before handoff to the engine
- [ ] Smooth policy changes at a defined activation point
- [ ] Clear user-facing state: interpreting, ready, or unable to interpret; preserve access to manual controls
- [ ] The instrument is fully usable with this feature disabled

**Exit criteria**
- Each seed prompt produces a valid, visible, editable policy whose offline render matches the documented behaviour.
- The same saved policy, seed, source, and render settings reproduce the same result in the tested engine build, without a new model call.
- Disabling inference or disconnecting a hosted model during playback does not interrupt audio or the active local policy.
- Malformed, late, and stale results leave the active policy intact, and applying a valid update satisfies the real-time engine's callback budget.

---

## Dependency summary

```
Layer 0  Foundations
   └─ Layer 1  Audio data and file I/O
        └─ Layer 2  Signal primitives (windows, interpolation)
             └─ Layer 3  Single grain
                  └─ Layer 4  Many grains (scheduler, mixer)
                       └─ Layer 5  Real-time engine
                            └─ Layer 6  Modulation system
                                 ├─ Layer 7  MIDI and instrument behaviour
                                 │    └─ Layer 8  Interface and packaging
                                 └─ Layer 9  AI-assisted modulation (experimental)
```

Layers 7 and 9 both depend only on Layer 6, so they can proceed in either order or in parallel. The suggested order is Layer 7 first, because a playable instrument gives the AI studies something real to control.

## Checkpoints

| Milestone | Reached when |
|---|---|
| **Load and verify a WAV** | Initial offline Layer 1 scope met; recorded 10 October 2026 |
| **Hear a grain** | Layer 3 exit criteria met |
| **Hear a texture** | Layer 4 exit criteria met |
| **Play it live** | Layer 5 exit criteria met |
| **Play it with a keyboard** | Layer 7 exit criteria met |
| **Share it** | Layer 8 exit criteria met |
| **Let a model steer it** | Layer 9 exit criteria met |
