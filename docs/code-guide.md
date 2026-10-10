# Reading the commented code

The source files contain the detailed documentation: class/function contracts,
parameter meanings, ownership rules, failure paths, and comments explaining the
implementation choices. This guide gives a reading order and connects the files.
It describes the current offline WAV milestone and existing JUCE starter plugin.

## Reading order

| Order | File | What to follow |
|---|---|---|
| 1 | [`src/audio/SampleBuffer.h`](../src/audio/SampleBuffer.h) | Constructor, planar storage, frame counts, const/writable access |
| 2 | [`src/io/WavFile.h`](../src/io/WavFile.h) | Public load/write contracts before the implementation details |
| 3 | [`experiments/wav-roundtrip/main.cpp`](../experiments/wav-roundtrip/main.cpp) | Complete load, inspect, save, reload, compare sequence |
| 4 | [`src/io/WavFile.cpp`](../src/io/WavFile.cpp) | Codec stream ownership, conversion, chunks, validation, temporary output |
| 5 | [`tests/AudioDataTests.cpp`](../tests/AudioDataTests.cpp) | Independent WAV fixtures, assertions, edge cases, test runner |
| 6 | [`src/PluginProcessor.h`](../src/PluginProcessor.h) and [implementation](../src/PluginProcessor.cpp) | Host callbacks, bus layout, audio boundary, current placeholders |
| 7 | [`src/PluginEditor.h`](../src/PluginEditor.h) and [implementation](../src/PluginEditor.cpp) | Borrowed processor reference, constructor, painting, layout |
| 8 | [`CMakeLists.txt`](../CMakeLists.txt) | Libraries, module dependencies, compiler settings, executables, CTest |
| 9 | [`drizel.jucer`](../drizel.jucer) | Separate Projucer plugin configuration and generated project files |
| 10 | [`.github/workflows/offline-audio.yml`](../.github/workflows/offline-audio.yml) | Linux/macOS jobs, dependency pin, configure/build/test steps |

## How to read the comments

`/** ... */` blocks document the API next to its declaration or definition. The
`@brief`, `@param`, `@return`, `@throws`, `@pre`, and `@warning` tags distinguish
purpose, inputs, outputs, errors, caller requirements, and usage limits. They
can be read directly in an editor; there is no required documentation generator.
Short `//` comments explain implementation decisions where they happen.

A **precondition** is the caller's responsibility. For example, `channelData(1)`
requires a stereo buffer. An assertion helps during debugging, but an invalid
index is not a supported input that returns an error result. Keep frame indices
within the returned channel array too.

## The offline execution path

The experiment's `main` validates two path arguments, calls `loadWav`, reports
properties, writes the new file, loads it again, and compares the two buffers.
All these operations run on the executable's main thread. That thread is an
ordinary offline thread here, not a real-time audio callback.

`loadWav` gives a JUCE reader ownership of an input stream, validates its decoded
format and length, allocates a `SampleBuffer`, and fills channel slices in chunks
of up to 4,096 frames. It checks actual read completion and rejects non-finite
values. The returned buffer owns the audio independently of the now-closed file.

`writeWav` checks the source data, writes a float32 temporary sibling, finalises
its header, checks the file stream, closes it, and moves it into place. It uses
the original sample rate and does not normalise, clip, downmix, or resample.
Its header/API comments also explain the limits of destination-existence checks.

For mono audio there is one value per frame; for stereo there are two. One
second at 48 kHz therefore has 48,000 frames whether mono or stereo. The stereo
buffer owns 96,000 float values, arranged as one complete left channel followed
by one complete right channel. The WAV adapter converts between that planar
layout and the file's interleaved frames.

## C++ concepts used in the implementation

| Syntax or type | Meaning in this project |
|---|---|
| Constructor initialiser list | Initialises dimensions/base classes/references before the body runs |
| `std::vector<float>` | Owns a dynamic allocation containing all source samples |
| `std::array<float*, 2>` | Fixed-size collection of borrowed channel pointers, not a second audio buffer |
| `const SampleBuffer&` | Borrows a buffer without copying and prevents modification through that reference |
| `const float*` | Permits reading samples through a pointer; does not own their storage |
| `method() const` | Allows the call on a const object and prevents ordinary member mutation through `this` |
| `noexcept` | Declares that an operation does not propagate exceptions; it does not check indices |
| `std::unique_ptr<T>` | One owner automatically destroys a heap object at scope exit |
| `.get()` | Borrows the managed pointer without changing ownership |
| `.release()` | Releases ownership without deleting; used with JUCE's reader factory contract |
| `std::move(...)` | Allows a move operation, such as transferring a `unique_ptr`; the cast alone does not move bytes |
| `.reset()` | Destroys the currently owned object; borrowed pointers to it become invalid |
| `static_cast<T>(...)` | Makes a conversion explicit; validation is still required before narrowing |
| `[&]` lambda capture | Borrows surrounding values; test callables run before those borrowed values expire |
| `override` | Verifies that a derived method matches a virtual base-class method |
| `final` | Prevents further derivation from `CheckedInputStream` |
| `#if`/`#ifndef` | Selects code at compile time from generated JUCE configuration |
| `namespace { ... }` | Limits helper names to the current source translation unit |

The default copy/move operations of `SampleBuffer` are distinct from the explicit
ownership transfers of codec streams. Copies duplicate audio data. Moves can
transfer vector storage without updating the scalar fields of the source
object; do not use the moved-from buffer for sample access. The comments specify
that it should only be reassigned or destroyed.

## Ownership and thread boundaries

| Object | Owner | Lifetime/use rule |
|---|---|---|
| Decoded samples | `SampleBuffer`'s vector | Keep the buffer stable and alive while readers borrow channel pointers |
| Input stream | Initially `stream`; then the JUCE reader | `checkedStream` observes it only while the reader lives |
| Output stream | Initially `fileStream`/`stream`; then the JUCE writer | `output` becomes invalid after `writer.reset()` |
| Temporary output path | `juce::TemporaryFile` | Attempts cleanup when normal control flow or exceptions leave scope |
| Processor | JUCE/plugin wrapper | Carries the audio/state lifetime, independent of whether the editor is open |
| Editor | JUCE/host editor lifecycle | Borrows the processor; must not destroy it |
| Callback audio buffer | Host/framework | Borrow channel pointers only during `processBlock` |
| Test fixtures | `TestDirectory` | Created and removed by the test process |

Const access does not make a buffer thread-safe by itself. Reading and writing
the same samples concurrently without a handoff would still be a data race.
The current milestone does not implement background loading or sample swapping.

## What the plugin currently does

The plugin files are still the JUCE starter scaffold. `prepareToPlay` and
`releaseResources` do no engine work. With its synth configuration,
`processBlock` clears all output channels to silence. Its inherited input-loop
placeholder performs no synthesis, and MIDI events are unused.

The capability methods report generated configuration flags. They do not add
note handling. Program methods return placeholder values; state callbacks do
not save or restore parameters/sample references. The editor paints a background
and centred text, and `resized` has no child components to arrange.

Those gaps are stated in the adjacent source comments. The new WAV loader is
not called by the plugin, and the offline CMake build does not build the plugin.
Connecting a prepared, stable buffer to grain processing is later roadmap work.

## Build and test comments

The root CMake file explains the header-only `drizel_core` interface target,
compiled `drizel_wav` archive, CLI executable, and test executable. It documents
which requirements propagate to consumers and why strict warning flags apply
to Drizel's sources rather than every third-party module source.

CTest has two registered entries: `audio_data` runs 21 internal named cases,
and `roundtrip_usage` checks that invoking the CLI without file arguments fails.
The test source documents every helper and case group, including the difference
between expected rejection and checking a precise exception category.

Exact comparisons are numeric sample comparisons. They do not require identical
WAV bytes, metadata, or the sign bit of zero. The excessive-file-length case
mutates a small fixture header; it does not allocate a multi-gigabyte test file.
The suite does not claim real-time safety, host compatibility, or listening QA.

The GitHub workflow explains its events, read-only permission, two OS jobs,
pinned JUCE source revision, and the configure/build/test sequence. The separate
Projucer file explains its file tree, modules, options, and macOS exporter.
Its groups now list the repository's source, tests, experiments, and documentation
for browsing. A visible file is not necessarily a compiled plugin source:
`compile="0"` keeps the offline adapter and both command-line entry points out
of the plugin target. They continue to build through CMake. Add future files
explicitly through Projucer's File explorer and save to regenerate IDE projects.
Generated `Builds/` and `JuceLibraryCode/` remain excluded from version control.

## Further detail

- [Audio-data design](audio-data.md): formats, memory budget, rate policy, and validation.
- [First experiment](../experiments/wav-roundtrip/README.md): running the CLI and listening checks.
- [Roadmap](roadmap.md): the future order of DSP and instrument work.
- [10 October 2026 development log](devlog/2026-10-10.md): completed work and target-machine validation.
- [Promptable modulation proposal](promptable-modulation.md): the experimental project direction; no model integration is implemented yet.
