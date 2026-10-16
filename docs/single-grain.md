# Step 4: one grain, rendered offline

Step 3 supplied a window and fractional sample readers. Step 4 joins them into
one reusable player. This corresponds to **Layer 3** of the dependency roadmap;
it is not the many-grain scheduler in Layer 4.

For output frame n:

```
position = start + n * rate
rate = 2^(semitones / 12) * sourceSampleRate / outputSampleRate
left[n]  = interpolate(sourceLeft, position)  * window[n] * leftGain
right[n] = interpolate(sourceRight, position) * window[n] * rightGain
```

A 100 ms grain at 48 kHz lasts 4,800 output frames at every pitch. At +12
semitones it reads twice as fast, traversing twice as much source audio.
At 24 kHz source / 48 kHz output, unshifted playback advances 0.5 source frames
per output frame. Window age always advances by one.

## Build and listen

Use the existing README toolchain setup, including a JUCE checkout:

```sh
cmake -S . -B build -DDRIZEL_JUCE_PATH=../JUCE
cmake --build build
ctest --test-dir build --output-on-failure
./build/drizel_single_grain input.wav grain-normal.wav 0 100 0 0 linear
./build/drizel_single_grain input.wav grain-up.wav 0 100 12 0 hermite
./build/drizel_single_grain input.wav grain-down.wav 0 100 -12 0 hermite
```

Arguments after the output filename are start milliseconds, length milliseconds,
semitones, pan (-1 left to +1 right), and interpolation mode. Output is stereo,
48 kHz, float32, with gain 0.5. Existing output paths are refused. Choose a start
inside an audible part of your input. Listen for smooth ends and the expected
pitch changes; longer grains make pitch easier to hear.

## Code reading guide

Every declaration and processing statement in `src/audio/Grain.h` and the
experiment has an adjacent explanation. Read these in order:

1. `GrainFrame`: two float values returned by value; `{}` produces silence.
2. `start`: `const SampleBuffer&` borrows the source, `const float*` borrows
   the window, and `std::size_t` stores frame counts. `noexcept` promises no
   escaping exceptions. Validation stops any old grain before rejecting a
   new trigger. Assignment changes the existing fixed-size state.
3. Pan gains: mono uses square roots of `(1-pan)/2` and `(1+pan)/2`, so squared
   gains sum to one. Centre attenuates each mono output by approximately 3 dB.
   Stereo uses balance: centre leaves channel levels unchanged; hard pan
   mutes the opposite channel. It does not mix stereo channels together.
4. `active`: a `const` accessor cannot change state. Remaining frames replace
   a separate boolean, avoiding contradictory active/finished states.
5. `next`: early return avoids dereferencing stale pointers after completion.
   Read source, multiply by envelope and pan gain, advance age and position,
   decrement duration, return the frame. Neither interpolation mode allocates.
6. Private members: trailing underscores identify stored state. Pointers do
   not own memory, so grain copies share source/window storage. Default member
   initialisers make a newly constructed grain inactive.
7. Experiment: parse complete finite numbers, check bounds, load source,
   allocate table/output, render, then write WAV. `try`/`catch` reports failures
   on the command-line thread. `auto` infers a type; `static_cast` explicitly
   converts units/count types; `++` advances a frame counter. CMake links this
   executable to the existing WAV adapter and registers the grain tests.
8. Tests: `check` throws on failure even in release builds; `near` tolerates
   floating-point rounding. Loops cover both readers and multiple rates.
   `static_assert` enforces trivial copying at compile time. Runtime checks
   cover duration, boundaries, sample-rate correction, panning and invalid
   retriggers. Test exceptions are reported as a nonzero process exit status.

## Contracts and limitations

- Keep source and window storage alive and unchanged until rendering ends.
  The caller supplies at least `length` window entries, each finite in [0,1].
  Source samples must be finite. The player cannot validate raw-pointer capacity.
- Grain lifetime is exactly `length` frames, **not** that many nonzero samples.
  Hann endpoints and out-of-source reads are silent. A one-frame Hann is unity;
  a two-frame Hann is silent. The CLI requires at least three frames.
- Source boundaries use the existing interpolation zero-padding policy. Negative
  rates and zero rates work in the core; the CLI provides positive pitch ratios.
- Start/reset and per-frame rendering allocate nothing. Window preparation,
  source loading, output allocation and WAV writing happen outside that path.
  This does not yet constitute a real-time plugin integration or timing audit.
- Hermite can overshoot. Neither reader is a band-limited resampler: raising
  pitch may alias. Gain 0.5 is headroom, not a peak guarantee or limiter.
- No scheduler, pool, overlap mixer or GUI controls are added in this step.

## Validation recorded here

The grain checks pass with GCC warnings treated as errors and AddressSanitizer/
UndefinedBehaviorSanitizer enabled (leak detection disabled because this runtime
cannot inspect process threads). All 12 existing signal primitive tests pass.
The grain state occupies 80 bytes on this tested 64-bit build and is trivially
copyable. Listening on the target Mac remains a manual check.

Full CMake/JUCE 8.0.15 build also passed, followed by all five CTest targets.
A generated 24 kHz mono 440 Hz source was rendered to 48 kHz stereo at 0, +12,
and -12 semitones. Each WAV contained 4,800 frames, only finite samples, and
exactly silent endpoints. Positive zero crossings over 100 ms were 44, 88 and
22 respectively, matching the expected pitch changes. These are numerical
checks, not a subjective listening evaluation.
