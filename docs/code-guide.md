# Reading the current code

**Updated 11 November 2026 for Step 11.** The shared-cloud MIDI instrument now
uses WAV analysis to guide grain starts. See the [roadmap](roadmap.md) for
remaining host validation, general modulation, ML and embedded work.

## Reading order

| Order | File | What to follow |
|---|---|---|
| 1 | [SampleBuffer](../src/audio/SampleBuffer.h) | Planar storage, sample rate, frame counts, lifetime and access |
| 2 | [WAV API](../src/io/WavFile.h) and [implementation](../src/io/WavFile.cpp) | Offline codec ownership, validation and read/write errors |
| 3 | [Window](../src/audio/Window.h) and [Interpolation](../src/audio/Interpolation.h) | Hann tables, fractional reads and source boundary handling |
| 4 | [Grain](../src/audio/Grain.h) | Playback position/rate, age, window, gain and pan |
| 5 | [SampleFeatures](../src/audio/SampleFeatures.h) and [SampleSelection](../src/audio/SampleSelection.h) | Centered FFT frames, feature weights, regional candidate tables and launch lookup |
| 6 | [GrainCloud](../src/audio/GrainCloud.h) | Fixed voice capacity, scheduling, seeded variation and overlap-add |
| 7 | [RealtimeEngine](../src/audio/RealtimeEngine.h) | Prepared samples, active/pending/retired handoff, smoothing and output safety |
| 8 | [Modulation](../src/audio/Modulation.h) | Sine/triangle LFO and current single position/pitch route |
| 9 | [MidiInstrument](../src/audio/MidiInstrument.h) | Held-key priority, amplitude envelope, bend and controller overrides |
| 10 | [Processor header](../src/PluginProcessor.h) and [implementation](../src/PluginProcessor.cpp) | Loader worker, parameter snapshots, timestamped MIDI, render path, presets/state |
| 11 | [Editor header](../src/PluginEditor.h) and [implementation](../src/PluginEditor.cpp) | Parameter attachments, waveform, telemetry, file loading and preset controls |
| 12 | [CMake](../CMakeLists.txt), [Projucer project](../drizel.jucer) and [CI](../.github/workflows/offline-audio.yml) | Core/offline targets, optional plugin build, dependencies and validation scope |

Start with public contracts, then trace one real execution path. Comments that
refer to a file's original implementation step explain that local scope; use the
current [roadmap](roadmap.md) and [Step 9 guide](step-9-instrument.md) for status.

## Trace the execution paths

### Offline rendering

Follow [WAV roundtrip](../experiments/wav-roundtrip/main.cpp),
[single grain](../experiments/single-grain/main.cpp) and
[grain cloud](../experiments/grain-cloud/main.cpp). These programs use their own
`main()` and run ordinary file I/O outside a real-time callback. They provide
reference renders and a place for experiments before live integration.

### Live playback and sample replacement

The loader prepares audio, grain windows and selection tables in one packet.
Trace cancellation, revision and rate checks, and active/pending/retired ownership
in `RealtimeEngine.h` alongside the processor. The loader reclaims retired packets.

### Guided grain launches

Follow the block snapshot of `grainSelection` through `RealtimeEngine::next` to
`GrainCloud::launch`. Position selects a region of the table, and a seeded draw
chooses a weighted candidate. Empty tables return the original Manual position.
See the [Step 11 devlog](devlog/2026-11-11-step-11-sample-aware-playback.md) for feature equations and measured results.

### MIDI and state

The processor snapshots parameters and dispatches events at their sample offsets.
`MidiInstrument` supplies last-note pitch, velocity envelope and controller state;
the shared cloud uses smoothed controls and a final amplitude gate. MIDI silence
does not stop cloud work. Read the Step 9 guide for supported channel/CCs and the
current lack of polyphony, sustain pedal and MPE.

Presets store parameters and an external WAV path, not sample audio or an exact
performance history. Scheduler/random history, held notes and LFO phase are not
serialised; current preset recall is not sample-identical render reproduction.

## Ownership questions to answer

| Resource | Question |
|---|---|
| SampleBuffer storage | Who owns it, and when may a grain borrow its samples? |
| PreparedSample | Which thread constructs, activates, retires and destroys it? |
| Host audio/MIDI buffers | How long are borrowed pointers/bytes valid? |
| Parameters and UI telemetry | What is atomic or snapshotted, and what is audio-thread state? |
| Editor and processor | Why must audio work continue when the editor closes? |
| Candidate tables | How does packet ownership keep the table and decoded source paired through cancellation and replacement? |

`const` restricts writes through an interface; it does not make concurrent writes
elsewhere safe. RAII destruction can perform work, so choose the destruction
thread deliberately. No file I/O, logging, locks, allocation, parsing, network
calls or unbounded loops belong in the callback.

## Tests and acceptance

Read [audio-data tests](../tests/AudioDataTests.cpp),
[primitive tests](../tests/SignalPrimitiveTests.cpp),
[grain tests](../tests/GrainTests.cpp), [cloud tests](../tests/GrainCloudTests.cpp),
[real-time tests](../tests/RealtimeEngineTests.cpp),
[modulation tests](../tests/ModulationTests.cpp),
[MIDI tests](../tests/MidiInstrumentTests.cpp) and
[processor tests](../tests/PluginProcessorTests.cpp).

Root CMake registers the relevant executables with CTest. Use
`ctest --test-dir <build> -N` to list tests for your build configuration.
The [Step 11 verification](devlog/2026-11-11-step-11-sample-aware-playback.md#what-the-checks-showed) records 11 CTest entries, 13 Python tests and the guided-path measurements.
A numerical test, successful CI build or validator run does not substitute for
listening, primary-machine host acceptance or measured callback headroom.

## How the layers attach

| Layer | Starting point | Planned or remaining work |
|---|---|---|
| 2 | Windows, interpolation and grain reader | Measured filtering/resampling quality |
| 6 | Existing tests, processor and engine | Timing and host/listening evidence |
| 7 | Modulation and smoothed controls | Bounded general source/route interface |
| 9–10 | Python analysis, C++ parity and worker-prepared selection | DAW listening and host timing during sample replacement |
| 11–12 | Analysis feature schema | Evaluated classifier then worker-side C++ inference |
| 14 | Layer 6 routes and local policy executor | Validated numeric policy crosses into playback |
| 15 | Optional output-stage experiment | Audio-rate inference needs its own timing/memory audit |
| 15–16 | Framework-independent core | Hardware adapter, then optional tiny inference |

Proposed source directories and experiment names in planning docs are not
implemented APIs. Consult the [roadmap](roadmap.md), [architecture](architecture.md),
[curriculum](../learnings/audio-dsp-ml-curriculum.md) and
[evidence template](proof-of-work.md) when working through a layer.
