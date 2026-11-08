# Proof of work and milestone evidence

**Status: process and template, 8 November 2026.** This document defines evidence for each layer; it does not report completed experiments or qualifications.

## What a milestone demonstrates

| Area | Evidence a reviewer can inspect |
|---|---|
| Audio software | A reproducible build, working instrument, state/host checks and clear ownership design |
| DSP | Equations/units, known-signal checks, spectra, listening examples and quality/cost trade-offs |
| Audio ML | Dataset provenance, recording-level splits, baseline, held-out metrics, errors and model card |
| Deployment | Python/C++ parity, target runtime/memory, failure behaviour and versioned preprocessing |
| Embedded audio | Actual hardware run, firmware and wiring, audio comparison and RAM/flash/timing measurements |

A tutorial followed, an unchecked roadmap, a video alone or code generated
without an explanation is not enough to substantiate the associated capability.
A measured negative result is useful research evidence when labelled honestly.

## Evidence layout

Use existing `docs/devlog/` for dated reports. Proposed locations below do not
exist until their experiment starts; do not publish broken links to them.

- `experiments/<topic>/`: runnable code, environment/setup and small fixtures.
- `learnings/<topic>.md`: own explanation, equations, worked examples and references.
- `docs/devlog/YYYY-MM-DD-layer-N.md`: result report using the template below.
- `docs/evidence/layer-N/`: small measurement tables, plots and artifact manifests.
- Larger recordings, datasets and weights: an explicitly identified release or
  artifact location, with checksums, access instructions and redistribution terms.
  Avoid committing large binary files by default.

The minimum record is enough for another person to reproduce the stated result.
Store only recordings/data/models that may be used and redistributed under the
chosen terms; for nonredistributable inputs, document a reproducible permitted
substitute and the limits of the published evidence.

## Reusable report template

Copy this structure into a new dated devlog when working on a layer. Fill in actual
results; use "not run" instead of plausible numbers or implied completion.

### Question and scope

- Layer, actual delivery step/commit, problem and musician-facing behaviour:
- Hypothesis and baseline:
- Acceptance thresholds chosen before the final comparison:
- Explicit exclusions and known limitations:

### Reproduce

- Source commit and clean/dirty state:
- Machine/OS/compiler, dependency and tool versions:
- Host/format or board/codec/firmware configuration, as applicable:
- Input IDs/hashes, provenance/licences and random seeds:
- Exact setup, build, run and evaluation commands:
- Where outputs are written and how they are retrieved:

### Learning and design

Explain the relevant concept in original language, include an equation with
units and a worked example where useful, and identify the responsible code.
Describe the ownership/execution boundary and one meaningful alternative.

### Results

| Check | Baseline | Candidate | Conditions | Verdict |
|---|---|---|---|---|
| Fill with an actual metric or observation | Not run | Not run | Specify settings | Pending |

- Relevant tests and actual pass/fail results:
- Listening protocol, gain treatment and example locations:
- Runtime/memory and measurement overhead:
- Failures, surprising results and attempted explanations:
- Decision: accept, revise or defer, with the reason:

### Additional ML record (when applicable)

- Label definitions, ambiguity policy, data manifest and grouped split IDs:
- Training-only preprocessing/calibration; validation selection procedure:
- Baseline, architecture, loss, hyperparameters, seeds and training command:
- Final held-out metrics, confusion matrix, per-source errors and uncertainty:
- Feature/schema/runtime versions, weights hash and intended/unsupported use:

### Additional real-time or hardware record (when applicable)

- Sample rate, block size, voices/routes and concurrent workload:
- Callback interval, declared headroom target, timing distribution and observed max:
- Run length, dropouts/underruns, instrumentation method and overhead:
- Board revision, wiring, memory map, stack/heap accounting and firmware hash:
- Reference comparison tolerance, reset/overload/fallback behaviour:

### Design/UI record (when applicable)

The [Plugin design/UI notebook](plugin-design-ui.md) is an ongoing wild card,
not an additional milestone. For a design change, link the question, alternatives,
annotated before/after views, rationale and observations. Distinguish a mockup
from an implemented interface; include host, accessibility or playback checks
when the implementation affects them. Exploratory notes can stay lightweight.

### Share and explain

- A concise demo showing the exact change, with source/sample/settings identified:
- An educational blog/devlog connecting theory, code, result and limitations:
- Links to commit/PR, report, plots/data and reproducible inputs:
- A short statement of what is now demonstrated and what remains unverified:

## Review and checkbox rules

Mark study complete only after the learner can explain and modify the result.
Mark implementation complete only after it exists and passes relevant checks.
Mark target validation complete only with a run on that target. Keep CI build,
listening acceptance, public packaging and hardware deployment separate.
Update [the roadmap](roadmap.md), the README
and [prerequisites](../learnings/prerequisites.md) only to the extent supported by
evidence. Historical devlogs retain their original scope and dates.
