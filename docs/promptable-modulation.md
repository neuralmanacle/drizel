# Promptable modulation policies

**Status:** Research proposal, added 10 October 2026. No prompt interpreter,
model integration, or policy executor is implemented yet. Integration belongs
to [Layer 9 of the roadmap](roadmap.md#layer-9-promptable-modulation-policies-experimental).

## The project idea

Drizel could let a musician describe how a sound should move. A model would
interpret the description and propose a modulation **policy**: a rule that
defines how a parameter evolves over time. The musician could inspect and edit
the interpretation, then apply it to the instrument.

For example, the grain scan could accept "very chaotic", "go around in circles",
or "one step forward two step back". The initial research question is whether
these descriptions can become useful, understandable, repeatable controls.

The model's job would be interpretation. A local, bounded modulation source
would execute an accepted policy, so its movement could continue independently
of inference latency or network availability. Conventional controls would remain
available, and the instrument would work with model interpretation disabled.

No model architecture, provider, inference location, or hardware requirement has
been selected. Local and hosted candidates must be evaluated before committing
to an implementation.

## First target: grain scan position

Grain scan position chooses where a newly triggered grain starts in the source
sample. Moving that position changes the regions explored by successive grains.
The read rate inside each grain controls its playback pitch separately. A prompt
about scan movement should initially leave pitch, density, gain, and pan alone
unless the musician explicitly selects those destinations.

The first vocabulary would cover three patterns. The mappings below are design
proposals; phrases do not have a single universal musical interpretation.

| Prompt | Proposed interpretation | Editable settings |
|---|---|---|
| "very chaotic" | Seeded irregular movement through a bounded region, with variation in step size and timing | Region, movement depth, update rate, timing variation, smoothing, seed |
| "go around in circles" | A repeating scan through a region, wrapping from its end back to its start | Region, period, direction, starting phase |
| "one step forward two step back" | Repeat the signed step sequence `+1, -1, -1`, multiplied by a chosen step size | Region, step size, step interval, starting position, boundary mode |

For the first prompt, "chaotic" means an irregular musical gesture in this
proposal; it does not yet specify a mathematical chaotic system. The seed makes
the accepted random pattern repeatable in a given engine implementation.

A sample timeline is one-dimensional. "Circles" therefore initially means a
cyclic scan, not a literal two-dimensional orbit. A later policy might coordinate
scan position and another parameter, but that is outside the first experiment.

For the step pattern, the proposed default is three equally timed steps, each
with the same magnitude: one forward, then two backward. A different reading
would be one forward step followed by a double-sized backward step. The preview
must reveal the chosen interpretation so the musician can correct it.

## Policy representation

The first schema should be small and explicit. Proposed fields include:

| Field | Meaning |
|---|---|
| Schema version | Identify how to validate and restore this policy |
| Target | Initially only grain scan position |
| Pattern | A supported operation such as cyclic, stepped, or seeded irregular movement |
| Region | Normalized start/end coordinates within the current source sample |
| Timing | Explicit seconds or beats, with a chosen tempo source for beat-based timing |
| Pattern settings | Step size, direction, variation, phase, or other values appropriate to the selected operation |
| Boundary mode | Wrap or clamp within the selected region |
| Transition settings | How and when an accepted update replaces the current policy |
| Seed | Reproduce stochastic choices without another model call |

Spatial step size would be expressed as a fraction of the selected region.
Model output would require finite values, a non-empty region, positive timing
intervals, supported enum values, and implementation-defined capacity limits.
Exact limits and the mapping from normalized positions to valid frame indices
must be specified and tested before the executor is implemented.

A cyclic scan requires wrap behaviour. A step or irregular policy would expose
its boundary mode. Wrap should be applied as a region operation; smoothing
through a wrap must not accidentally sweep across the region in the opposite
direction. That interaction needs an explicit rule and an audible experiment.

The interpreter would return structured data drawn from this vocabulary. It
would not return executable C++ or scripts for the audio thread to evaluate.

## Proposed execution and ownership

1. The musician enters a prompt and selects its destination and sample region.
2. A worker interprets the prompt and validates the candidate's schema, units,
   ranges, and resource limits. Hosted network requests, if any, occur here.
3. The UI shows the proposed movement and exposes its editable controls.
4. Applying the policy prepares a bounded numeric representation outside the
   audio callback and hands it to the engine at a defined activation point.
5. A local modulation source advances the policy and supplies smoothed controls
   to the existing engine. Accepted repeating patterns need no further model call.

The real-time handoff would use a bounded queue with defined ownership. It must
not cause allocation, deallocation, parsing, logging, locks, or network work in
the callback. Layer 5 establishes the audio thread's ownership rules; Layer 6
provides the parameter and modulation interfaces this feature will use.

Finite trajectories are an additional representation, not a requirement for
every repeating pattern. They would use timestamped breakpoints prepared with
lookahead and an explicit continuation when the future buffer runs out.

Invalid or late candidates should leave the current local policy running.
Results for an older prompt or a replaced sample must be discarded before
activation. A new accepted policy needs a defined transition to avoid unexpected
parameter jumps. The UI should show when interpretation is pending or failed
while keeping manual control available.

Preset state should save the interpreted policy, schema version, edits, seed,
and original prompt. The saved numeric policy should restore without repeating
inference. Reset and transport behaviour must also be defined so that offline
renders can be compared repeatably.

## Experiments and acceptance

The first study would compare manually configured patterns with model-derived
versions of the three seed prompts and their paraphrases. It should record the
chosen model/version, prompt, interpreted policy, validation outcome, time to a
usable result, and the musician's judgement of the preview and render.

Planned checks include:

- Cyclic scans remain inside their region and repeat at the configured period.
- The step pattern follows its documented sequence at exact step times,
  including boundary crossings.
- Irregular movement stays inside its region and configured variation limits;
  the same seed repeats in the tested implementation.
- Invalid fields, non-finite values, excessive sizes, and unsupported operations
  are rejected before the engine receives a policy.
- Replacing the prompt or sample prevents an older result from taking effect.
- Disabling inference or losing a hosted connection does not interrupt an
  accepted repeating policy or audio playback.
- Applying a policy satisfies the real-time engine's callback budget, and the
  same saved policy, seed, source, and render settings reproduce a result in the
  tested engine build without another model call.

Model-dependent behaviour must be measured before making latency or quality
claims. Longer-term possibilities include coordinating position, density, pitch,
and spread, or providing sample analysis as context. These remain experiments.

## Relationship to the current milestone

The implemented WAV loader supplies the decoded source audio this feature would
eventually explore. The next implementation work remains windows and
interpolation, then one grain, scheduling, a real-time engine, and general
modulation. The [10 October development log](devlog/2026-10-10.md) records the
current boundary between working code and this proposed direction.
