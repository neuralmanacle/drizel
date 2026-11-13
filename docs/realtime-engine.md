# Step 6 — Play the grain cloud live

Step 5 rendered a texture to a file. Step 6 connects that same `GrainCloud` to
JUCE's audio callback. This corresponds to **Layer 5** of the roadmap, not
Layer 6 (the later modulation system).

## Try it on your Mac

Open `drizel.jucer` in Projucer, save/regenerate the Xcode project, and build
Standalone, VST3, or AU. The WAV adapter now compiles in the plugin, and the
project explicitly uses C++17 and the `src` header search path.

Alternatively, with JUCE 8.0.15 available:

```sh
cmake -S . -B build-plugin -DDRIZEL_JUCE_PATH=../JUCE -DDRIZEL_BUILD_PLUGIN=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-plugin --parallel 2
ctest --test-dir build-plugin --output-on-failure
```

CMake puts plugin products under `build-plugin/DrizelPlugin_artefacts/` and does
not automatically install them. The default CMake configuration still builds
only the command-line experiments and tests.

Click **Load WAV**, choose a supported mono/stereo PCM16, PCM24 or float32 WAV,
and wait for Ready. Play defaults on: this is a free-running texture instrument,
so MIDI notes are not required. Use a file with sound near its beginning, or
move Position to an audible section. Loading failures appear in the status line
and preserve the currently playing source. The editor can close while audio
continues; the processor owns the loader and engine.

| Control | Meaning |
|---|---|
| Position, 0–1 | Exact start target in Manual mode and region selector in guided modes |
| Grain selection | Manual, Attack, Sustain, Avoid silence or Bright |
| Pitch, −24 to +24 | Semitones relative to source pitch, including source/output rate correction |
| Density, 1–100 | Number of grains started each second |
| Output gain, 0–1 | Smoothed multiplier after the cloud mix |
| Play | Smoothed mute/unmute; the internal scheduler continues while muted |

Grains are fixed at 100 ms with Hann envelopes, linear interpolation and stereo
pan spread. Near the end of a file, grains read zero-padded silence instead of
wrapping. Pitch shifting retains the earlier interpolator's aliasing limitations.
For the controls added after Step 6, see [Step 9 MIDI](step-9-instrument.md)
and [Step 11 sample-aware playback](devlog/2026-11-11-step-11-sample-aware-playback.md).

## Ownership and the audio boundary

`PreparedSample` owns decoded samples, its cloud and the selection tables. The source is declared
before the cloud so destruction happens in the safe reverse order. The packet
stays at one address: grains borrow both source data and window tables.

1. The control thread queues a path and increments a request revision under a
   mutex. `prepareToPlay` queues a new output rate through the same mechanism.
2. The worker snapshots the request, decodes the WAV, scans its peak and prepares
   windows and candidate tables. Analysis checks for cancellation between
   windows. The worker discards obsolete results before publication.
3. The worker exchanges a raw pointer into a one-slot pending mailbox. An
   exchange transfers exclusive ownership; it is not a shared pointer/refcount.
   Superseded pending packets are destroyed by the worker.
4. Audio observes only whether pending is non-null, fades down over 10 ms, then
   exchanges the pending pointer to take ownership. It never dereferences a
   pointer borrowed from an atomic load, since the worker could replace it.
5. At zero fade gain, audio activates a packet with the current output rate and
   request revision. It places the previous packet in the retirement slot, or
   retires the incoming packet if its rate or revision is obsolete.
6. The worker collects retirement slots every 20 ms. If that slot is occupied,
   audio postpones the swap at silence; it never waits, deletes or overwrites an
   uncollected packet. Fade-in takes another 10 ms. This deliberate short dip is
   not a seamless crossfade, and stalled loading/reclamation can extend it.

The worker joins before processor members are destroyed. Host preparation and
processor destruction require audio processing to be stopped. Loading is bounded
by the existing decoded-memory limit (256 MiB per sample); active, pending and
in-flight data can coexist, so total memory can exceed one file's limit. Decoding
is not interruptible mid-file; rapid requests are coalesced after decoding.

## What runs for each sample?

The processor reads cached atomic parameter values once per block. Every output
frame then passes those targets through a 20 ms one-pole smoother:

`current += (1 - exp(-1 / (sampleRate * 0.020))) * (target - current)`

The coefficient is calculated during preparation. The cloud receives the
smoothed source position, pitch and density without regenerating windows or
rescanning samples. Scheduler countdown is rescaled by the interval ratio to
preserve fractional trigger phase. Existing grains retain their original start,
pitch and amplitude. At launch, guided selection consults a bounded table for
that region, falling back to Manual when the table is empty. Output gain and Play
smooth the final mix. A finite check and final clamp protect the output.

Preparation computes grain gain at the **maximum supported density**, 100/s.
Live controls can reduce density but cannot exceed that prepared ceiling. This
keeps overlapping old/new grains inside the same conservative bound, without
abrupt gain changes when density moves. Sparse textures are consequently quieter.

`processBlock` handles each actual buffer length and clears unused channels.
Stereo uses separate left/right samples; mono receives their average. It performs
no file I/O, window creation, source scan, lock acquisition, allocation, deletion,
logging, UI calls or thread creation. JUCE's scoped denormal guard covers the
callback. Atomic pointer and float lock-freedom are checked at compile time.

## State and validation

Host state stores parameter values, including grain selection mode, and the
selected sample path. Restoring queues background decoding and table generation.
Older states restore Manual mode. Missing files report failure. An
empty saved path queues a silent source. Projects depend on files remaining at
their saved paths. The full preset/browser design remains later work.

`RealtimeEngineTests.cpp` checks silence before loading, finite bounded output,
rendering across 44.1/48/96/192 kHz output rates, repeatable streams
across irregular frame groups, smoothed muting, sample replacement, rejection
of old-rate packets, invalid controls and simultaneous publication/reclamation.
Replacement `new`/`delete` functions abort if invoked by the simulated audio
thread. AddressSanitizer and UndefinedBehaviorSanitizer additionally check memory/undefined behaviour
(leak detection is disabled because the execution sandbox prevents its process inspection). These tests
exercise the framework-independent callback path, not a physical audio device.

The aggregate render timer is a throughput indicator only. Target-machine
worst-case callback timing at 64/128/256/512 samples, extended listening, host
save/reload, rapid file swaps, device changes and zipper-noise assessment remain
required before declaring the roadmap's real-time exit criteria complete.

## File guide

- `audio/GrainCloud.h`: adds a bounded live-update method without changing the
  offline renderer's defaults or prepared windows.
- `audio/RealtimeEngine.h`: owns the two mailboxes, fade/smoothing history and
  the active prepared sample. Inline comments describe each ownership step.
- `PluginProcessor.*`: supplies host parameters, worker requests, lifecycle,
  callback rendering and serialised state.
- `PluginEditor.*`: supplies a small load/status panel and JUCE's generic host
  parameter editor. File chooser callbacks use a safe component pointer.
- `tests/RealtimeEngineTests.cpp`: tests engine behaviour and callback allocation
  rules without requiring a host, GUI or audio device.

### Recorded automated checks (22 October 2026)

- Debug CMake build against JUCE 8.0.15: successful; all 8 CTest entries passed.
- Processor/editor translation units: strict GCC C++17 syntax check against the
  real JUCE headers passed. This is not a linked plugin or host validation.
- AddressSanitizer/UndefinedBehaviorSanitizer engine run: passed with leak
  detection disabled for the sandbox limitation described above.
- Optimised engine throughput: 480,000 frames (10 s at 48 kHz), maximum supported
  density, about 0.066 s elapsed on this shared Linux runner. This is aggregate
  throughput, not worst-case per-block timing or a target-Mac latency guarantee.
