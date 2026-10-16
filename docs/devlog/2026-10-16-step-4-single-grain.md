# Step 4 — Single-grain rendering

Combined the existing window and interpolation primitives into a fixed-size,
reusable grain player. It borrows prepared source/window storage, tracks output
age independently of source position, supports linear/Hermite reads, and applies
per-grain gain and mono pan or stereo balance. Rendering allocates nothing.

Added an offline WAV experiment with source/output rate correction, tests, and
an annotated code guide in `docs/single-grain.md`. Updated the roadmap without
marking the target-machine listening milestone complete.

Validation: GCC strict-warning build with JUCE 8.0.15; all five CTest targets
pass. Grain tests also pass ASan/UBSan with leak detection disabled for runtime
compatibility. Three 100 ms WAV renders numerically verify duration, finite
samples, silent endpoints, and octave pitch shifts. Mac build/listening remains.
