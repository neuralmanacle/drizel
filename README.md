![Drizel logo](docs/assets/drizel-logo.png) Drizel

**A granular synthesizer built with C++ and JUCE.**

Drizel explores sound design through sample playback, overlapping grains, and modulation. The granular DSP engine is being implemented from scratch, with JUCE providing the application and audio plugin framework.

> **Status:** Early development. The features below describe the planned scope, not currently available functionality.

## About

Drizel is an independent audio software project developed alongside a structured study of C++ and digital signal processing. It combines practical DSP implementation with documented experiments and design decisions.

The project focuses on three goals:

- **Custom DSP:** Implement grain playback, windowing, interpolation, and scheduling without an existing granular synthesis library.
- **Real-time performance:** Design audio processing with predictable execution and careful memory management.
- **Playable sound design:** Develop an instrument with expressive modulation, MIDI control, and a dedicated interface.

## Planned capabilities

- WAV sample loading and buffered playback
- Windowed grains with interpolated sample playback
- Overlapping grain scheduling and mixing
- Playback position and pitch modulation
- Real-time audio output
- MIDI input and control
- Graphical user interface
- Standalone application and audio plugin builds

## Development roadmap

### 1. Core DSP

- [ ] Load WAV files into an audio buffer
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

See the [architecture documentation](docs/architecture.md) for design notes.

## Building the project

The current project uses **JUCE and Projucer**.

### Requirements

- JUCE with Projucer
- A compatible C++ development environment, such as Xcode on macOS
- Git for version control

### Build steps

1. Clone the repository.
2. Open the project’s `.jucer` file in Projucer.
3. Verify the JUCE module paths and select the appropriate IDE exporter.
4. Save the project and open it in your IDE.
5. Build the desired target.

Projucer regenerates the supporting project files in `JuceLibraryCode/` and `Builds/`.

## Repository structure

| Path | Contents |
|---|---|
| `src/` | Granular engine, plugin, and application source |
| `tests/` | Automated tests as they are developed |
| `experiments/` | Focused DSP prototypes and exploratory code |
| `learning/` | C++, digital audio, and DSP study notes |
| `docs/` | Architecture, design decisions, and roadmap |

Learning notes and experiments document the reasoning behind the implementation. See the [prerequisite roadmap](learning/prerequisites.md) for the study plan.

## References

- [Sean Luke — *Computational Music Synthesis*](https://people.cs.gmu.edu/~sean/book/synthesis/)
- Bjarne Stroustrup — *A Tour of C++*
- [JUCE documentation](https://juce.com/learn/documentation/)

## License

Copyright (c) 2026 Neural Manacle

Drizel is licensed under the GNU Affero General Public License,
version 3 only (AGPL-3.0-only). See [LICENSE](LICENSE).

Third-party dependencies, including JUCE, retain their respective licenses.
