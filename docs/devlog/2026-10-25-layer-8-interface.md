# 25 October 2026 — Layer 8 interface increment

Replaced the generic editor with six APVTS-bound sliders, LFO shape/destination
choices and a Play toggle. Added asynchronous WAV chooser and single-file WAV
drag-and-drop, a background-decoded waveform, base-position marker, active-grain
count and latest-block output-peak display with UI decay. No DSP defaults or
parameter IDs changed.

The loader publishes a path/revision snapshot for waveform refresh, including
same-path reloads and restored host state. Failed loads retain the previous
waveform. The snapshot describes prepared audio; it can precede the audio swap
fade completing. UI code does not borrow the audio-owned sample. The callback
publishes only lock-free numeric telemetry and scans the fixed voice pool once
per block for activity. This bounded scan remains covered by allocation guards.

Validation in this Linux environment:

- Debug offline CMake build against JUCE 8.0.15 succeeded.
- CTest: 9/9 passed, including new unloaded/playing/restart telemetry assertions
  and existing real-time allocation/deallocation guards.
- Editor and processor passed C++17 syntax-only compilation against JUCE 8.0.15
  using a temporary module header; this is not a plugin link/build test.
- `git diff --check` passed.
- Full plugin CMake configuration failed because pkg-config is missing; native
  GUI/audio development headers are also absent. Installing prerequisites was
  blocked by this environment's package-manager permissions.

No target-Mac build, GUI rendering/listening, two-host state roundtrip, plugin
validator, callback deadline measurement or release packaging was performed.
Layer 8 remains open. See [the interface guide](../step-8-interface.md) for user
instructions, meter semantics, thread ownership and manual acceptance checks.
