# 22 October 2026 — Step 6: real-time engine

Connected the Step 5 grain cloud to the JUCE processor. Added background WAV
loading, lock-free ownership handoff, worker-side reclamation, smoothed position,
pitch, density, gain and play controls, and a small loading/control panel.
Sample changes fade through silence. Host state saves controls and the source
path. CMake can optionally build Standalone/VST3/AU; Projucer includes the WAV
adapter and core headers. CI now also requests plugin builds on macOS.

Core tests cover rate changes, irregular frame groups, concurrent swaps,
finite output, mute smoothing and allocation/deletion guards. See
[the implementation guide](../realtime-engine.md) for ownership, build steps,
trade-offs and validation limitations. Physical-device listening and worst-case
callback timing remain open; this does not mark Layer 5's exit criteria complete.
