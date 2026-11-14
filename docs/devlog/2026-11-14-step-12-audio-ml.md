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


## Step 12.1 follow-up — conservative labeling and class balancing

The first user-run evaluation completed on six recordings (386 labeled frames).
It exposed weak cross-recording generalization: both learned models missed
attack and noise in the pooled evaluation, and the DSP-rule baseline had higher
macro-F1. These are *Step 12* results, **not** results of Step 12.1.

Step 12.1 changes:
- Abstain on ambiguous quiet claves tails and percussive claves decay rather
  than asserting that they are silence or sustained tonal material.
- Add log-RMS, frame-to-frame changes in RMS/onset/centroid/flatness, and
  waveform zero-crossing rate. The original Step 10 feature map stays v1.
- Apply inverse-frequency loss weights to both learned classifiers, computed
  only from each fold's training labels.
- Report pooled out-of-fold confusion matrices, accuracy, per-class scores,
  and recording-specific fold metrics; preserve recording-level isolation.
- Add regression tests for abstention, class weights, temporal features and
  aggregate metrics.

**Verification status:** Changes were committed to the development branch,
and the source was inspected. No Python test execution or fresh Step 12.1
evaluation was possible in the authoring environment. Re-run the documented
commands locally; do not reuse the Step 12 report as Step 12.1 evidence.
The new run should be reviewed for label coverage and failure cases.

**Deployment verdict: defer.** No C++ audio callback or plugin inference change.
