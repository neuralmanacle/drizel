"""Step 12: offline, recording-grouped audio-region classification.

Requires NumPy and the Step 10 analysis dependencies. No model runs in processBlock.
This experiment intentionally reports missing-class coverage instead of claiming
generalisation from six short recordings.
"""
import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
LABELS = ("attack", "tonal", "noise", "silence")
FEATURES = ("rms", "onset_strength", "centroid_hz", "flatness")
SEED = 12012


def label_for(recording, frame, sample_rate):
    center = frame["center_sample"]
    attack = recording.get("attack_sample")
    if attack is not None and attack <= center < attack + round(.035 * sample_rate):
        return "attack"
    for region in recording["regions"]:
        if region["start_sample"] <= center < region["end_sample_exclusive"]:
            name = region["label"].lower()
            if "silence" in name or "quiet" in name:
                return "silence"
            if "noise" in name or "rain" in name:
                return "noise"
            if any(word in name for word in ("sine", "guitar", "sax", "claves")):
                return "tonal"
    return None  # Never treat unannotated material as negative examples.


def vector(frame):
    values = [frame.get(key) for key in FEATURES]
    return np.array([0.0 if value is None else float(value) for value in values])


def load_dataset(manifest_path, cache_dir):
    manifest_path = Path(manifest_path).resolve()
    manifest = json.loads(manifest_path.read_text())
    xs, ys, groups = [], [], []
    provenance = []
    for recording in manifest["recordings"]:
        wav = (manifest_path.parent / recording["file"]).resolve()
        if not wav.is_file() and recording.get("generated_fixture"):
            wav.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run([sys.executable,
                            str(ROOT / "experiments/sample-analysis/fixture.py"),
                            str(wav)], check=True, capture_output=True, text=True)
        if not wav.is_file():
            raise FileNotFoundError(
                f"Missing corpus recording: {wav}. Restore the original WAV "
                "specified in experiments/sample-analysis/corpus/manifest.json; "
                "only generated fixtures can be recreated automatically."
            )
        digest = hashlib.sha256(wav.read_bytes()).hexdigest()
        if digest != recording["sha256"]:
            raise ValueError("WAV checksum mismatch: " + str(wav))
        output = cache_dir / digest
        output.parent.mkdir(parents=True, exist_ok=True)
        map_path = output / "sample-map.json"
        if not map_path.exists():
            subprocess.run([sys.executable, str(ROOT / "experiments/sample-analysis/analyze.py"),
                            str(wav), str(output)], check=True, capture_output=True, text=True)
        sample_map = json.loads(map_path.read_text())
        if sample_map["source"]["sha256"] != digest or sample_map["schema_version"] != 1:
            raise ValueError("Stale or incompatible sample map: " + str(map_path))
        rate = sample_map["source"]["sample_rate_hz"]
        count = 0
        for frame in sample_map["frames"]:
            label = label_for(recording, frame, rate)
            if label is not None:
                xs.append(vector(frame))
                ys.append(LABELS.index(label))
                groups.append(digest)
                count += 1
        provenance.append({"file": recording["file"], "sha256": digest,
                           "license": recording["license"], "labeled_frames": count})
    if not xs:
        raise ValueError("No annotated frames")
    return np.stack(xs), np.array(ys), np.array(groups), provenance


def standardize(train, other):
    mean = train.mean(axis=0)
    std = train.std(axis=0)
    std = np.where(std < 1e-9, 1.0, std)
    return (train - mean) / std, (other - mean) / std, mean, std


def softmax(logits):
    z = logits - logits.max(axis=1, keepdims=True)
    exp = np.exp(z)
    return exp / exp.sum(axis=1, keepdims=True)


def train_linear(x, y, epochs=300, lr=.08):
    rng = np.random.default_rng(SEED)
    w = rng.normal(0, .01, (x.shape[1], len(LABELS)))
    b = np.zeros(len(LABELS))
    target = np.eye(len(LABELS))[y]
    for _ in range(epochs):
        diff = (softmax(x @ w + b) - target) / len(y)
        w -= lr * (x.T @ diff + .001 * w)
        b -= lr * diff.sum(axis=0)
    return lambda values: values @ w + b, {"w": w.tolist(), "b": b.tolist()}


def train_mlp(x, y, epochs=350, lr=.035):
    rng = np.random.default_rng(SEED)
    w1 = rng.normal(0, .15, (x.shape[1], 12))
    b1 = np.zeros(12)
    w2 = rng.normal(0, .15, (12, len(LABELS)))
    b2 = np.zeros(len(LABELS))
    target = np.eye(len(LABELS))[y]
    for _ in range(epochs):
        h = np.tanh(x @ w1 + b1)
        diff = (softmax(h @ w2 + b2) - target) / len(y)
        dw2 = h.T @ diff + .001 * w2
        db2 = diff.sum(axis=0)
        dh = (diff @ w2.T) * (1 - h * h)
        dw1 = x.T @ dh + .001 * w1
        db1 = dh.sum(axis=0)
        w2 -= lr * dw2
        b2 -= lr * db2
        w1 -= lr * dw1
        b1 -= lr * db1
    return lambda values: np.tanh(values @ w1 + b1) @ w2 + b2, {
        "w1": w1.tolist(), "b1": b1.tolist(),
        "w2": w2.tolist(), "b2": b2.tolist()}


def metrics(truth, predicted):
    matrix = np.zeros((len(LABELS), len(LABELS)), dtype=int)
    for actual, guess in zip(truth, predicted):
        matrix[actual, guess] += 1
    per_class = {}
    f1s = []
    for i, label in enumerate(LABELS):
        tp = matrix[i, i]
        precision = float(tp / max(1, matrix[:, i].sum()))
        recall = float(tp / max(1, matrix[i].sum()))
        f1 = 2 * precision * recall / max(1e-12, precision + recall)
        per_class[label] = {"support": int(matrix[i].sum()), "precision": precision,
                            "recall": recall, "f1": f1}
        if matrix[i].sum():
            f1s.append(f1)
    return {"macro_f1_observed_classes": float(np.mean(f1s)) if f1s else None,
            "per_class": per_class, "confusion_matrix": matrix.tolist()}


def evaluate(x, y, groups):
    # Leave-one-recording-out: no frames from the same WAV cross a fold.
    folds = []
    for held_out in sorted(set(groups)):
        test = groups == held_out
        train = ~test
        xt, xv, mean, std = standardize(x[train], x[test])
        counts = np.bincount(y[train], minlength=len(LABELS))
        majority = int(np.argmax(counts))
        linear, _ = train_linear(xt, y[train])
        mlp, _ = train_mlp(xt, y[train])
        predictions = {
            "majority": np.full(test.sum(), majority, dtype=int),
            "rule": np.array([0 if a[1] > .02 else 3 if a[0] < 1e-4 else 2 if a[3] > .5 else 1
                              for a in x[test]], dtype=int),
            "linear": linear(xv).argmax(axis=1),
            "mlp": mlp(xv).argmax(axis=1),
        }
        folds.append({"held_out_sha256": held_out, "test_frames": int(test.sum()),
                      "train_labels": [LABELS[i] for i in np.flatnonzero(counts)],
                      "missing_train_labels": [LABELS[i] for i in np.flatnonzero(counts == 0)],
                      "metrics": {name: metrics(y[test], pred)
                                  for name, pred in predictions.items()}})
    return folds


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=ROOT / "experiments/sample-analysis/corpus/manifest.json")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as directory:
        x, y, groups, provenance = load_dataset(args.manifest, Path(directory))
    folds = evaluate(x, y, groups)
    report = {"schema_version": 1, "seed": SEED, "labels": LABELS,
              "features": FEATURES, "dataset": provenance, "folds": folds,
              "limitations": ["Recording-grouped exploratory leave-one-out evaluation",
                              "Missing classes in training folds; not a deployment-ready model",
                              "No independent untouched final test set",
                              "Rule thresholds are exploratory, not tuned on a validation set"],
              "deployment_verdict": "defer"}
    path = args.output / "report.json"
    path.write_text(json.dumps(report, indent=2) + "\n")
    print(path)


if __name__ == "__main__":
    main()
