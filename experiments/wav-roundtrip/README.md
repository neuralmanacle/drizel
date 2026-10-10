# Experiment 01: audio into memory and back

**Question:** Can we decode a source WAV, represent its samples in our own C++
buffer, and write it back without changing the audio?

Build and run the commands in the [project README](../../README.md#first-experiment-load-inspect-and-copy-a-wav).
Use a short mono or stereo WAV first, and give the output a new filename.

Example output for one second of stereo audio at 48 kHz:

```text
Channels: 2
Sample rate: 48000 Hz
Frames: 48000
Duration: 1.000 seconds
Verified: decoded samples match exactly.
Saved: /path/to/new-copy.wav
```

## Read the code in this order

The [commented-code guide](../../docs/code-guide.md) also covers the plugin
scaffold, C++ ownership syntax, CMake, and CI. The source files contain detailed
function contracts and explanations beside the relevant statements.

1. [`SampleBuffer.h`](../../src/audio/SampleBuffer.h): a vector owns the samples;
   channel and frame counts describe their arrangement. `const` access lets a
   future grain read the buffer without changing the source.
2. [`WavFile.h`](../../src/io/WavFile.h): two offline operations, loading and
   writing, with explicit format and failure policies.
3. [`main.cpp`](main.cpp): load, print properties, save, reload, compare. Follow
   this small program before studying the codec adapter's error handling.
4. [`AudioDataTests.cpp`](../../tests/AudioDataTests.cpp): independent WAV fixtures
   check channel order, integer scaling, and failure cases.

## What the numbers mean

A **sample** is one amplitude value in one channel. A **frame** contains one
sample from every channel at a given instant. A one-second stereo file at
48,000 Hz has 48,000 frames and 96,000 sample values.

The buffer is planar: the left channel occupies one contiguous range and the
right channel another. To read the left channel's frame 100:

```cpp
const float sample = buffer.channelData(0)[100];
```

Only access valid channel and frame indices. Duration is `frameCount / sampleRate`.
The file's sample rate tells us how its frame positions map to time; it is not
the bit depth and does not specify the future audio device's rate.

## Observations and checks

- PCM16 and PCM24 values decode exactly into 32-bit floats. Saving those decoded
  values as float32 avoids introducing another integer quantisation step.
- Different left and right test signals catch accidental channel swaps or mixing.
- The adapter detects truncated sample data explicitly because JUCE's reader may
  otherwise fill a short read with zeros.
- A successful run means sample values, frame count, channel count, and sample
  rate match. WAV bytes and metadata need not match.
- Listen to your input and copy at the same level in an external audio player.
  This experiment itself does not play sound. Automated equality checks passed;
  listening on the target machine remains a manual check.

## Next experiment

The maintainer recorded a successful real-file round trip on 10 October 2026:
stereo, 48 kHz, 2,273,072 frames, 47.356 seconds. The CLI reported exact decoded
sample equality. See the [development log](../../docs/devlog/2026-10-10.md).

Implement and test the Hann window and linear interpolation from Layer 2, then
use them to render one short grain into a new buffer and save that as a WAV.
