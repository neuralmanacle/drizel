# Step 10 — Offline sample analysis

Step 10 delivers the offline reference for [Layer 9](../roadmap.md#layer-9-audio-characteristics-and-sample-analysis).
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

## Annotated recording study

The 10 November follow-up adds five short recorded excerpts alongside the generated
fixture. The [manifest](../../experiments/sample-analysis/corpus/manifest.json)
identifies each source and its rights, describes any conversion and trim, and
binds the half-open annotations to the WAV's SHA-256. Note labels come from the
upstream SFZ key centers, MIDI 40 for the guitar and MIDI 60 for the tenor sax.
The chord and percussion labels come from their published source descriptions.
The real attack markers use the first sample reaching 10% of the excerpt's peak
amplitude across channels. These are approximate reference points on the raw
waveforms. The synthetic impulse has an exact marker from the generator.

### Pitch and confidence

[study.py](../../experiments/sample-analysis/study.py) analyzes 80 ms windows
on the same 512-sample hop grid as the sample map. It removes each channel's
DC offset and skips pitch estimation below 0.001 full-scale AC RMS (−60 dBFS).
It compares the waveform with delayed copies over candidate lags for 60–1,000 Hz.
At each lag it sums the dot products across channels and divides by the geometric
mean of the two overlaps' energies. This normalized autocorrelation measures
how closely the waveform repeats, including in opposite-polarity stereo.

The shortest local peak within 95% of the highest peak wins. This favors one
period over longer multiples when their scores are similar. A parabola through
the peak and its neighbors refines the lag before computing `sample_rate / lag`.
The peak height is the periodicity score used for the sweep. Only complete
windows inside each labeled region count. A reference match is within 50 cents
of the nominal frequency with A4 = 440 Hz. Median errors below use accepted
frames at the 0.80 cutoff.

| Region | Reference (Hz) | Accepted at 0.60 | Accepted at 0.80 | Accepted at 0.95 | Median F0 (Hz) | Median absolute error (cents) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Guitar E2 decay | 82.4069 | 59/59 | 59/59 | 59/59 | 82.4438 | 0.77 |
| Tenor sax C4 sustain | 261.6256 | 51/51 | 51/51 | 51/51 | 261.5956 | 0.30 |
| 750 Hz sine | 750 | 21/21 | 21/21 | 21/21 | 749.9969 | 0.01 |
| Guitar chord | — | 53/64 | 0/64 | 0/64 | — | — |
| Claves decay | — | 0/4 | 0/4 | 0/4 | — | — |
| Quiet claves tail | — | 0/24 | 0/24 | 0/24 | — | — |
| Rain field recording | — | 0/59 | 0/59 | 0/59 | — | — |
| Seeded white noise | — | 0/30 | 0/30 | 0/30 | — | — |
| Digital silence | — | 0/2 | 0/2 | 0/2 | — | — |

All accepted reference windows were within 50 cents. At 0.60 the chord produced
53 false accepts for a single-pitch interpretation, with a median candidate of
123.75 Hz. Several strings can produce a combined waveform with a repeat lag.
At 0.80 this chord was rejected and both single notes retained full coverage.
The score describes waveform repeatability. The sweep covers two isolated
recorded notes and one chord, and adjacent windows share samples.

### Features versus the annotations

With 2,048-sample frames and 512-sample hops, the study summarizes windows wholly
inside each region. For flux, both the current and previous window must fit.
Spectral medians use the frames where the feature is available.

| Region | RMS (FS) | Onset strength (FS) | Centroid (Hz) | Flatness | Silent frames |
| --- | ---: | ---: | ---: | ---: | ---: |
| Guitar E2 decay | 0.24352 | 0.00251 | 257.55 | 5.83e-8 | 0/61 |
| Guitar chord | 0.12429 | 0.02088 | 310.21 | 4.27e-6 | 0/67 |
| Tenor sax C4 sustain | 0.41491 | 0.00673 | 428.94 | 5.15e-5 | 0/54 |
| Claves decay | 0.00407 | 0.00048 | 2,183.80 | 4.19e-4 | 0/7 |
| Quiet claves tail | 0.000072 | 0.00000 | 2,081.82 | 0.01018 | 19/27 |
| Rain field recording | 0.01788 | 0.00594 | 1,220.15 | 1.10e-5 | 0/61 |
| Seeded white noise | 0.09923 | 0.03527 | 12,013.20 | 0.56271 | 0/34 |
| Digital silence | 0 | 0 | — | — | 5/5 |

- **Attacks.** Search for the strongest flux frame within 2,048 source samples
  of each marker. The guitar note peak was centered 1.97 ms early and the claves
  peak 3.02 ms late. Both windows contain their marker. The chord peak was
  26.83 ms late and its window excludes the first crossing. Flux responds to the
  strongest spectral change in the strum, which can occur after its first sound.
- **Chords.** The E2 note has an 82.44 Hz estimated fundamental and a 257.55 Hz
  centroid because the centroid includes its harmonics. The chord also has low
  flatness. A harmonic spectrum can come from either one note or several.
- **Noise.** Both white noise and rain have zero pitch accepts. Rain has far
  lower flatness because its energy is concentrated in a smaller part of the
  band. Even white noise produces flux as energy moves between FFT bins.
- **Quiet passages.** The claves tail falls through the −80 dBFS spectral
  silence threshold, which makes centroid and flatness unavailable in 19 of 27
  windows. Its spectral medians describe the eight measurable windows. The
  separate pitch gate leaves all 24 tail windows without a candidate.

## Reproduction and verification

- Base revision `0dd07cb` on Linux with Python 3.12.14, NumPy 2.3.5,
  SciPy 1.17.0 and Matplotlib 3.10.8. Follow the
  [setup and commands](../../experiments/sample-analysis/README.md#run-it).
- All 13 Python tests passed in a fresh environment. They cover the synthetic
  checks, WAV decoding, source identity and the annotated study, including
  pitch estimates, the level gate and chord, noise and quiet controls.
- Separate runs with and without plotting produced byte-identical JSON for all
  188 frames. For a comparison across environments, match source hash,
  frame indices, settings and availability exactly, then compare numerical
  features with absolute tolerance `1e-9` and relative tolerance `1e-8`.
- The annotated study report matched byte for byte across two runs on the same
  machine. With and without plotting, the recorded E2 map also matched byte for byte.
- All 10 existing offline CTest entries passed in a Linux Debug build with
  GCC 14.2 and JUCE 8.0.15 (`91ad83ae`).

**Follow-up:** [Step 11](2026-11-11-step-11-sample-aware-playback.md) ports RMS,
onset and centroid to C++ and uses them for guided grain selection in Layer 10.
