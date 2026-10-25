# Layer 8 — Interface and packaging: first increment

This increment replaces the generic test panel with an instrument editor.
Layer 8 is not complete: shipping and host validation remain open.

## Use

1. Build the Standalone or plugin using the existing CMake or Projucer route.
2. Choose **Load WAV**, or drop one WAV onto the editor. Loading runs on the
   existing worker; a failed load leaves the previous sound and waveform intact.
3. Adjust Position, Pitch, Density and Output gain. Play controls smoothed gain.
4. Choose the LFO shape and destination, then raise depth and adjust rate.

The waveform is decoded separately by JUCE AudioThumbnail, whose cache uses a
background thread. It follows the most recently prepared source path, including
host-state restoration; the engine may still be fading to that source. The white
marker shows the **base position**, not individual grains or the modulated target.
Active grains reports the actual voice count at the last processed block end.
The output bar displays the latest block peak with a UI decay; short peaks between
UI polls can be missed. It is not a true-peak meter or clipping-history indicator.
A successful reload refreshes the thumbnail even when the file path is unchanged.

## Threading and ownership

APVTS attachments bind controls bidirectionally to existing host parameters.
Automation and saved values therefore update the same controls used by the DSP.
No parameter IDs or defaults changed. Attachments are destroyed before their
controls. Asynchronous file-dialog completion uses a SafePointer.

The audio thread counts active voices once per block and publishes that count
and a numeric output peak through lock-free atomics. The UI polls at 30 Hz.
Thumbnail decoding, strings, component painting and status locks stay outside
processBlock. The engine telemetry accessor is audio-thread-only; the editor
never dereferences the active sample or cloud.

## Validation and remaining acceptance checks

The numerical suite includes unloaded, playing and restarted activity checks,
with the existing callback allocation/deallocation guard covering telemetry.
CMake and Projucer already compile the changed editor/processor files; no new
compiled source is needed. The guide is registered in the Projucer tree.

On the target Mac, build both Debug and Release Standalone/VST3 (and AU if used):

```sh
cmake -S . -B build-plugin -DDRIZEL_JUCE_PATH=../JUCE -DDRIZEL_BUILD_PLUGIN=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-plugin --parallel 2
```

Manual acceptance still required:

- Open/close the editor repeatedly, including with a file dialog or waveform
  decode in flight; drop valid, invalid, mono and stereo WAVs.
- Automate every control and confirm UI and audible changes; restore saved state
  with both present and missing source files, including an empty saved path.
- Observe waveform, base marker, activity and output during playback and swaps.
- Test host save/reload in at least two hosts and run a plugin validation tool.
- Listen at 64–512 sample buffers and measure worst-case callback deadlines.
- Check keyboard access, screen-reader labels and display scaling on the target.
- Review AGPL-3.0 and JUCE obligations, then prepare distribution/install assets.

No packaged release, host validation, target-Mac build, or audible quality claim
is made by this implementation increment. Earlier MIDI and modulation roadmap
layers also retain their documented open items.
