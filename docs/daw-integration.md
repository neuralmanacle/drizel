# Ableton / DAW input testing (experimental)

Drizel can now accept a stereo or mono host audio input while retaining MIDI input support. It cannot access arbitrary clips in the Ableton project: the DAW must route and play the audio/MIDI to the plugin.

## Build

Use CMake with `-DDRIZEL_BUILD_PLUGIN=ON`, or regenerate the Xcode project using `drizel.jucer`. The plugin is now classified as an **audio effect** (not a synth), so Live should allow insertion on an audio track. This changes the AU component type as well; rescan and replace old plugin instances, retaining a backup of project files.

## Audio track: use incoming audio

1. Put Drizel on an audio track with an audio clip (or route another track's audio to it).
2. In the Drizel interface, choose **Audio monitor** to hear the incoming audio in addition to Drizel's synthesized grains. **Audio off** is the previous synth-only behavior.
3. For a sample captured from the DAW, choose **Audio capture**, click **Record DAW**, start playback in Live, then click **Stop DAW**. After audio has processed another block, click **Use DAW take**. This exports a temporary WAV and loads it into Drizel's existing sample loader.
4. Capture is limited to 10 seconds at the current sample rate. The capture buffer is allocated in `prepareToPlay`; writes and stopping happen inside the audio callback, while WAV exporting happens outside it.

The WAV is saved to the system temporary directory and may be deleted by the OS; for durable samples, export and copy the take from there separately. DAW capture is **not** project-wide sample discovery.

## MIDI routing

Drizel already handles incoming MIDI from the host on **channel 1**. Select **MIDI** instead of **Drone** in the plugin UI, load a WAV or use a captured DAW take, and send notes to the plugin. Ableton Live's MIDI routing to a VST3 audio effect varies by track and version; use an upstream MIDI instrument chain/routing configuration or a MIDI file/keyboard in a host that supports MIDI-to-effect routing. A generic VST3 cannot read the entire future MIDI clip from Live.

## Validation

- Build both the CMake VST3 and the Projucer-generated plugin and check for errors.
- Run `ctest --test-dir build --output-on-failure` for the offline tests; with `DRIZEL_BUILD_PLUGIN=ON`, run the processor integration test.
- In Live, inspect the FX versus Instrument category, MIDI routing, sample loading, stereo monitoring, ten-second capture, and restart/reload behavior.
- In the processor tests, audio monitor preserves host audio, captured DAW audio exports as WAV, and incoming channel-1 MIDI triggers a note.

**Note:** These changes have not been compiled or run on an Ableton/macOS installation from this session. Treat this branch as a reviewable prototype rather than a verified release.
