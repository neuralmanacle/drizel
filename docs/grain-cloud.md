# Step 5: overlap grains into a texture

Step 4 produced one windowed grain. Step 5 launches many instances of that
same player at different times and adds their stereo output frames together.
This implements the first offline **Layer 4: Many grains** milestone, not
Layer 5 real-time plugin integration.

## The idea

At 48 kHz, density 20 means a nominal launch every 2,400 output frames (50 ms).
A 100 ms grain remains alive when the next one begins. At any output frame:

```
left[n]  = sum(each active grain's left output at n)
right[n] = sum(each active grain's right output at n)
```

Each grain still owns its own read position and window age. One call to
`cloud.next()` advances the entire cloud by **one stereo frame**. Overlap adds
amplitudes at the same moment; it does not concatenate grain recordings.
Nominal average overlap is density * grainSeconds: 20 * 0.1 = 2.

| Part | Purpose |
| --- | --- |
| `CloudSettings` | Units and ranges for a prepared render |
| `prepare()` | Validate, scan source peak, allocate three windows, seed randomness |
| Scheduler countdown | Decide when to launch; retain fractional frame timing |
| `std::array<Grain, Capacity>` | Reusable slots, 64 by default |
| `launch()` | Draw variations and start one inactive slot |
| Mixer in `next()` | Sum the existing Grain outputs into one stereo frame |
| `next(false)` | Disable new launches while existing grains finish |

## Build and listen

Use the existing JUCE setup. Build in Release for useful timing numbers:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/drizel_grain_cloud input.wav sparse.wav 5 5 500 42
./build/drizel_grain_cloud input.wav medium.wav 5 20 500 42
./build/drizel_grain_cloud input.wav dense.wav 5 80 500 42
```

Arguments are input, new output filename, trigger seconds, density, centre
start milliseconds, and seed. Select a start inside your recording; replace
500 with 0 for a short source. Existing output files are refused.
The output contains the trigger duration plus 125 ms to finish all grains.
Defaults: 48 kHz stereo; grain lengths 75/100/125 ms; position +/-50 ms;
pitch +/-3 semitones; pan +/-0.75; interval jitter +/-20%; linear interpolation.
`CloudSettings` exposes the remaining controls in C++, including Hermite.
To specify an interval T seconds, set density to 1/T.

Listen for sparse separated events versus a sustained texture at higher density,
smooth grain ends, stereo spread, and natural completion of the last grains.
The same seed and settings reproduce the same render in the same build.
Cross-platform bit identity is not promised: pow/sqrt/cos can round differently.

## Reading the code, in order

Declarations and processing statements carry adjacent explanations. C++ details:

1. `template <std::size_t Capacity = 64>` makes capacity a compile-time value.
   `std::array` stores grain objects inline; it cannot resize itself.
2. `const SampleBuffer&` borrows the original source. `prepare` scans it and
   creates Hann tables before playback. `source_` becomes non-null last, so an
   exception leaves playback safely stopped. The source must outlive the cloud's
   final rendering call and remain unchanged.
3. Copy/move functions are `= delete` because live grains borrow this object's
   window addresses. This is a lifetime constraint, not a DSP computation.
4. `interval_ = outputRate / density` has units of output frames per grain.
   `countdown_ += interval` preserves fractional error. Subtracting one per call
   advances time by one frame. Events are quantized to output frames, never early.
5. The explicit unsigned LCG wraps modulo 2^32. `unit()` maps its state to [0,1),
   and `bipolar()` maps to [-1,1). This is a simple non-cryptographic texture RNG.
6. Length selects one of three precomputed tables; it is discrete variation.
   Position, pitch and pan use continuous random values. Starts and pans clamp
   to legal ranges, so distribution can accumulate at the edges. Source reads
   after a grain crosses a boundary retain the existing zero-padding behavior.
7. `2^(semitones/12) * sourceHz/outputHz` converts pitch and sample-rate ratio
   into source frames read per output frame. Pitch changes traversal speed,
   while length independently controls the envelope's lifetime.
8. The range-for loop `auto& grain` refers to each stored object without copying.
   The first inactive slot is reused. If all are busy, the new event is dropped.
   Existing grains finish naturally; random draws still advance for dropped events.
9. The mixer calls each grain once. `double` accumulation reduces rounding error;
   `static_cast<float>` converts the final sum to the existing sample format.
10. `noexcept` on playback promises no escaping exceptions. Playback has no
    allocations, locks, file I/O or console calls. It still performs trigger-time
    pow/sqrt work and bounded scans; real-time timing needs a later target audit.

## Gain policy and limits

Use a conservative overlap bound rather than dividing by the current active count:

```
minimumHop = max(1, nominalHop * (1 - jitter))
overlapBound = min(capacity, ceil(longestLength / minimumHop))
grainGain = masterGain / (overlapBound * max(1, sourcePeak) * readerBound)
```

`readerBound` is 1 for linear and 1.25 for the current cubic Hermite reader.
The Hermite bound follows its four coefficient absolute values; their sum is
1 + t*(1-t), at most 1.25. Prepared Hann values and pan factors are at most one.
Thus the output peak is bounded by masterGain in exact arithmetic, with float
rounding tolerance in tests. This is conservative headroom, not a limiter or
perceived-loudness normalization. Dense renders can sound quieter than expected.
Finite source samples above 1e30 are rejected to avoid float intermediate overflow.
Changing source data after preparation invalidates the bound and is forbidden.

- Settings are snapshots; no live edits, parameter smoothing or GUI integration.
- Source and window preparation allocates and may throw; `next` does neither.
- Capacity overflow drops new events; the CLI reports drop counts and peak usage.
- High pitches still alias: neither existing reader is a band-limited resampler.
- Three windows trade a small preparation cost for allocation-free length variation.
- Timing diagnostics are offline averages, not worst-case callback guarantees.
- The CLI limits triggering to 60 seconds and density to 1,000 grains/second.
  The core supports up to one event per output frame, with pool limits enforced.

## Validation on 19 October 2026

GCC 13.3, C++17, Linux, Release, JUCE 8.0.15: all seven CTest targets passed.
The new core tests also passed AddressSanitizer and UndefinedBehaviorSanitizer
(leak detection disabled for this execution environment). Tests cover explicit
window-sum mixing, integer/fractional launch timing, overflow accounting,
tail draining, peak headroom, same/different seeds, and failed preparation.

Generated a 24 kHz mono 440 Hz source and rendered 5 seconds plus 125 ms tail
at 48 kHz stereo. Every output had 246,000 frames, finite bounded samples, and
silent endpoints. Illustrative timings from this container, including output
writes to memory and peak tracking but excluding WAV I/O and preparation:

| Density | Started | Dropped | Maximum active | Peak | Render seconds |
| --- | ---: | ---: | ---: | ---: | ---: |
| 5 | 25 | 0 | 1 | 0.542546 | 0.0127 |
| 20 | 100 | 0 | 3 | 0.216426 | 0.0168 |
| 80 | 398 | 0 | 11 | 0.138624 | 0.0285 |
| 1000 | 3169 | 1832 | 64 | 0.101508 | 0.1274 |

Jitter makes the number of triggers fluctuate around density * seconds.
These measurements are not a worst-case per-block or per-grain CPU audit.
Listening on the target Mac, target CPU profiling, and plugin integration
remain open. The broader roadmap Layer 4 exit criteria are not all closed.
