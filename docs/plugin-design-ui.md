# Plugin design/UI

**Wild card — ongoing and unnumbered.** This is Drizel's design notebook: a place
to explore, document and revisit the interface alongside any capability in the
[roadmap](roadmap.md). It has no fixed position, delivery-step number or "finish
once" milestone. A small layout experiment and a larger visual redesign both
belong here.

## What to document

| Area | Questions to explore | Useful evidence |
|---|---|---|
| Visual identity | What should Drizel feel like? How do colour, type, texture and motion support that? | References with attribution, moodboards, palette/type studies and alternatives |
| Layout and hierarchy | What must a musician see first? Which controls belong together? | Sketches, wireframes, panel comparisons and annotated screenshots |
| Controls and interaction | Knob or slider? Which units, ranges, defaults and fine adjustments are understandable? | Interaction notes and a short recording of the control in use |
| Waveform and grain feedback | What is actually being displayed: base position, modulated position, grain launches or analysis regions? | Labelled mockups and a comparison with actual engine behaviour |
| Modulation and performance | How can routes, amounts, MIDI overrides and active states be understood quickly? | Route sketches and a short musical task walkthrough |
| Sample and preset workflow | Can someone load, audition, save and recover a missing sample without confusion? | Task sequence, state/error examples and observed difficulties |
| Analysis and ML feedback | How should regions, confidence, pending work and manual fallback appear? | Explorations tied to the relevant planned feature, labelled as mockups |
| Accessibility and scaling | Are labels readable? Can controls be reached by keyboard and understood without colour alone? | Focus-order notes, screen-reader checks and actual scale/host screenshots |
| Rendering and responsiveness | Does opening, resizing or animating the editor affect playback or interaction? | UI measurements and playback observations with conditions stated |

These are prompts for exploration, not a required checklist for every entry.
Choose a design question that matters to the current work.

## Existing interface baseline

The [interface guide](step-8-interface.md) documents the first instrument editor;
the [Step 9 guide](step-9-instrument.md) adds MIDI/envelope/root and preset
controls. Those guides and their verification logs remain the implementation
record. Source lives in [PluginEditor.h](../src/PluginEditor.h) and
[PluginEditor.cpp](../src/PluginEditor.cpp), with processor parameters/state in
[PluginProcessor.cpp](../src/PluginProcessor.cpp).

In that documented baseline, the waveform marker represents base position;
it does not visualise every grain or the modulated target. The output display
is a UI-polled block peak, not a true-peak or clipping-history display. New
visual concepts should state whether they illustrate existing data or require
new engine telemetry.

No new mockup, visual identity, redesign or usability result is claimed by
creating this notebook. Layer 8 still tracks host, lifecycle, state and packaging
acceptance. Design exploration can happen at any time; a live UI change needs
the checks appropriate to what it changes.

## Design journal

Add dated entries below as work happens, or link a longer dated devlog from here.
Keep explorations, chosen designs, implementations and observed results clearly
labelled. Preserve a rejected direction when its reasoning is useful.

No new design entries have been recorded here yet. The existing interface guides
above are the starting references.

### Entry template

Copy this structure for a design session; fill only the relevant fields.

- **Date and question:** What problem or creative direction are you exploring?
- **Context:** Which musician task, screen or capability does it concern?
- **References:** What inspired it, and which specific idea are you studying?
- **Exploration:** Link sketches, mockups, screenshots or prototypes; label their status.
- **Alternatives:** What changed between versions, and why?
- **Decision:** What are you trying, choosing or deferring? What trade-off does it make?
- **Implementation:** If built, link the commit/PR and identify affected controls or state.
- **Observation:** What happened when you or another musician used it? Record the task and conditions.
- **Evidence:** Before/after views or video, plus relevant accessibility, host or performance checks.
- **Revisit:** What remains uncertain or might change with later capabilities?

## Keep the notebook useful

Prefer a small, annotated before/after comparison over a large collection of
unexplained screenshots. Keep image assets reasonably sized and link them by
relative path only after they exist. Do not imply that a mockup is implemented
or that a personal impression is a measured usability result.

For actual changes, preserve host parameter identity/automation and saved state
unless an intentional migration is documented. Keep painting, strings and
component work off the audio callback; respect the editor/processor lifetime
boundary. Use the [evidence guide](proof-of-work.md) when a design experiment
becomes implementation proof of work.
