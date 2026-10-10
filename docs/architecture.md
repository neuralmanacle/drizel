# Engine Architecture

## First implemented layer

The offline experiment loads a WAV through `src/io/WavFile.cpp` into
`src/audio/SampleBuffer.h`, then writes a float32 WAV and compares the decoded
samples. The buffer uses standard C++ types and retains its source sample rate.
See [audio data and file I/O](audio-data.md) for layout, ownership, error handling,
and the sample-rate policy. The grain-processing flow below is still planned.

## Data flow

```text
SourceBuffer
    ↓
GrainScheduler
    ↓
GrainPool
    ↓
GrainReader
    ↓
Envelope
    ↓
Mixer
    ↓
OutputProcessor
```

## Grain state

Each grain contains:

- Source position
- Current read position
- Playback rate
- Duration
- Current age
- Amplitude
- Pan
- Active state

## Processing model

The engine processes audio in blocks. It does not allocate memory,
load files, or perform blocking operations inside the audio callback.

## Promptable modulation policies (proposed)

The experimental model integration would turn a musician's description of
movement into a policy that ordinary modulation code can execute. The first
target is **grain scan position**: the position at which newly triggered grains
begin reading the sample. Moving that position is separate from changing the
read rate, and therefore pitch, within each grain.

| Stage | Responsibility | Execution context |
|---|---|---|
| Prompt and preview | Accept text, show the interpreted pattern, and expose editable settings | UI/message thread |
| Policy interpreter | Produce a structured candidate from a supported vocabulary | Inference worker; local or hosted model remains an open choice |
| Validator and preparation | Check types, ranges, timing, and limits; prepare a bounded numeric representation | Worker outside the audio callback |
| Policy handoff | Deliver prepared controls with defined ownership and activation timing | Bounded queue between worker and engine |
| Local modulation source | Advance accepted patterns and feed the engine's smoothed parameters | Real-time engine, with no allocation or blocking |

Repeated scans, step sequences, and seeded irregular movement would continue
locally after a policy is accepted. Finite breakpoint trajectories would use
lookahead and a defined continuation when updates arrive late. The model is
never required to generate a fresh decision for each audio block or grain.

Only validated numeric controls would cross into the real-time engine. Prompt
text, parsing, model inference, and network I/O stay outside it. Model output
would select supported operations and values; it would not be executed as C++
or a script. A malformed response would leave the current policy active.

Policy changes would have an explicit activation point and smoothing policy.
Replacing a sample would invalidate any pending policy that targets the old
sample. The queue, capacity limits, and ownership handoff still need to be
designed and tested during Layers 5, 6, and 9; none is implemented by the WAV
milestone.

Preset state would include the interpreted policy, its version, edited values,
and random seed as well as the original prompt. Restoring a supported policy
should not require another model call. Conventional modulation and manual
controls remain available when model interpretation is disabled.

See the [promptable modulation proposal](promptable-modulation.md) for example
prompts, boundary behaviour, and the planned experiments.
