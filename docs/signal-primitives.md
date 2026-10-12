# Signal primitives: Hann window and linear interpolation

This increment implements the first two DSP primitives. It does not complete
all of roadmap Layer 2 or implement a grain voice, scheduler, or live playback.

## 1. Shape amplitude with a Hann window

A grain reads a short piece of source audio. Its amplitude envelope fades the
piece in and out so the cut does not introduce an abrupt nonzero endpoint.
For a symmetric Hann table of N frames, with N >= 2:

```text
w[n] = 0.5 * (1 - cos(2*pi*n/(N-1)))
output[n] = source[n] * w[n]
```

For five frames the table is `[0, 0.5, 1, 0.5, 0]`. Multiplying by the table
keeps the middle at full level and makes the first and last frames silent.

`makeHannWindow(N)` in `src/audio/Window.h` returns an owning `std::vector<float>`.
Prepare it outside audio processing; the future grain reader only indexes the
table. Preparation allocates and computes cosines; table reads do neither.
Mirroring the first half ensures exact float symmetry. Endpoints are explicitly
zero. No peak normalisation is applied, so even lengths have a peak below one.

| Length | Policy |
|---|---|
| 0 | Empty table |
| 1 | `[1]`; no fade is possible |
| 2 | `[0, 0]`; entirely silent |
| 3 or more | Symmetric Hann, zero endpoints |

Future grain validation must account for these short cases. This primitive
does not impose a musical minimum grain length.

This table uses N-1 in the denominator. A periodic Hann uses N instead, so it
has different endpoint and overlap properties. Do not assume our even-length
table sums to one with a hop of N/2. For odd N, hop (N-1)/2 has a unit sum in
the steady overlapping region (within floating-point rounding). For example,
an N=9 table shifted by four frames has complementary overlapping weights.
Finite start/end regions and arbitrary scheduling need separate gain treatment.

## 2. Read between source samples

`readLinear(data, frames, position)` in `src/audio/Interpolation.h` borrows a
channel array; position is measured in source frames and may be fractional.
Between neighbouring values a and b, at fraction t:

```text
value = (1-t)*a + t*b
```

At position 3.25, take 75% of frame 3 and 25% of frame 4. Integer reads return
the stored sample exactly. Double intermediates prevent overflow when blending
large, opposite finite float values; audio headroom is not clipped to [-1,1].

The chosen boundary policy is **zero padding of the discrete source samples**.
The interpolation itself continues across the first and last sample:

| Position, for source `[2, 4]` | Result |
|---|---|
| -1 | 0 |
| -0.5 | 1 (zero blended with the first sample) |
| 0 | 2 |
| 0.5 | 3 |
| 1 | 4 |
| 1.5 | 2 (last sample blended with zero) |
| 2 | 0 |

Null or empty input and NaN/infinite positions produce silence. Other input
must provide `frames` readable, finite samples. Storage must stay alive and
unchanged during reading. The function performs no allocation and is `noexcept`.
It neither wraps nor holds the last value indefinitely.

## 3. Connect the primitives later

The future grain voice will hold an age and a fractional source position. Age
indexes its envelope; source position advances by a playback increment:

```cpp
const auto envelope = drizel::makeHannWindow(length); // Preparation only.
// Later, with 0 <= age < length and valid channel/storage lifetime:
const float value = drizel::readLinear(source.channelData(channel),
                                     source.frameCount(), position)
                  * envelope[age];
```

At equal source/output rates, increment 1 reads at original speed and 0.5 at
half speed. For different sample rates, the future increment is
`pitchRatio * sourceSampleRate / outputSampleRate`. Position advancement and
grain lifetime are deliberately outside these two primitives.

Linear interpolation is a starting point, not an anti-aliasing filter. It can
attenuate high frequencies, and faster playback can alias. Higher-order or
band-limited methods and listening/spectral comparisons remain roadmap work.

## Build and verify

The normal CMake build now registers `signal_primitives` with CTest, alongside
the existing audio-data and CLI-usage entries. Its executable runs ten named
cases; checks remain active in Release builds. It links only `drizel_core`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

An existing build keeps its cached `DRIZEL_JUCE_PATH`. For a fresh build, supply
`-DDRIZEL_JUCE_PATH=/path/to/JUCE` as described in the README.

The primitive suite also compiles independently of JUCE or CMake:

```sh
mkdir -p build
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Isrc \
    tests/SignalPrimitiveTests.cpp -o build/drizel_signal_tests
./build/drizel_signal_tests
```

Tests cover reference windows, 1,024 window lengths, overlap convention,
integer and fractional reads, zero padding, one-frame/empty inputs, invalid
positions, finite headroom, stereo channels, and forward/reverse traversal.
Read this note, then `Window.h`, `Interpolation.h`, and their tests in that order.
