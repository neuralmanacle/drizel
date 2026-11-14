# Offline grain-scan policies

Layer 13.1 asks whether three editable grain-position patterns produce
recognizably different movement on the same source. This experiment uses
Drizel's C++ grain voice and renders 100 ms Hann grains at 20 launches per
second. Each pattern uses the same source, region, pitch, pan and grain gain.

- Cyclic scans forward and wraps after each period.
- Stepped repeats +1, −1, −1 with a configurable step size and interval.
- Irregular uses a fixed seed to vary both step distance and dwell time.

All three use wrap boundaries. The scan region bounds grain starts. A grain
can read past that region as it plays. The experiment uses direct settings.
The plugin integration and interpretation of natural-language prompts are
future Layer 13 work.

## Reproduce on macOS

From the repository root, after building the existing `drizel_wav` CMake
target into `build/`:

```sh
clang++ -std=c++17 -O2 -I src experiments/llm-control/render.cpp \
  build/libdrizel_wav.a -framework Accelerate -framework AudioToolbox \
  -framework CoreAudio -framework CoreMIDI -framework Foundation \
  -framework Cocoa -framework IOKit -framework Security \
  -framework QuartzCore -framework CoreMedia -o build/drizel_scan_render
python3 experiments/llm-control/scan.py path/to/pcm16.wav \
  build/drizel_scan_render build/scan-preview
```

The output directory contains three stereo WAVs and a JSON trajectory per
pattern in `trajectories.json`. Use a fresh output directory for another run.
Use `--help` to see the region, period, step and seed controls.

The first run used the opening eight seconds of the local Desktop guitar WAV,
converted to PCM16. The scan covered 5–90% of the source and used a two-second
cyclic period. The stepped scan moved by 18% of the region every 250 ms.
The irregular seed was 12013. Two runs produced byte-identical audio and
trajectories. The guitar recording stays local. In the first listening pass,
the listener could hear all three movements clearly and would use all three.
This feedback covers one guitar excerpt at the initial settings.

## Preview a movement prompt

The local phrase matcher covers the three roadmap examples and a few
explicit paraphrases. The preview prints a versioned JSON policy that you
can edit before applying. Unknown phrases return an error.

```sh
python3 experiments/llm-control/prompt.py preview \
  'One step forward, two steps back' > build/stepped-policy.json
# Edit build/stepped-policy.json, then render the selected pattern.
python3 experiments/llm-control/prompt.py apply build/stepped-policy.json \
  path/to/pcm16.wav build/drizel_scan_render build/stepped-preview
```

Region and step size are normalized fractions. Period and render duration
are in seconds. The step interval is measured in grain launches, with
20 launches per second. The preview uses the same defaults as the direct
render. Only settings relevant to the selected pattern affect its movement.

The next interface experiment adds a typed command field, push-to-talk
transcription, and a shared editable preview. The first combined command
will prototype stepped movement with seeded variation in distance and
timing. See the [design proposal](../../docs/promptable-modulation.md#typed-and-spoken-commands).

## Combine stepped movement with variation

```sh
python3 experiments/llm-control/prompt.py preview \
  'Make the grain more chaotic and stepped' > build/chaotic-stepped.json
python3 experiments/llm-control/prompt.py apply build/chaotic-stepped.json \
  path/to/pcm16.wav build/drizel_scan_render build/chaotic-stepped-preview
```

The version 2 policy keeps the forward, back, back direction sequence.
Edit `distance_variation` and `timing_variation` to change the fractional
variation around the base step and interval. Both accept 0 through 0.9.
The seed controls repeatability. Zero variation reproduces fixed steps.
Without an active policy the example starts at 0.30 for each setting.
Pass `--active path/to/policy.json` to preview an edit to an existing policy.
For a command containing “more”, this prototype adds 0.15 to each variation
setting up to 0.9, preserving the region and base settings. Review the
proposed JSON before applying. Version 1 policies remain valid.

The interpreter matches a small set of typed phrases. Rendering runs offline.

## Live command and recall check

In the CMake-built VST3 or standalone app, load a WAV and use the Movement
command field. Preview “make the grain more chaotic and stepped,” edit the
proposed settings, then Apply. Save and reload a preset to restore the numeric
policy. Motion restarts from its initial phase and fixed seed after state recall,
including recall of the same policy. Changing the active policy also restarts motion. Older presets load with Manual scan.

The macOS standalone build also has Hold to talk. Grant the system speech and
microphone permissions on first use, then hold again to record. Release to
transcribe. Review the editable text and use Preview and Apply as with typed
commands. Recognition uses the system's on-device recognizer when available.
The VST3 uses typed commands. Live DAW listening and microphone capture on a
running standalone instance still need verification.

### Preview validation

A preview belongs to the sample or preset request active when it was
created. Requesting another sample or recalling state invalidates pending
previews and speech results. Invalid settings leave the active policy
playing. Use Preview again after a successful Apply to make another edit.

The Mac VST3 and standalone builds passed all 11 CTest entries, including
processor checks for invalid and stale previews and preset recall. A live
acceptance pass should check that Preview leaves playback alone, Apply
changes movement, preset recall restores it, and changing the sample
invalidates a pending preview. In standalone, use the same phrase in the
typed field and through Hold to talk, then compare the editable settings.
