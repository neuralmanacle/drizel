# Step 5 — overlapping grains

Added a fixed-capacity pool and seeded scheduler around the Step 4 grain player.
The offline renderer now creates stereo textures with position, pitch, pan,
length and timing variation. All active grains contribute through overlap-add.

Preparation owns three Hann tables, scans the source peak and computes a
conservative fixed gain. Playback allocates nothing. A saturated pool drops
new events; existing grains finish their envelopes. The render includes a tail.

Validation: all seven CTest targets passed with JUCE 8.0.15 and GCC 13.3 in
Release. Core tests also passed ASan/UBSan. Generated-source WAV checks covered
5, 20, 80 and 1000 grains/second; all produced finite, bounded stereo output
with silent endpoints. The 64-slot limit held under saturation.

See [the guide](../grain-cloud.md) for formulas, code explanations, commands,
measurements and limitations. Mac listening and worst-case timing remain open.
