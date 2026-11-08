# Step 9 — A playable granular instrument

Step 9 combines roadmap Layer 7 (MIDI) and Layer 8 (interface and packaging).
The previous Step 8 interface is the starting point. This increment implements
instrument controls and preset files; it does not claim release readiness.

## Play it

1. Build with JUCE 8.0.15 using the commands below, or open `drizel.jucer`, save
   it in Projucer, and rebuild your Xcode targets. Regenerate the project so the
   MIDI-input capability reaches the plugin wrappers.
2. Load or drop a WAV. Keep **Output on** enabled.
3. Set **Mode** to **MIDI**. Route a keyboard or DAW MIDI track on **channel 1**.
   In Standalone, select the MIDI input device in its audio/MIDI settings.
4. Play MIDI note **60** for the original sample pitch. Set **Root MIDI note**
   to the note represented by your source; this is manual, not pitch detection.
5. Adjust attack and release. Velocity controls envelope amplitude. Switch to
   **Drone** for the original continuous texture workflow.
6. Choose **Save preset** and a filename ending in `.drizel`. **Load preset**
   recalls controls and asynchronously reloads the referenced WAV.

| Input | Behaviour |
|---|---|
| Note on/off | Last-note priority; releasing the newest key returns to an older held key |
| Velocity zero note-on | Note-off |
| Pitch bend | Fixed -2 to +2 semitones |
| CC1 | Adds 0–1 to LFO depth, clamped to 1; choose an LFO destination first |
| CC74 | Overrides base position, 0–1 |
| CC71 | Overrides density, 1–100 grains/sec |
| CC121 | Clears CC overrides, mod wheel and bend; keeps held notes |
| CC123 | Releases all notes through the release envelope |
| CC120 | Immediate silence and controller reset |

Other channels, sustain pedal, MPE and polyphony are not implemented. CC values
are performance overrides; they do not move host parameter controls or get saved
in presets. CC121 returns control to the knobs. Mode changes, prepare and state
restore clear the held-note/controller state. The default is Drone for existing
sessions and the original load-and-play workflow.

## Design and timing

`MidiInstrument.h` is independent of JUCE. A fixed 128-key stack provides
monophonic last-note priority. Repeated note-on replaces a key's previous entry.
The amplitude envelope has attack, sustain-at-velocity and release stages;
retriggering ramps from the current level. Envelope times are captured when a
stage starts; changing a knob affects the next stage.

`processBlock` reads borrowed MIDI bytes without constructing SysEx messages.
Every event is dispatched before the sample at its offset; simultaneous events
retain buffer order. No file I/O, allocation, parameter notifications or locks
are added to the audio callback. Parameters are snapshotted once per block.

The note offset is added after LFO pitch routing, then enters the existing 20 ms
pitch smoother. The live pitch range accommodates all MIDI notes, root settings,
base tuning and bend. Existing grains retain their launch pitch for up to their
100 ms duration, so note changes deliberately blend rather than retuning all
active grains. Very high transposition uses the existing interpolator without
anti-alias filtering; this is a sound-quality limitation, not pitch detection.

One shared cloud stays running under the final amplitude gate. This preserves
bounded storage and avoids abrupt grain replacement. MIDI silence does not stop
cloud CPU work. Applying the envelope after output smoothing gives an exact
silent gate before the first note and after release. Sample offsets describe
control dispatch; the Hann grain attack and scheduler affect audible onset.

## Presets and state

Preset files use the same binary XML state as DAW save/restore. They contain host
parameters and an absolute WAV path, not sample audio. Invalid preset roots and
oversized preset files are rejected before applying state. Missing WAVs report
load failure and keep the currently loaded source, as in ordinary sample loading.
Keep the original WAV at its path when reopening a preset/session.

Recalling a preset restores a patch, not a sample-identical performance: held
notes, controller overrides, cloud scheduler/random history and LFO phase are
not serialized. The source file must be unchanged. Identical offline renders
would require a separate transport/seed-reset contract and are not claimed here.

## Build and validation

Linux Standalone/VST3 builds and all 11 plugin-enabled CTest entries passed.
See the [verification log](devlog/2026-11-05-step-9.md) for scope and limitations.

```sh
cmake -S . -B build -DDRIZEL_JUCE_PATH=../JUCE \
  -DDRIZEL_BUILD_PLUGIN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

CMake builds Standalone and VST3; macOS also builds AU. Build products are under
`build/DrizelPlugin_artefacts/Debug/`. Builds are not automatically installed.
The Projucer project also declares MIDI input. CI compiles macOS plugin formats, runs core and processor integration tests,
and uploads unsigned Debug build artifacts with source commit and license
pointers for development testing. These are not signed release installers or a
complete third-party notice bundle. A configured CI job is not evidence it has passed.

Before marking Layers 7/8 complete, record the actual results:

- On the primary Mac, launch Standalone, select MIDI input, load a WAV and play.
- Test VST3/AU in two named hosts. Verify MIDI channel routing, velocity, note
  overlaps, zero-velocity note-on, bend, CCs, release, panic and mode changes.
- Check 44.1/48/96 kHz and 64/128/512-frame blocks; listen for clicks and measure
  worst-case callback time at density 100, during loading and UI interaction.
- Save/reopen a host project and preset; verify parameter values, sample path,
  missing-file status, malformed preset handling and all editor attachments.
- Run the platform's plugin validator (for AU: `auval -v aumu Drzl NeuM`) and
  a VST3 validator/pluginval; retain logs. Build success is not host validation.
- Audit distribution files against root `LICENSE` and pinned JUCE `LICENSE.md`,
  including module/SDK third-party notices; include corresponding source and
  build instructions as required by the selected licensing route. No legal or
  distribution compliance sign-off is claimed by this implementation.
- Decide signing, notarization and installation packaging before public release.

## Get Step 9 on your Mac

Step 9 was merged into `main` in commit `341d211` on 5 November 2026.
For the current implementation, use `main` from your existing checkout (replace
the first path with your local repo):

```sh
cd /path/to/drizel
git status
git fetch origin
git switch main
git pull --ff-only origin main
git log -1 --oneline
```

Commit or stash local edits first if Git reports a conflict; these commands
do not discard edits. For a separate clean checkout instead:

```sh
git clone --branch main https://github.com/neuralmanacle/drizel.git drizel-step-9
cd drizel-step-9
```

For Projucer, run `open drizel.jucer`, verify JUCE module paths, save to
regenerate the Xcode project, then open that project and build Standalone.
For CMake, with CMake/Xcode command-line tools installed and JUCE 8.0.15 at
`../JUCE` (adjust the path for your checkout):

```sh
cmake -S . -B cmake-build-step9 -DDRIZEL_JUCE_PATH=../JUCE \
  -DDRIZEL_BUILD_PLUGIN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-step9 --parallel 2
ctest --test-dir cmake-build-step9 --output-on-failure
open cmake-build-step9/DrizelPlugin_artefacts/Debug/Standalone/drizel.app
```

This fresh build directory avoids old generated build settings. Load a WAV,
select MIDI mode and enable a channel-1 MIDI device in Standalone settings.
Play note 60 with root 60, check velocity/release/bend, then save and recall a
preset. Switch to Drone to check audio output without a MIDI keyboard.

## Files to read

- `src/audio/MidiInstrument.h`: key priority, envelope and controller state.
- `src/PluginProcessor.cpp`: timestamped events, final amplitude gate, presets.
- `src/PluginEditor.cpp`: mode, envelope/root controls and preset dialogs.
- `tests/MidiInstrumentTests.cpp`: core behaviour and partitioned event timeline.
- `tests/PluginProcessorTests.cpp`: actual MIDI offsets, output gating, presets and host state.
- `CMakeLists.txt` and `drizel.jucer`: MIDI-enabled build configuration.

## Continue through the roadmap

Close the outstanding Layer 5 real-time and Layer 7/8 instrument/host checks above,
complete Layer 6 general modulation and the Layer 2 sound-quality experiments.
The [integrated roadmap](roadmap.md) then continues through analysis, sample-aware
playback, audio ML, promptable control and embedded audio as Layers 9–16.
Use the [curriculum](../learnings/audio-dsp-ml-curriculum.md) and
[evidence template](proof-of-work.md) at each layer. This planning update does
not change Step 9's recorded validation results.
