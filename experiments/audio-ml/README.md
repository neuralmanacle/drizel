# Step 12 — Audio ML and region classification (Layer 11)

This is an **offline exploratory experiment**, not a deployed Drizel feature.
It consumes the Step 10 `sample-map.json` feature contract (RMS, onset,
centroid, flatness), using existing WAV provenance and annotated regions.

## Run

From the repository root, using the Step 10 Python environment:

```sh
python3 -m venv build/step12-venv
source build/step12-venv/bin/activate
python -m pip install -r experiments/sample-analysis/requirements.txt
python -m unittest discover -s experiments/audio-ml -p 'test_*.py' -v
python experiments/audio-ml/train.py --output build/step12-results
```

Read `build/step12-results/report.json`. No generated files are committed.
The command checks WAV SHA-256 against the existing manifest, runs Step 10
analysis in temporary directories, and assigns labels only to explicitly
annotated regions. Attack is a 35 ms interval following a documented
`attack_sample`. Guitar/sax/sine are provisionally "tonal"; rain/noise are "noise";
explicit digital silence is "silence". Claves decay, quiet tails and mixed or
unknown regions **abstain** rather than receiving invented ground-truth labels.
These are **operational proxy labels**, not independently reviewed ground truth.
An unannotated frame is excluded rather than silently labeled as silence.

## Evaluation

Leave-one-original-recording-out folds prevent adjacent windows from the same
recording appearing in train and test. Feature means/std are fit on training
frames only. All models use the same nine features: log-RMS, onset strength, centroid,
flatness, their four adjacent-frame deltas (RMS delta is unlogged), and
zero-crossing rate from the original WAV. Deltas are causal and computed
within each recording; zero-crossing rate uses mean-channel audio. The comparison includes
majority, fixed DSP rules, NumPy softmax regression and a 12-hidden-unit tanh MLP.
Linear and MLP training use inverse-frequency class weights calculated from
the training partition only; missing classes receive zero weight. The majority
and fixed-rule baselines remain unweighted. The random seed is fixed. Each fold records its missing training classes and
per-class precision, recall, F1, macro-F1 over observed classes and confusion
matrix. `aggregate` pools out-of-fold predictions to give one confusion matrix,
per-class scores and accuracy per model. Per-recording folds remain available;
pooled metrics weight recordings by labeled-frame count. The report deliberately **does not** rank models as deployment-ready:
the corpus is small, proxy-labeled, and some folds lack entire classes.
The thresholds and hyperparameters are exploratory; there is no separate
untouched final test set. No calibrated probabilities are claimed.

## Next evidence gate

Add multiple independent recordings **per class** and per session, write a
reviewed labeling guide with mixed/ambiguous/abstain labels, freeze recording
group splits, choose metrics before final testing, tune only on validation,
then compare audible grain choices against Step 11. If a model beats the DSP
baseline in relevant held-out data, Layer 12 can explore worker-side C++
inference with safe manual/DSP fallback. Never train or infer in the audio
callback.

## Step 12.1 reproducibility and interpretation

Run the commands above from a clean checkout, and compare the generated
`report.json` to the prior Step 12 run; the metrics **must be recomputed** after
label and feature changes. Inspect `dataset[].labeled_frames` for coverage and
`aggregate` for pooled metrics. A fold may contain no test examples for a class;
zero support is not evidence of success on that class. Check per-recording
results rather than selecting models by aggregate accuracy alone.

This is a **proxy-labeled exploratory experiment**, not a validated audio ML
benchmark. The six source WAVs have different recording conditions, no final
untouched test partition, and no human-reviewed ambiguous labels. A held-out
recording is not equivalent to a held-out recording session or instrument family.
Class weighting and extra features are hypotheses, not established gains.
No model weights are deployed, and Step 11 remains the production baseline.

For a stronger next evaluation, expand the manifest with reviewed labels,
multiple recordings per class and distinct sessions; freeze an independent test
partition before hyperparameter selection. Record the failure examples and
compare ML-selected and DSP-selected grain audio by listening.
