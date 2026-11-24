# Drizel as an Ableton audio effect + MIDI synth

**Live FX (default)**: Place Drizel on an audio track, play audio, and select **Live FX** in DAW audio. It consumes the incoming samples directly into a 2-second rolling stereo history, renders 80ms grains in real time, and replaces the input with transformed granular audio. Position selects a 0.4–1.1 second look-behind; pitch and density reshape the output. Allow roughly 0.4 seconds of input history before wet audio starts. No WAV loading or capture button is required. This first implementation is fully wet, with no dry/wet knob.

**MIDI (sound generation)**: Select **MIDI** instead of **Drone**. A loaded WAV is played through the existing grain instrument when MIDI channel-1 notes arrive. If no WAV is loaded, a simple built-in sine oscillator produces a tone, gated by the existing MIDI attack/release envelope. The MIDI mode overrides Live FX processing, so place a second Drizel instance on an audio track when you also want live granular audio processing.

**Other DAW audio modes**: Audio monitor passes host audio through while the existing loaded-WAV engine can play. Audio capture retains the experimental record/stop/use-take workflow. Audio off keeps the original loaded-WAV behavior. These modes are optional, not the primary Live FX path.

**Ableton MIDI routing limitation**: This plugin is registered as a VST3 audio effect to accept host audio. Ableton may not route MIDI clips directly to it in all configurations; testing the actual version and routing is necessary. A conventional VST3 cannot inspect an entire future Ableton MIDI clip.

## Local verification

1. Build the new branch and rescan Drizel as an audio effect.
2. Put a drum or guitar clip on an audio track; insert Drizel and choose **Live FX** with **Drone** mode. Play the track for at least 0.4s; vary POSITION, PITCH and DENSITY to hear grain transformations.
3. Use a separate instance with **MIDI** mode. Route channel-1 notes from a MIDI-capable host or Ableton routing. Confirm a sine tone without any WAV, then load a WAV and verify granular note playback.
4. Check mono/stereo, 44.1/48/96kHz, varying buffer sizes, project save/restore, offline rendering, and CPU use.

**This is an unverified development branch:** code has not been compiled on a Mac or tested in Ableton from this session.
