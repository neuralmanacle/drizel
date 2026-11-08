# Experiments

Experiments answer a small question before a production change. See the
[post-Step-9 plan](../docs/post-step-9-plan.md), [curriculum](../learnings/audio-dsp-ml-curriculum.md)
and [evidence template](../docs/proof-of-work.md) for scope and acceptance.

## Existing entry points

| Experiment | Source | Purpose |
|---|---|---|
| WAV roundtrip | [Guide](wav-roundtrip/README.md) and [source](wav-roundtrip/main.cpp) | Decode, write, reload and compare audio |
| Single grain | [Source](single-grain/main.cpp) | Offline windowed grain rendering |
| Grain cloud | [Source](grain-cloud/main.cpp) | Offline scheduling and overlap-add |

The C++ programs build through root CMake. Follow the [root build instructions](../README.md).

## Proposed experiments — not created or implemented yet

| Proposed directory | Step | Question |
|---|---|---|
| `realtime-benchmark/` | 10 | What workload fits the measured callback budget? |
| `modulation/` | 11 | Are sources/routes bounded, repeatable and musically useful? |
| `resampling-quality/` | 12 | What sound-quality/cost trade-off does each reader provide? |
| `sample-analysis/` | 13 | Can audio characteristics be mapped reliably to source time? |
| `sample-selection/` | 14 | Do feature-guided grain choices improve control? |
| `audio-region-classifier/` | 15 | Does a small model improve on the DSP/simple learned baseline? |
| `cpp-inference/` | 16 | Do deployed preprocessing/predictions agree with Python? |
| `llm-control/` | 17 | Are editable prompt-derived policies useful and reliable? |
| `neural-effect/` | 18, optional | Does a small neural effect justify its runtime cost? |
| `embedded-audio/` | 19 | Can the reduced engine run on actual hardware? |
| `embedded-inference/` | 20, optional | Can a useful model coexist with the device's audio budget? |

Create only the directory needed for the active experiment. Its README should
state the hypothesis, input provenance, setup, exact commands, output location,
comparison method and limitations. Keep training dependencies optional and
separate from the normal plugin build. Do not add placeholder code or empty
folders to imply progress; publish evidence and update status when work exists.
