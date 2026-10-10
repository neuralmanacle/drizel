# Audio data and offline WAV I/O

## Scope

This first implementation supports the roadmap's offline audio-data work.
`SampleBuffer` is standard C++17; only `WavFile.cpp` depends on JUCE. The offline
CMake target is separate from the existing Projucer plugin target. No loader or
writer is called from `processBlock`.

## Representation and ownership

`SampleBuffer` owns one `std::vector<float>` with a planar layout: all frames of
channel 0, then all frames of channel 1. It records a source sample rate, one or
two channels, and a frame count. All allocated samples start at zero. There is
no resize API; assigning another buffer can replace the shape/storage. Allocation
sizes are checked for overflow. Detailed API contracts and implementation
comments are linked from the [code guide](code-guide.md).

Frames count time positions, not the total number of scalar sample values.
`durationSeconds()` is frame count divided by source sample rate. Channel
pointers avoid allocating or copying when future DSP reads the source.

Construction, copying, assignment, and destruction can allocate or free memory.
They belong outside an audio callback. Future grain voices must read a stable,
const buffer whose owner outlives them. Background loading and safe buffer
replacement are Layer 5 work; this change does not establish a concurrent
ownership scheme.

## WAV policy

| Property | Decision |
|---|---|
| Channels | Mono and stereo only; no channel mixing |
| Input encodings | 16-bit PCM, 24-bit PCM, 32-bit IEEE float WAV |
| Internal samples | Float32; finite values outside `[-1, 1]` are preserved |
| Empty audio | Loader and writer reject zero frames |
| Invalid data | Throw on invalid/unsupported headers, short sample reads, NaN, or infinity |
| Memory limit | 256 MiB of decoded float samples per load; caller can set a smaller/larger cap |
| Output encoding | Float32 WAV to preserve the decoded samples |
| Existing output | Refuse an existing file; write a temporary sibling and publish after flushing |
| Metadata | Not copied; sample data, channel count, frame count, and sample rate are preserved |
| Errors | Offline functions throw standard exceptions; CLI reports them and exits nonzero |

The default limit fits about 11.65 minutes of stereo at 48 kHz, excluding object
overhead and codec work buffers. The round-trip CLI holds the input and reloaded
copy simultaneously, so its sample storage can reach twice the per-load limit.
Files are decoded and encoded in chunks of up to 4,096 frames. Chunking bounds
codec transfer sizes; the complete decoded sample still lives in memory.

The writer requires a whole-number sample rate and checks that both sample rate
and byte rate fit JUCE's signed 32-bit header conversions. Loading and writing
involve filesystem calls, memory allocation, and exceptions, so both are strictly
offline operations. The WAV reader is JUCE's codec, not a new RIFF parser.

“Exact round trip” means equal decoded float samples and audio properties. It
does not mean a byte-for-byte copy of the WAV container or its metadata.

## Sample-rate mismatches

Keep the source sample rate at load time. Later playback will advance its source
position by:

```text
source frames per output frame = source sample rate / output sample rate
                                * 2^(pitch in semitones / 12)
```

For example, 44.1 kHz source audio rendered at 48 kHz advances by 0.91875 source
frames per output frame at its original pitch. Layer 2 will provide fractional
reads; interpolation quality and anti-aliasing require their own tests. No
resampling is performed by this milestone.

## Build and validation

CMake compiles the required JUCE modules directly, without GUI tooling or an
audio-device dependency. The development baseline is JUCE 8.0.15 at commit
`91ad83ae34a81e0833b1a2b0866f54846370ae53`. The plugin still uses Projucer.
Warnings are errors for Drizel's new C++ sources, not third-party module sources.

CTest runs a small exception-based C++ test executable with no test-framework
dependency. Its checks stay enabled in Release builds. Fixtures are generated
independently using minimal RIFF headers and known sample values, covering:

- Each supported encoding in mono and stereo at 44.1 and 48 kHz.
- Exact decoded samples and float32 round trips, including PCM extrema, channel
  order, and finite float headroom above unity.
- One-frame audio and 10,001-frame transfers spanning multiple chunks.
- Missing, empty, invalid, unsupported, and truncated files.
- A configurable decoded memory limit and a huge claimed payload rejected before allocation.
- Invalid sample rates, non-finite samples, output failures, and existing-file protection.

The very-long-file guard is tested with a small fixture that claims an excessive
length; multi-gigabyte stress tests are not part of this first milestone.

On 10 October 2026, the maintainer reported a successful CMake build and passing
CTest checks on macOS. A real stereo WAV at 48 kHz, with 2,273,072 frames
(47.356 seconds), was loaded, saved, reloaded, and verified to have identical
decoded samples. The input encoding was not recorded in the CLI output.
See the [dated development log](devlog/2026-10-10.md) for the reported result.

The maintainer also reported the starter plugin opening with its original
"Hello World!" editor. That verifies the scaffold can run on the target machine;
sample loading, audio playback, and host validation are separate future work.
Listening checks and large-file stress testing have not yet been recorded.
