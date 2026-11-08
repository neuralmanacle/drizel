# Reading the current code

**Updated 8 November 2026 for the merged Step 9 implementation.** The plugin is
a playable shared-cloud MIDI instrument, not the original silent JUCE scaffold.
Audio analysis, model inference, general modulation and embedded deployment are
planned work; see the [post-Step-9 plan](post-step-9-plan.md).

## Reading order

| Order | File | What to follow |
|---|---|---|
| 1 | [SampleBuffer](../src/audio/SampleBuffer.h) | Planar storage, sample rate, frame counts, lifetime and access |
| 2 | [WAV API](../src/io/WavFile.h) and [implementation](../src/io/WavFile.cpp) | Offline codec ownership, validation and read/write errors |
| 3 | [Window](../src/audio/Window.h) and [Interpolation](../src/audio/Interpolation.h) | Hann tables, fractional reads and source boundary handling |
| 4 | [Grain](../src/audio/Grain.h) | Playback position/rate, age, window, gain and pan |
| 5 | [GrainCloud](../src/audio/GrainCloud.h) | Fixed voice capacity, scheduling, seeded variation and overlap-add |
| 6 | [RealtimeEngine](../src/audio/RealtimeEngine.h) | Prepared samples, active/pending/retired handoff, smoothing and output safety |
| 7 | [Modulation](../src/audio/Modulation.h) | Sine/triangle LFO and current single position/pitch route |
| 8 | [MidiInstrument](../src/audio/MidiInstrument.h) | Held-key priority, amplitude envelope, bend and controller overrides |
| 9 | [Processor header](../src/PluginProcessor.h) and [implementation](../src/PluginProcessor.cpp) | Loader worker, parameter snapshots, timestamped MIDI, render path, presets/state |
| 10 | [Editor header](../src/PluginEditor.h) and [implementation](../src/PluginEditor.cpp) | Parameter attachments, waveform, telemetry, file loading and preset controls |
| 11 | [CMake](../CMakeLists.txt), [Projucer project](../drizel.jucer) and [CI](../.github/workflows/offline-audio.yml) | Core/offline targets, optional plugin build, dependencies and validation scope |

Start with public contracts, then trace one real execution path. Comments that
refer to a file's original implementation step explain that local scope; use the
current [roadmap](roadmap.md) and [Step 9 guide](step-9-instrument.md) for status.

## Trace three execution paths

### Offline rendering

Follow [WAV roundtrip](../experiments/wav-roundtrip/main.cpp),
[single grain](../experiments/single-grain/main.cpp) and
[grain cloud](../experiments/grain-cloud/main.cpp). These programs use their own
`main()` and run ordinary file I/O outside a real-time callback. They provide
reference renders and a place for experiments before live integration.

### Live playback and sample replacement

A loader worker prepares audio and cloud state. The engine owns active, pending
and retired packets through a defined handoff; callback processing does not
load/decode files or destroy sample storage. Trace replacement, stale output-rate
rejection and worker reclamation in `RealtimeEngine.h` alongside the processor.
An atomic pointer alone is not a lifetime or real-time-safety proof.

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
| Future analysis/model output | How will source identity, cancellation and reclamation be handled? |

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

Root CMake registers the relevant executables with CTest. The Step 9 verification
log reports 11 entries in the plugin-enabled configuration; actual discovery
with `ctest --test-dir <build> -N` depends on the selected build configuration.
See the [Step 9 verification log](devlog/2026-11-05-step-9.md) for recorded results.
A numerical test, successful CI build or validator run does not substitute for
listening, primary-machine host acceptance or measured callback headroom.

## How the next work attaches

| Step | Starting point | Planned boundary |
|---|---|---|
| 10 | Existing tests, processor and engine | Measurement and host/listening evidence |
| 11 | Modulation and smoothed controls | Bounded general source/route interface |
| 12 | Windows, interpolation and grain reader | Measured quality option, not assumed alias rejection |
| 13–14 | Source buffer and loading ownership | Offline reference then worker-prepared analysis maps |
| 15–16 | Analysis feature schema | Evaluated classifier then worker-side C++ inference |
| 17 | Step 11 routes and local policy executor | Validated numeric policy crosses into playback |
| 18 | Optional output-stage experiment | Audio-rate inference needs a separate real-time audit |
| 19–20 | Framework-independent core | Separate hardware adapter, then optional tiny inference |

Proposed source directories and experiment names in planning docs are not
implemented APIs. Consult the [architecture](architecture.md),
[curriculum](../learnings/audio-dsp-ml-curriculum.md) and
[evidence template](proof-of-work.md) before starting a new step.
