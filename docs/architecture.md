# Engine Architecture

## First implemented layer

The offline experiment loads a WAV through `src/io/WavFile.cpp` into
`src/audio/SampleBuffer.h`, then writes a float32 WAV and compares the decoded
samples. The buffer uses standard C++ types and retains its source sample rate.
See [audio data and file I/O](audio-data.md) for layout, ownership, error handling,
and the sample-rate policy. Steps 4–5 added the single grain and offline cloud;
Step 6 now connects them to JUCE playback. See [real-time engine](realtime-engine.md)
for the active/pending/retired ownership protocol, live controls and worker loader.
The callback borrows host buffers and never decodes or destroys sample storage.

## Data flow

| Current stage | Responsibility |
|---|---|
| Prepared source | Worker-decoded sample storage, candidate tables and prepared cloud in one packet |
| Grain selection | Map smoothed Position to a source region and choose a candidate at launch, with Manual fallback |
| Scheduler and pool | Launch grains into fixed-capacity reusable voices |
| Grain reader and window | Interpolate source samples and shape grain amplitude |
| Mixer and output | Sum voices, smooth controls, apply safety and MIDI amplitude gate |
| Control path | Current LFO route and MIDI performance controls |

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
designed and tested for the policy path in Layer 13. Step 9 already has a
separate sample-ownership handoff; that does not implement a policy queue.

Preset state would include the interpreted policy, its version, edited values,
and random seed as well as the original prompt. Restoring a supported policy
should not require another model call. Conventional modulation and manual
controls remain available when model interpretation is disabled.

See the [promptable modulation proposal](promptable-modulation.md) for example
prompts, boundary behaviour, and the planned experiments.

## Analysis, ML and hardware boundaries

The [roadmap](roadmap.md) defines the capability layers and their dependency
gates. The table distinguishes current execution paths from future work.

| Responsibility | Execution and ownership | Layer |
|---|---|---|
| General sources and routes | Bounded audio-thread state with explicit units and reset rules | 6 |
| Analysis reference | Implemented offline Python experiment with versioned JSON | 9 |
| Source characteristics | Implemented C++ analysis on the loader, tied to the decoded source in the packet | 10 |
| Grain candidate selection | Implemented bounded table lookup at launch, with Manual fallback | 10 |
| Training and evaluation | Offline Python; recording-level splits and versioned features | 11 |
| Region inference | Worker-side C++ model, versioned preprocessing and bounded result handoff | 12 |
| Language interpretation | Worker, schema validation and UI preview before activation | 13 |
| Numeric policy execution | Local engine source; continues without inference responses | 13 |
| Neural audio effect | Optional causal path; separate bounded-memory/timing or buffered-latency design | 14 |
| Embedded adapter | Hardware-specific audio/control I/O around the reduced independent core | 15 |
| Embedded model | Budgeted preprocessing and inference scheduled alongside device audio | 16 |

### Analysis and model handoff

Step 11 keeps the source and candidate tables in the same prepared packet. The
loader checks for a new request or shutdown between analysis windows. The audio
thread validates the request revision and output rate at activation and returns
retired packets to the loader. Preset recall regenerates the tables from the WAV
and restores the saved mode. See the [Step 11 devlog](devlog/2026-11-11-step-11-sample-aware-playback.md).

Future model results need their own identity and versioning for preprocessing
and weights. Keep model validation and reclamation outside the callback.

### Different inference workloads

Region analysis/classification and prompt interpretation stay outside the audio
callback. They can produce future controls or immutable labels. A neural audio
effect is different: synchronous inference consumes callback time, while an
asynchronous effect introduces buffering and latency. Layer 14 requires a
separate feasibility decision, reset/overload contract, allocation audit and
actual timing measurements; it is not authorised by the worker-analysis design.

### Hardware portability

Keep generic buffers, grain rendering, modulation and analysis contracts separate
from JUCE, codec drivers and ML runtimes. Select an embedded target from sample,
voice, buffer, model and activation budgets. Replace desktop services with explicit
hardware adapters and limited configurations; porting the whole desktop UI is
not the Layer 15 goal. Real device playback and measurement are acceptance gates.
