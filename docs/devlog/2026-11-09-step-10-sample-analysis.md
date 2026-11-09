# Step 10 — Offline sample analysis

Step 10 delivers the first part of [Layer 9](../roadmap.md#layer-9-audio-characteristics-and-sample-analysis).
Drizel's grain position is a coordinate in a WAV. The offline sample map
describes what is around that coordinate by measuring level, spectral change,
brightness and flatness. The Python reference gives future sample-aware
selection a known input and feature contract to compare against.

## Reference design

[analyze.py](../../experiments/sample-analysis/analyze.py) decodes a WAV at its
original sample rate, measures each centered frame, and writes the settings and
measurements as JSON. Its plot shows the waveform, spectrogram and feature tracks
on the same time axis.

At 48 kHz, the 2,048-sample FFT observes about 42.67 ms per frame and spaces bins
23.4375 Hz apart. Advancing by 512 samples gives a new measurement every
10.67 ms. The tested 750 Hz tone sits at bin 32. The normalized one-sided spectrum
of the steady 0.5-peak sine sums to about 0.125 FS², consistent with an RMS of
about 0.354 FS.

Averaging channel power keeps opposite-polarity stereo energy visible. A
waveform average would erase that signal before analysis. The
[experiment guide](../../experiments/sample-analysis/README.md) specifies the
frame bounds, padding, normalization and unavailable values for a future C++
port. Analysis runs in a separate offline process from the instrument.

## Synthetic fixture results

[fixture.py](../../experiments/sample-analysis/fixture.py) generates a 2-second
mono PCM16 WAV at 48 kHz with one impulse, a steady sine, white noise from RNG
seed 0, and quiet gaps. It uses generated data under the repository license.
The measured WAV SHA-256 is
`cdd12eb0af8a28c1853a97514f4305abde61807d87e01f23f18e9a0c4b16a196`.
The published [map](../evidence/layer-9/sample-map.json) and
[plot](../evidence/layer-9/analysis.png) come from that file.

| Check | Expected | Observed |
|---|---|---|
| Quiet near 0.125 s | Zero RMS and unavailable spectral shape | 0 RMS, centroid and flatness `null` |
| Single impulse at 0.250 s | Local flux peak in a frame containing sample 12,000 and flatness > 0.99 | Peak at 0.24533 s, window [10,752, 12,800), flatness 0.99962 |
| 750 Hz sine near 0.750 s | Centroid within 0.1 Hz and RMS within 0.0001 FS of 0.5/√2 | 750.00001 Hz, 0.35355 FS RMS, flatness below 0.00001 |
| White noise near 1.500 s | RMS within 0.01 FS of 0.1 and flatness > 0.45 | 0.09861 FS RMS, flatness 0.56094 |

The impulse and the noise both have centroids near 12 kHz. Flatness is higher
for the impulse than for this finite noise frame. Noise also produces a steady
flux baseline, and the hard cutoff of the sine creates a flux peak. These are
measurements of spectra and their changes. Interpreting them as attacks, tails
or noise regions requires context from neighboring frames and the source.

## Reproduction and verification

- Base revision `0dd07cb` on Linux with Python 3.12.14, NumPy 2.3.5,
  SciPy 1.17.0 and Matplotlib 3.10.8. Follow the
  [setup and commands](../../experiments/sample-analysis/README.md#run-it).
- All 9 Python tests passed in a fresh environment. They cover the table above,
  source intervals at clip boundaries, opposite-polarity stereo, supported WAV
  encodings, source hashing and invalid audio.
- Separate runs with and without plotting produced byte-identical JSON for all
  188 frames. For a comparison across environments, match source hash,
  frame indices, settings and availability exactly, then compare numerical
  features with absolute tolerance `1e-9` and relative tolerance `1e-8`.
- All 10 existing offline CTest entries passed in a Linux Debug build with
  GCC 14.2 and JUCE 8.0.15 (`91ad83ae`).

The annotated guitar and other recording comparisons, pitch study and Layer 10
C++ parity and grain selection remain open.
