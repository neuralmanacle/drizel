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

## Step 14.2 — Train an offline neural saturation model

This experiment uses an independent scalar MLP on each audio sample (1 → 32 → 32 → 1 with tanh activations). It is appropriate for memoryless saturation only; it cannot learn time-dependent effects. Training uses mean squared error, a seeded sample subset, and a **signal-kind holdout** (`noise` by default) for an elementary generalization check. These synthetic signals are not a substitute for independent recordings. The held-out signal is not used in optimization.

```bash
python3 -m pip install -r requirements-ml.txt
python cli.py dataset output/pairs --duration 1 --seed 14
python train.py train output/pairs/manifest.json output/trained --holdout noise --epochs 60
python train.py predict output/trained/model.pt output/pairs/sine_dry.wav output/trained/sine_inferred.wav
python -m pytest -q
```

Outputs: `model.pt` (trusted local PyTorch weights), `report.json` (loss curve, train/held-out metrics), and held-out `*_predicted.wav`. `predict` preserves input sample rate and channel count. Inference is **offline only**. A model trained with one drive/mix setting does not generalize automatically to other settings; Step 14.3 can introduce conditioning. This model may not be bit-identical across hardware/PyTorch versions. The float WAV export clips predicted overs to [-1,1]; evaluation metrics use unclipped predictions.
