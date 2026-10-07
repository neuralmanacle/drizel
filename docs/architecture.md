# Engine Architecture

## Data flow

```text
SourceBuffer
    ↓
GrainScheduler
    ↓
GrainPool
    ↓
GrainReader
    ↓
Envelope
    ↓
Mixer
    ↓
OutputProcessor
```

## Grain state

Each grain contains:

- Source position
- Current read position
- Playback rate
- Duration
- Current age
- Amplitude
- Pan
- Active state

## Processing model

The engine processes audio in blocks. It does not allocate memory,
load files, or perform blocking operations inside the audio callback.