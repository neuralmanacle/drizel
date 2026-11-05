# Development Roadmap

This roadmap builds Drizel from the bottom up. Each layer depends only on the layers beneath it, and each ends with a concrete, testable result before the next begins. Nothing in a higher layer is started until the exit criteria of the layer below are met.

> **Status — 5 November 2026:** Step 9 combines Layer 7 MIDI instrument
> behaviour with Layer 8 interface/preset and packaging work. See the
> [Step 9 guide](step-9-instrument.md) for the implementation and acceptance
> checklist. The first offline WAV milestone remains recorded in the
> [10 October log](devlog/2026-10-10.md). Host listening, primary-platform
> acceptance and distribution checks remain open; promptable modulation is
> still a research direction.

## How to read this roadmap

- **Layers** are ordered by dependency, not by difficulty or visibility. The user interface sits near the top because it needs everything beneath it.
- **Checkboxes** record verified implementation or completed validation, not release readiness. Study items require learner confirmation. Partly completed items are split so automated checks do not imply listening or host acceptance.
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
- [x] Strict compiler warnings and warnings-as-errors configured for core tests and the WAV adapter
- [ ] Continuous formatting policy and automated formatting checks agreed

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

**Exit criteria**
- Position, pitch, density, and spread can each be driven by any source.
- Adding a new source requires no changes to the engine.

> Layer 6 remains partial: a general source interface, routable envelopes, random
> walk and density/spread modulation routes are still open. Step 9 uses the existing
> LFO route and a dedicated MIDI amplitude envelope; it does not complete Layer 6.
>
> This layer is the attachment point for MIDI (Layer 7) and for local execution of prompt-derived policies (Layer 9).

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

**Exit criteria**
- A MIDI keyboard plays the instrument with correct pitch and clean note starts and releases.
- A saved preset reloads to an identical sound.

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
