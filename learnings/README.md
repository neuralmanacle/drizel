# Learning path

Drizel is the practical project for learning audio software engineering, DSP,
audio ML and embedded deployment. Study progress is separate from code progress:
a working feature does not automatically establish personal understanding.

## Start and follow

1. Review [prerequisites](prerequisites.md) and explain the current engine from
   the [code guide](../docs/code-guide.md).
2. Follow the [post-Step-9 delivery plan](../docs/post-step-9-plan.md), starting
   with Step 10 host acceptance and timing measurements.
3. Use the matching block in the [DSP/ML/embedded curriculum](audio-dsp-ml-curriculum.md).
4. Publish a reproducible experiment and result with the
   [proof-of-work template](../docs/proof-of-work.md).

## Learning order

| Block | Concepts | Delivery steps |
|---|---|---|
| Foundations | C++, ownership, sampled audio, maths, MIDI and plugin lifecycle | Revisit existing Steps 1–9 as needed |
| Reliable instrument | Timing budgets, concurrency, modulation and host behaviour | 10–11 |
| Sound quality | Fourier analysis, windows, filtering and resampling | 12 |
| Audio understanding | Python, signal characteristics and sample maps | 13–14 |
| Applied ML | Dataset design, baselines, neural training and evaluation | 15 |
| Local deployment | C++ parity, inference, state and fallback | 16 |
| Language control | Validated policies, preview and deterministic execution | 17 |
| Neural audio | Causal effect modelling and real-time feasibility | 18, optional |
| Hardware | Embedded audio bring-up, memory, DMA and profiling | 19 |
| Edge ML | Quantisation, runtime compatibility and on-device measurements | 20, optional |

## Notes and experiments

Write notes in original language: the concept, an equation/example, the relevant
Drizel code, a failure case and references. Create a small experiment before
production integration; keep a useful negative result as an experiment rather
than forcing it into the instrument. See [experiment organisation](../experiments/README.md).
Update study checkboxes only after explaining and modifying the result yourself.
