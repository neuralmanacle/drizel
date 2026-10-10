# ![Drizel logo](docs/assets/drizel-logo.jpg) 

# Drizel

**A granular synthesizer built with C++ and JUCE.**

Drizel explores sound design through sample playback, overlapping grains, and modulation. The granular DSP engine is being implemented from scratch, with JUCE providing the application and audio plugin framework.

An experimental part of the vision is **promptable modulation**: describe how
the sound should move, then let a model translate that intention into an editable
modulation policy. Grain scanning is the first proposed use case.

> **Status — 10 October 2026:** The first offline WAV loading and saving milestone
> has passed automated tests on the maintainer's Mac and a real-file round trip.
> Grain synthesis, live playback, the sample-loading UI, and promptable modulation
> are still planned. See the [development log](docs/devlog/2026-10-10.md).

## About

Drizel is an independent audio software project developed alongside a structured study of C++ and digital signal processing. It combines practical DSP implementation with documented experiments and design decisions.

The project focuses on four goals:

- **Custom DSP:** Implement grain playback, windowing, interpolation, and scheduling without an existing granular synthesis library.
- **Real-time performance:** Design audio processing with predictable execution and careful memory management.
- **Playable sound design:** Develop an instrument with expressive modulation, MIDI control, and a dedicated interface.
- **Promptable behaviour:** Explore natural-language control of modulation while keeping the instrument usable with conventional controls alone.

## Planned capabilities

- WAV sample loading and buffered playback
- Windowed grains with interpolated sample playback
- Overlapping grain scheduling and mixing
- Playback position and pitch modulation
- Real-time audio output
- MIDI input and control
- Graphical user interface
- Standalone application and audio plugin builds
- Optional promptable modulation policies (experimental): a model interprets
  descriptions of movement, and local modulation sources execute the accepted
  behaviour outside the model's inference loop

## Promptable modulation: the research direction

A musician could describe how the grain scan should behave instead of manually
building every modulation pattern. A **policy** means a rule for how a parameter
evolves over time. These are proposed interpretations to test:

| Prompt | Possible grain-scan behaviour |
|---|---|
| "very chaotic" | Irregular, seeded movement within a chosen sample region, with adjustable depth and speed |
| "go around in circles" | Repeat a scan through the selected region, wrapping at its end |
| "one step forward two step back" | Repeat one forward step followed by two backward steps, at a chosen interval |

The intended interface would show the interpreted pattern and expose its rate,
depth, region, and boundary behaviour for editing. A local modulation source
would run the accepted policy; model inference would stay off the audio thread.
The existing DSP and conventional controls would remain usable with the model
disabled. No model architecture or provider has been selected.

See the [promptable modulation proposal](docs/promptable-modulation.md) for the
scope, example interpretations, execution design, and experiments still needed.

## Development roadmap

### 1. Core DSP

- [x] Load WAV files into an audio buffer (offline, mono/stereo PCM16, PCM24, float32)
- [ ] Generate a single windowed grain
- [ ] Implement interpolation for variable-rate playback
- [ ] Schedule and mix overlapping grains

### 2. Real-time engine

- [ ] Integrate the engine with real-time audio processing
- [ ] Add playback position and pitch modulation
- [ ] Validate processing behaviour and performance

### 3. Instrument and interface

- [ ] Add MIDI support
- [ ] Build the graphical interface
- [ ] Package and test standalone and plugin builds

### 4. Promptable modulation (experimental)

- [ ] Define editable modulation policies for grain scan: irregular movement, cyclic scans, and directional step patterns
- [ ] Run small studies in `experiments/` on which control tasks a model can handle and what prompting it needs
- [ ] Translate prompts into a bounded policy schema and preview the interpretation before applying it
- [ ] Execute accepted policies locally, with repeatable seeds and saved settings
- [ ] Measure inference latency and jitter from the target deployment location
- [ ] Analyse samples offline (onsets, loudness, spectral features) to give the model a semantic map
- [ ] Add a lock-free modulation queue with timestamped breakpoints and lookahead
- [ ] Run inference on a worker, including network requests if a hosted model is used; keep local modulation running when a response is late

See the [development roadmap](docs/roadmap.md) for further detail.

## Proposed architecture

The engine reads from a shared sample buffer. A scheduler starts grain voices, each with its own playback position, rate, and envelope. Active grains are summed to produce the output.

| Component | Responsibility |
|---|---|
| Sample buffer | Store decoded source audio |
| Grain scheduler | Determine when grains start |
| Grain voices | Maintain each grain’s playback state |
| Interpolation | Read samples at fractional positions |
| Window envelope | Shape each grain’s amplitude |
| Overlap-add mixer | Sum active grain outputs |
| Output stage | Apply output gain and any future effects |
| Modulation sources | LFOs, MIDI, and local execution of optional prompt-derived policies feeding smoothed parameters |
| Policy interpreter | Translate a prompt into validated, editable modulation settings outside the audio thread |

The proposed model integration keeps inference off the audio thread. Accepted
policies run locally without waiting for another model response. For policies
expressed as finite trajectories, a worker would prepare timestamped points
ahead of playback; late updates would leave the current local policy running.

See the [architecture documentation](docs/architecture.md) for design notes.

## Building the project

The plugin uses **JUCE and Projucer**. The first offline experiment has a separate
**CMake** build so audio-data code can be tested without a DAW or GUI.

### First experiment: load, inspect, and copy a WAV

Requirements: a C++17 compiler, CMake 3.22 or newer, and a JUCE source checkout.
The offline build is tested against **JUCE 8.0.15** (commit
`91ad83ae34a81e0833b1a2b0866f54846370ae53`). It compiles only JUCE's core,
audio-basics, and audio-formats modules.

From the Drizel repository root:

```sh
# Skip this clone if you already have a JUCE source checkout.
git clone --depth 1 --branch 8.0.15 https://github.com/juce-framework/JUCE.git ../JUCE

cmake -S . -B build -DDRIZEL_JUCE_PATH=../JUCE -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure

./build/drizel_wav_roundtrip "/path/to/input.wav" "/path/to/new-copy.wav"
```

Set `DRIZEL_JUCE_PATH` to your existing JUCE directory if it is elsewhere.
These commands use CMake's default Makefiles/Ninja-style output layout on macOS
and Linux. On macOS, install Xcode Command Line Tools and CMake first if needed.
For a multi-configuration generator, add `--config Debug` to the build command,
`-C Debug` to CTest, and find the executable in `build/Debug/`.

The experiment prints channel count, sample rate, frame count, and duration. It
writes a **32-bit float WAV**, reloads it, and verifies every decoded sample.
Choose a new output filename: existing files are rejected. This preserves audio
samples and sample rate, but does not preserve the original encoding or metadata.
The default decoded sample memory limit is 256 MiB per load.

Read [the first experiment](experiments/wav-roundtrip/README.md) for a guided
walkthrough and [the audio-data design](docs/audio-data.md) for the decisions.
For detailed function comments, C++ ownership explanations, and a file-by-file
reading order, start with [the commented-code guide](docs/code-guide.md).

### Plugin requirements

- JUCE with Projucer
- A compatible C++ development environment, such as Xcode on macOS
- Git for version control

### Plugin build steps

1. Clone the repository.
2. Open the project’s `.jucer` file in Projucer.
3. Verify the JUCE module paths and select the appropriate IDE exporter.
4. Save the project and open it in your IDE.
5. Build the desired target.

Projucer regenerates the supporting project files in `JuceLibraryCode/` and `Builds/`.

The `.jucer` file explicitly lists the repository files in its project tree,
including `src/audio`, `src/io`, experiments, tests, and documentation. Adding a
file on disk does not automatically register it with Projucer. Add future files
through its File explorer's **+** or group context menu, then save the project
to update the exported IDE project.

The offline adapter, experiment, and test `.cpp` files are listed for browsing
with **Compile disabled**. They still build through the CMake commands above.
In particular, the experiment and test runner each define their own `main()`
and must not be compiled into the plugin target. Only `PluginProcessor.cpp` and
`PluginEditor.cpp` currently compile as Drizel's plugin source files.

## Repository structure

| Path | Contents |
|---|---|
| `src/` | Granular engine, plugin, and application source |
| `src/audio/` | Framework-independent audio data and future DSP |
| `src/io/` | Offline file I/O adapters |
| `tests/` | Command-line audio-data tests, run through CTest |
| `experiments/` | Focused DSP prototypes and exploratory code |
| `experiments/llm-control/` | Studies on model-driven modulation: prompts, latency, failure modes |
| `learnings/` | C++, digital audio, and DSP study notes |
| `docs/` | Architecture, design decisions, and roadmap |
| `docs/devlog/` | Dated progress reports with validation results and remaining work |

Learning notes and experiments document the reasoning behind the implementation. See the [prerequisite roadmap](learnings/prerequisites.md) for the study plan.

## References

- [Sean Luke — *Computational Music Synthesis*](https://people.cs.gmu.edu/~sean/book/synthesis/)
- Bjarne Stroustrup — *A Tour of C++*
- [JUCE documentation](https://juce.com/learn/documentation/)

## License

Copyright (c) 2026 Neural Manacle

Drizel is licensed under the GNU Affero General Public License,
version 3 only (AGPL-3.0-only). See [LICENSE](LICENSE).

Third-party dependencies, including JUCE, retain their respective licenses.
