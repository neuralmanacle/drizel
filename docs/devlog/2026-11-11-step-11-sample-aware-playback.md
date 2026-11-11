# Teaching Drizel where to start a grain

*Drizel Step 11 · 11 November 2026*

A granular synth can play the same guitar recording as a sharp, percussive texture or as a cloud of sustained tones. The difference can start with a single decision: where each grain begins reading the recording. Until now, Drizel made that decision from the Position control, with MIDI and LFO modulation available to move it. Step 11 adds four ways to choose a start based on the sound near that position.

Load a WAV, play in Drone or MIDI mode, and change **Grain selection** from **Manual** to **Attack**, **Sustain**, **Avoid silence**, or **Bright**. Position still determines which part of the source the selector searches. Move it across a recording with several phrases and the choices follow it. Switch modes while playing and listen as new grains enter the cloud. Each grain already playing keeps the start and pitch it captured at launch.

## From a WAV to a feature frame

[Step 10](2026-11-09-step-10-sample-analysis.md) established the Python reference. It analyzed a WAV and saved a JSON map for inspection. Step 11 ports the three measurements needed for playback to [framework-independent C++](../../src/audio/SampleFeatures.h). The plugin rebuilds the map from the decoded WAV whenever the loader prepares a source. Python, NumPy and the JSON file stay in the offline workflow. Flatness remains available in the Python analysis.

The analyzer advances by 512 **source frames** through a series of centered 2,048-frame windows. At 48 kHz, each window spans 42.67 ms and adjacent centers are 10.67 ms apart. The source rate controls those durations. A 48 kHz WAV yields the same feature timing whether the host runs at 44.1 or 96 kHz.

For each channel, the analyzer multiplies the window by a periodic Hann taper and takes a radix-2 FFT. Samples beyond the WAV boundaries contribute zeros to the FFT. It computes RMS separately from the real samples in the clipped window, before applying the taper. This keeps zero padding from diluting the level at either end.

```text
w[n] = 0.5 * (1 - cos(2πn / N))

rms = sqrt(sum of real sample squares / (real frame count * channels))

P[k] = fold[k] * sum over channels of |FFT(x * w)[k]|²
       / (channels * N * sum(w²))

fold[k] = 1 at DC and Nyquist, 2 in the interior
```

The periodic analysis taper uses a denominator of N. The symmetric Hann grain envelope uses N−1 so its endpoints are both zero. The first controls spectral leakage in the analysis. The second fades each audible grain in and out.

Averaging spectral power across channels preserves energy in opposite-polarity stereo. If the right channel is the inverse of the left, both channels contribute the same power spectrum. From that one-sided spectrum, Drizel computes a centroid and a measure of spectral growth.

```text
frequency[k] = k * sourceRate / N
centroidHz   = sum(frequency[k] * P[k]) / sum(P[k])

magnitude[k] = sqrt(P[k])
increase[k]  = max(0, magnitude[k] - previousMagnitude[k])
onset        = sqrt(sum(increase[k]²))
```

Frames at or below RMS 0.0001 (−80 dBFS in amplitude) contribute zero magnitude to the onset history. The initial frame has no predecessor. The Python reference represents its onset and undefined centroids as null. The C++ selector uses zero for those values.

## Turning measurements into grain starts

A feature frame gives us a time and a few numbers. The selector turns those numbers into weights. The thresholds below are design choices for the first version and can be evaluated against different recordings.

| Mode | Eligible frames and weights |
| --- | --- |
| Manual | Use the smoothed, modulated Position directly, including fractional source frames. |
| Attack | Require onset divided by RMS to be at least 0.2. Weight by absolute onset and start 25 ms before the selected frame center, clamped at the start of the WAV. |
| Sustain | Require a preceding frame and onset divided by RMS below 0.2. Weight by RMS multiplied by the distance from the 0.2 cutoff, scaled to 0–1. |
| Avoid silence | Give each nonsilent analysis hop equal weight when building the table. |
| Bright | Require centroid to reach 5% of Nyquist. Weight by the square of centroid divided by Nyquist. At 48 kHz, the minimum is 1.2 kHz. |

The four guided modes share the RMS gate. Attack divides onset by RMS for the eligibility check so relative spectral growth can qualify at different levels. Its absolute onset weight then favors stronger events among eligible frames. The pre-roll gives the 100 ms playback envelope time to rise before the measured event. A bright frame can also be an attack, and a sustained noise can have a high centroid. These measurements express preferences for grain starts.

The table builder groups consecutive hops into up to 16 regions. It targets about 32 hops per region when sizing a short source, and caps the region count for a long source. At launch, Position selects a region using the same source-frame coordinates. Within each region, the worker groups the hops into at most 128 consecutive buckets. Each bucket keeps the strongest eligible representative for each mode. Avoid silence keeps the first eligible hop as its representative and weights the bucket by its count of eligible hops. This preserves relative bucket coverage while bounding memory for long WAVs.

The worker accumulates the weights into a cumulative distribution. A draw in the interval from zero to the total weight lands in the interval assigned to one representative. The audio thread finds that representative with an upper-bound binary search over at most 128 entries. The tables hold at most 8,192 entries across all regions and modes, equivalent to 128 KiB of entry payload on the tested build at two doubles per entry.

A region without eligible candidates falls back to the exact Manual position, including MIDI and LFO modulation. The region constrains the **feature anchor**. Attack pre-roll can cross its left boundary, and a grain can continue reading across either region boundary. At the end of the WAV, the existing interpolator supplies zeros. Avoid silence checks the analysis window around a start, so a selected start can still lead into or out of a quiet patch.

## Keeping the audio thread predictable

[The realtime engine](../../src/audio/RealtimeEngine.h) already moves prepared WAVs from the loader to playback through two atomic pointer mailboxes. The first transfers a prepared packet to audio. The second returns the old packet to the loader for reclamation. The source, candidate tables and grain cloud now travel together in that packet. The audio thread always reads the table built for the source it is currently rendering.

The processor increments a revision for each load, recall or device preparation. Analysis checks that revision and the worker's exit signal between windows. Publication checks the revision again under the existing request lock. At handoff, playback validates the packet revision and output rate, then uses the existing 10 ms fades around a successful swap. If an in-flight analysis is superseded, the worker drops its partial map. The current source and its table remain playable until a replacement arrives.

The processor snapshots the choice parameter once per audio block. The grain cloud consults the selector only on a scheduled launch. The scheduler still advances its fractional countdown, the voice pool still caps overlap at 64 grains, and old voices finish their envelopes. A dedicated seeded generator drives feature selection, so enabling a mode leaves the pan, pitch, length and jitter draw sequence intact. Re-preparing the cloud resets its seed. Switching modes continues the guided sequence from its last draw.

Preset and host state store the mode and sample path. Recall rebuilds the table from the file at that path, so replacing the WAV there changes the next analysis. Older presets load in Manual mode. The source analysis is regenerated on device preparation as part of the existing worker flow, which also rebuilds the output-rate-dependent grain windows.

## What the checks showed

I compared the C++ port against the Step 10 Python implementation on 188 windows of the two-second synthetic fixture, with the inverse of the first channel in the second. The fixture combines silence, an impulse, a 750 Hz tone and noise. These are maximum absolute differences across the comparison.

| Field | Maximum absolute difference | Test tolerance |
| --- | ---: | ---: |
| Center | 0 source frames | Exact |
| RMS | 0 | 1 × 10⁻¹² |
| Onset | 1.11 × 10⁻¹⁶ | 1 × 10⁻¹² |
| Centroid | 2.66 × 10⁻¹⁰ Hz | 1 × 10⁻⁸ Hz |

The C++ tests also pin a smaller set of Python reference values, exercise the mode rules and local fallback, and reject an obsolete packet with a matching sample rate. The allocation guard checks guided playback for allocation and reclamation on the render thread. The processor test covers new and older preset files.

For a same-source comparison, I rendered from Position 0 with a density of 100 grains per second and output gain at 0.8. The opening of the fixture is silent and its first region includes the impulse. The Manual take had zero peak amplitude. Avoid silence drew from nonsilent windows in that region and produced a left-channel peak of 0.0110 FS, about −39.2 dBFS.

A separate optimized C++ probe measured the engine on an arm64 Mac at 48 kHz. Each trial rendered ten seconds into 256-frame blocks. The table reports the median of seven renders and the median of their 99th-percentile block times. In the rebuild condition, a second thread repeatedly built tables from the same fixture while the engine rendered from an already-prepared packet. One table build took about 6.9 ms for this two-second stereo fixture, excluding file decoding.

| Render condition | Time for 10 seconds of audio | Block p99 |
| --- | ---: | ---: |
| Manual | 101.5 ms | 63.0 µs |
| Avoid silence | 101.5 ms | 64.3 µs |
| Avoid silence during worker rebuilds | 101.9 ms | 56.8 µs |

A 256-frame block at 48 kHz spans 5.33 ms. The probe measured the core render loop on one machine. DAW scheduling, I/O, file decoding and other plugins affect real host deadlines. The small timing differences between runs are within the variation of this probe. The macOS Standalone, VST3 and AU targets built, and all 11 CTest entries and 13 Step 10 Python tests passed.

## Listening for the difference

Try a recording with both a pick attack and a sustained note inside one region. Keep the Position, Pitch and Density controls fixed. Switch from Attack to Sustain and listen for a change in how often the transient enters the cloud. Then move Position to the next phrase and repeat. A sparse recording with gaps makes the Avoid silence mode easier to hear. For Bright, a recording that moves between a muted sound and a higher-frequency passage gives the selector more contrast to work with.

The useful experiment is whether these simple preferences lead to a texture you would choose by ear. The next measurements are a DAW listening pass across more recordings and host-level timing while replacing long WAVs. The selector gives future region classifiers a concrete DSP baseline to compare against.
