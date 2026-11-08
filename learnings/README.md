# Learning path

Drizel is the practical project for learning audio software engineering, DSP,
audio ML and embedded deployment. Study progress is separate from code progress:
a working feature does not automatically establish personal understanding.

## Start and follow

1. Review [prerequisites](prerequisites.md) and explain the current engine from
   the [code guide](../docs/code-guide.md).
2. Follow the [roadmap](../docs/roadmap.md), closing existing host, timing,
   modulation and sound-quality gaps before live analysis/ML integration.
3. Use the matching block in the [DSP/ML/embedded curriculum](audio-dsp-ml-curriculum.md).
4. Publish a reproducible experiment and result with the
   [proof-of-work template](../docs/proof-of-work.md).

## Learning order

| Layers | Concepts |
|---|---|
| 0–2 | C++, ownership, sampled audio, maths, windows, filtering and resampling |
| 3–4 | Grain rendering, scheduling, mixing and bounded voice state |
| 5–8 | Timing, concurrency, modulation, MIDI, UI, presets and host acceptance |
| 9–10 | Python, signal characteristics, C++ analysis and sample-aware selection |
| 11–12 | Dataset design, baselines, neural training, evaluation and C++ inference |
| 13 | Validated language policies, preview and deterministic execution |
| 14, optional | Causal neural effects and real-time feasibility |
| 15 | Embedded audio, memory, codec/DMA and hardware profiling |
| 16, optional | Quantisation and measured on-device inference |

## Plugin design/UI

Use the [design notebook](../docs/plugin-design-ui.md) as an unnumbered wild card
alongside any topic. Document visual references, layout/interaction experiments,
what you learned and why a design changed. It has no fixed place in the layer sequence.

## Notes and experiments

Write notes in original language: the concept, an equation/example, the relevant
Drizel code, a failure case and references. Create a small experiment before
production integration; keep a useful negative result as an experiment rather
than forcing it into the instrument. See [experiment organisation](../experiments/README.md).
Update study checkboxes only after explaining and modifying the result yourself.
