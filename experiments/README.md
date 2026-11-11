# Experiments

Experiments answer a small question before a production change. See the
[roadmap](../docs/roadmap.md), [curriculum](../learnings/audio-dsp-ml-curriculum.md)
and [evidence template](../docs/proof-of-work.md) for scope and acceptance.

## Existing entry points

| Experiment | Source | Purpose |
|---|---|---|
| WAV roundtrip | [Guide](wav-roundtrip/README.md) and [source](wav-roundtrip/main.cpp) | Decode, write, reload and compare audio |
| Single grain | [Source](single-grain/main.cpp) | Offline windowed grain rendering |
| Grain cloud | [Source](grain-cloud/main.cpp) | Offline scheduling and overlap-add |
| Sample analysis | [Guide](sample-analysis/README.md) and [source](sample-analysis/analyze.py) | Time-aligned WAV features and a reproducible sample map |

The C++ programs build through root CMake. Follow the [root build instructions](../README.md).

## Proposed experiments — not created or implemented yet

| Proposed directory | Layer | Question |
|---|---|---|
| `resampling-quality/` | 2 | What sound-quality/cost trade-off does each reader provide? |
| `realtime-benchmark/` | 5, 7–8 | What workload fits the callback budget and host acceptance? |
| `modulation/` | 6 | Are sources/routes bounded, repeatable and musically useful? |
| `sample-selection/` | 10 | Do feature-guided grain choices improve control? |
| `audio-region-classifier/` | 11 | Does a model improve on DSP/simple learned baselines? |
| `cpp-inference/` | 12 | Do deployed preprocessing/predictions agree with Python? |
| `llm-control/` | 13 | Are editable prompt-derived policies useful and reliable? |
| `neural-effect/` | 14, optional | Does a small neural effect justify its runtime cost? |
| `embedded-audio/` | 15 | Can the reduced engine run on actual hardware? |
| `embedded-inference/` | 16, optional | Can a useful model coexist with the audio budget? |

Create only the directory needed for the active experiment. Its README should
state the hypothesis, input provenance, setup, exact commands, output location,
comparison method and limitations. Keep training dependencies optional and
separate from the normal plugin build. Do not add placeholder code or empty
folders to imply progress; publish evidence and update status when work exists.
