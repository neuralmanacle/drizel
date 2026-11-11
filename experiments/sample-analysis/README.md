# Offline sample analysis

Step 10 builds the Layer 9 reference. The experiment asks whether measured
level, spectral change, spectral shape and periodicity line up with known events in a WAV.
It runs as a Python command independently of the C++ build.

## Run it

Use Python 3.12 and run from the repository root. The direct dependency versions
are pinned in `requirements.txt`.

```sh
python3 -m venv build/sample-analysis-venv
source build/sample-analysis-venv/bin/activate
python -m pip install -r experiments/sample-analysis/requirements.txt

python experiments/sample-analysis/fixture.py build/sample-analysis-input/fixture.wav
python experiments/sample-analysis/analyze.py \
    build/sample-analysis-input/fixture.wav build/sample-analysis-demo --plot
python -m unittest discover -s experiments/sample-analysis -v
```

To analyze your own recording, pass a mono or stereo WAV and a new output directory.
The command writes `sample-map.json` and adds `analysis.png` when you pass `--plot`.
Choose a fresh output directory for each run. The fixture generator also requires
a new output file.

```sh
python experiments/sample-analysis/analyze.py \
    "/path/to/guitar.wav" build/sample-analysis-guitar --plot
```

The reference accepts integer PCM, including PCM16 and packed PCM24, and IEEE
float32 WAV. It keeps the source rate and converts integer PCM to full-scale
amplitude units. Float32 values retain their original amplitude, including
finite values above unity. The command keeps the recording and frame results
in memory. Plotting also retains the spectra, so omit `--plot` for a map-only run.

## Frame and channel conventions

The default window is 2,048 samples and the default hop is 512 samples, about
42.67 ms and 10.67 ms respectively at 48 kHz. `--frame-size` accepts powers of two
from 32 to 16,384. The hop defaults to a quarter of that size and `--hop-size`
can set it from 1 to half the window size.

For window length `N` and hop `H`, frame `i` is centered at source sample `i * H`
and has timestamp `i * H / sample_rate`. Centers begin at sample zero and stop
before the end of the source. Each frame reads the interval
`[center - N/2, center + N/2)`. The FFT uses a periodic Hann window and zeroes for
samples outside the WAV. The exported `start_sample` and `end_sample_exclusive`
are the clipped source interval. RMS uses that interval directly, so boundary
padding does not lower its value.

Stereo retains each channel through the FFT and averages power across channels.
RMS likewise averages squared samples across both channels before taking the
square root. An opposite-polarity stereo recording therefore keeps its measured
energy. The waveform plot shows both original channels.

## Feature contract

One-sided FFT bin power `P[k]` is the mean squared FFT magnitude across channels,
divided by `N * sum(window**2)`. Interior bins count twice to include the
negative-frequency power. The spectrogram shows `10 * log10(P[k])` down to
−120 dBFS with extra vertical space for low frequencies.

| Feature | Calculation | What to look for |
|---|---|---|
| RMS | Square root of mean squared source amplitude over the clipped interval | A steady 0.5-peak sine has RMS near 0.354 FS. Perceived loudness also depends on frequency and listening level. |
| Onset strength | Euclidean norm of the positive bin-by-bin increase in `sqrt(P[k])` from the previous frame | A pick attack can create a peak. Changing noise or leakage around a cutoff can also produce peaks. |
| Centroid | Power-weighted mean of bin frequencies in Hz | Higher values indicate that more of the measured energy lies at higher frequencies. A 750 Hz sine centers near 750 Hz. |
| Flatness | Geometric mean of bin power divided by arithmetic mean, with a floor of `peak_bin_power * 1e-12` in the logarithm | A steady sine has a low ratio. Broadband noise and a click can both have high ratios. |

Frames at or below `1e-4` FS RMS (−80 dBFS) have `is_silent: true` and contribute a
zero spectrum to onset strength. Centroid and flatness are `null` in those frames
and in frames with no windowed power. Onset strength is `null` for the first
frame because it has no predecessor.

The JSON records schema version 1, the WAV filename and SHA-256 of the exact
bytes decoded, the sample rate, channel count, source frame count, analysis
settings, feature units and the frame records. The time origin is the first
decoded source sample. Use the integer intervals to align downstream work to
the recording.

## Evidence and source material

The fixture is generated entirely by `fixture.py` under the repository license.
It has a single impulse at 0.25 s, a 750 Hz sine from 0.50 to 1.00 s, seeded
white noise from 1.25 to 1.75 s, and silence elsewhere. See the
[example map](../../docs/evidence/layer-9/sample-map.json),
[feature plot](../../docs/evidence/layer-9/analysis.png) and
[Step 10 devlog](../../docs/devlog/2026-11-09-step-10-sample-analysis.md)
for measured values and tolerances.

## Run the annotated recording study

The [corpus manifest](corpus/manifest.json) records the creator, source and
rights for each WAV, the exact bytes used in the study, and half-open regions in
source samples. It includes a guitar note, a guitar chord, a tenor sax note,
claves with a quiet tail, a rain field recording and the generated fixture.

```sh
python experiments/sample-analysis/study.py > build/sample-analysis-study.json
python experiments/sample-analysis/analyze.py \
    experiments/sample-analysis/corpus/guitar-e2.wav build/sample-analysis-e2 --plot
```

The study checks each SHA-256, measures regional feature summaries, compares
annotated attacks with local onset peaks, and sweeps pitch confidence thresholds.
Pitch uses 80 ms windows and is reported separately from `sample-map.json`.
See the [measured results](../../docs/devlog/2026-11-09-step-10-sample-analysis.md#annotated-recording-study)
for the pitch method, reference notes and failure cases.

### Use your local guitar WAV

Use the waveform or your existing plot to mark a clean single-note region, a
chord or a quiet tail. Convert each time to a sample index with
`round(seconds * sample_rate_hz)` using the rate from your sample map. Choose
regions longer than 100 ms so both analyses have complete windows. Copy the
manifest structure into a local folder such as `build/guitar-study/`, and add one
entry with `file` (relative to that manifest), `sha256` (from your sample map),
and your `regions`. Set `expected_f0_hz` for a single note whose pitch you know
independently, and optionally set `attack_sample` on the recording entry.

```sh
python experiments/sample-analysis/study.py --manifest build/guitar-study/manifest.json \
    > build/guitar-study/report.json
```
