![Drizel logo](docs/assets/drizel-logo.jpg)

# Drizel

**Granular synthesis instrument and real-time audio effect by Neural Manacle.**

Drizel transforms samples and live audio into evolving textures through a custom C++ granular DSP engine built with JUCE.

[Download releases](https://github.com/neuralmanacle/drizel/releases) · [Development roadmap](docs/roadmap.md)

## Plugins

| Plugin | Purpose |
| --- | --- |
| Drizel | Real-time granular processing on DAW audio tracks |
| Drizel Instrument | MIDI-driven granular synthesis on instrument tracks, with or without a loaded sample |

## Features

- Live audio granulation and WAV sample loading
- Overlapping windowed grains with interpolated playback
- Position, pitch, grain density and output gain controls
- LFO modulation and MIDI control
- Sample-aware grain selection: Manual, Attack, Sustain, Avoid silence and Bright
- Preset/state recall and collapsible virtual keyboard

## Installation

Download the macOS VST3 package from [Releases](https://github.com/neuralmanacle/drizel/releases) when available. Extract both plugin bundles into `~/Library/Audio/Plug-Ins/VST3/` and rescan plugins in your DAW.

Use **Drizel** on an audio track and **Drizel Instrument** on a MIDI track. Release-specific compatibility and installation requirements are listed with each release.

## Build from source

Requires CMake 3.22+, a C++17-compatible toolchain and a JUCE source checkout. On macOS, install Xcode Command Line Tools.

```sh
git clone https://github.com/neuralmanacle/drizel.git
cd drizel

# Skip if you already have a JUCE checkout.
git clone --depth 1 --branch 8.0.15 https://github.com/juce-framework/JUCE.git ../JUCE

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DDRIZEL_BUILD_PLUGIN=ON -DDRIZEL_JUCE_PATH=../JUCE
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Adjust `DRIZEL_JUCE_PATH` to your JUCE checkout. CMake builds separate instrument and effect targets in VST3 and Standalone formats, plus AU on macOS. Plugin bundles are under `build/DrizelPlugin_artefacts/` and `build/DrizelInstrument_artefacts/`.

## Development

The current source version is **1.0.0**. Final release build, host validation and distribution checks remain separate from implementation status.

Future research includes audio ML, promptable modulation and dedicated embedded hardware. Experimental features are not part of the current release scope.

- [Architecture](docs/architecture.md)
- [Development roadmap](docs/roadmap.md)
- [DSP experiments](experiments/README.md)
- [Sample analysis](experiments/sample-analysis/README.md)
- [Promptable modulation research](docs/promptable-modulation.md)
- [Interface design](docs/plugin-design-ui.md)
- [Learning curriculum](learnings/audio-dsp-ml-curriculum.md)

## License

Copyright © 2026 Neural Manacle.

Drizel is licensed under [AGPL-3.0-only](LICENSE). Third-party dependencies retain their respective licenses; WAV excerpts are subject to the terms in the [corpus manifest](experiments/sample-analysis/corpus/manifest.json).
