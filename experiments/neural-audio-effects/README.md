# Step 14.1 — Offline neural audio-effect reference

This is a **DSP baseline**, not a trained neural network or a real-time plugin effect.

## Setup

```bash
cd experiments/neural-audio-effects
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
python -m pytest -q
```

## Generate paired data

```bash
python cli.py dataset output/pairs --rate 48000 --duration 1 --seed 14 --channels 1 --drive 2 --mix 1 --output-gain 1
python cli.py process input.wav output/processed.wav --drive 2 --mix 0.8 --output-gain 0.9
```

`output/` is generated data and should not be committed. The manifest records the parameters and relative filenames. The five synthetic sources are sine, harmonic, impulse, seeded noise, and silence. All WAVs are floating-point PCM; output exceeding ±1 is rejected, not clipped. Mono/stereo sample rate is preserved; multi-channel, empty, nonfinite and out-of-range inputs are rejected. No resampling is performed. The deterministic signal seed does not make third-party library versions bitwise identical across environments.

**Important:** Generated pairs are useful for a pipeline smoke test but not sufficient for generalization claims. Expand with licensed real recordings, varied drives and recording-level train/validation/test splits in Step 14.2. The simple tanh reference is memoryless and does not model oversampling or alias suppression.
