# Prerequisites

| Topic | Why it matters | Status | Related implementation |
|---|---|---|---|
| C++ arrays and structs | Store audio and grain state | Not started | `src/audio/` |
| Sample rate | Convert time to samples | Not started | `AudioBuffer` |
| WAV files | Load source material | Not started | `WavReader` |
| Interpolation | Read fractional positions | Not started | `Interpolator` |
| Envelopes | Avoid clicks | Not started | `Envelope` |
| Resampling | Control grain pitch | Not started | `Resampler` |
| Modulation | Move grain parameters | Not started | `Modulation` |
| Scheduling | Trigger grains | Not started | `GrainScheduler` |
| Real-time audio | Produce audio without dropouts | Not started | `GranularEngine` |
| MIDI | Control the instrument | Not started | `MidiController` |
| FFT | Analyze sound | Not started | `SpectrumAnalyzer` |