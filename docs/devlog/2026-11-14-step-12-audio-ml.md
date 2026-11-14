# Step 12 — Audio ML and region classification
**Date:** 14 November 2026  
**Layer:** 11  
**Branch:** `feature/step-12-audio-ml`

## Objective

Compare deterministic DSP feature rules with a majority classifier, softmax
regression and a small tanh MLP, using the Step 10 sample map and Step 11
grain-selection baseline as the engineering reference.

## Changes

- `experiments/audio-ml/train.py`: verifies source SHA-256, extracts the
  existing Step 10 feature map, assigns provisional region labels, performs
  recording-grouped leave-one-out evaluation and writes JSON metrics.
- `experiments/audio-ml/test_train.py`: unit checks for attack precedence,
  unknown labels, training-only scaling, finite model output and missing-class
  metric reporting.
- `experiments/audio-ml/README.md`: commands, labeling limitations and
  reproducibility guidance.

## Method

Features: RMS, onset strength, spectral centroid and spectral flatness.
Provisional classes: attack, tonal, noise, silence. The labels are mapped from
existing source annotations, not manually reviewed frame-by-frame. We use
leave-one-recording-out folds to prevent leakage across overlapping frames.
Scaler statistics are computed on training data alone. The two learned models
are written with NumPy to keep the experiment independent of plugin builds.

## Verification and limitations

**Code committed; execution not verified in this environment.** No test
successes, F1 scores, or listening outcomes are claimed. Run the documented
commands locally and attach `report.json` to the evidence before making any
accuracy claim. With six short recordings, some held-out folds have classes
absent from training, making generalization claims particularly weak.

**Verdict: defer deployment.** This is a reproducible exploratory scaffold,
not completion of Layer 11. Add independently labeled recordings, a frozen
train/validation/test split, error analysis, a calibrated abstention policy,
and a Step 11 listening comparison before choosing a model for Layer 12.

## Learning topics

1. Dataset leakage and recording-level grouping
2. Training-only feature normalization
3. Multiclass softmax and cross-entropy gradients
4. Dense neural networks and tanh backpropagation
5. Confusion matrices, per-class F1 and missing-class coverage
6. Why classification accuracy alone does not imply better grain selection
