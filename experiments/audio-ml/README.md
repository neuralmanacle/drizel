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
`attack_sample`. Guitar/sax/sine and claves decay are provisionally "tonal";
rain/noise are "noise"; quiet tail and digital silence are "silence".
These are **operational proxy labels**, not independently reviewed ground truth.
An unannotated frame is excluded rather than silently labeled as silence.

## Evaluation

Leave-one-original-recording-out folds prevent adjacent windows from the same
recording appearing in train and test. Feature means/std are fit on training
frames only. All models use the same four features. The comparison includes
majority, fixed DSP rules, NumPy softmax regression and a 12-hidden-unit tanh MLP.
The random seed is fixed. Each fold records its missing training classes and
per-class precision, recall, F1, macro-F1 over observed classes and confusion
matrix. The report deliberately **does not** rank models as deployment-ready:
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
