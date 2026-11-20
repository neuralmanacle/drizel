"""Step 14.3: evaluate a conditioned neural saturation model offline."""
import argparse
import hashlib
import json
import platform
from pathlib import Path
from time import perf_counter_ns

import numpy as np

from audio_io import read_wav, write_wav
from effect import saturate
from neural import metrics

TRAIN = ("guitar-e2.wav", "tenor-sax-c4.wav")
VALIDATION = ("rain.wav",)
TEST = ("guitar-chord.wav", "claves.wav")
TRAIN_LEVELS = (0.2, 0.6, 1.0)
TEST_LEVELS = (0.08, 0.4, 0.85)
TRAIN_DRIVES = (1.0, 2.0, 4.0)
TEST_DRIVES = (1.5, 3.0, 4.0)
BLOCKS = (64, 128, 256)
WIDTHS = (8, 16, 32)
SEED = 14


def features(samples, drive, slopes, biases):
    driven = samples.reshape(-1, 1) * drive
    # Odd symmetry guarantees zero output at digital silence.
    return (np.tanh(driven * slopes + biases)
            - np.tanh(-driven * slopes + biases)) / (2 * np.tanh(drive))


def fit(training, width):
    rng = np.random.default_rng(SEED)
    slopes = rng.uniform(0.15, 1.8, width)
    biases = rng.uniform(0.2, 2.5, width)
    design = []
    targets = []
    for recording in training:
        for level in TRAIN_LEVELS:
            for drive in TRAIN_DRIVES:
                audio = recording * level
                indices = rng.choice(len(audio), min(len(audio), 2048), replace=False)
                x = audio[indices]
                design.append(features(x, drive, slopes, biases))
                targets.append(saturate(x, drive).reshape(-1))
    matrix = np.concatenate(design)
    weights = np.linalg.lstsq(
        np.vstack((matrix, 0.001 * np.eye(width))),
        np.concatenate((*targets, np.zeros(width))), rcond=None,
    )[0]
    return slopes, biases, weights


def predict(audio, drive, model, mix=1.0, gain=1.0):
    if drive == 0:
        return audio * gain
    slopes, biases, weights = model
    wet = (features(audio, drive, slopes, biases) @ weights).reshape(audio.shape)
    return gain * ((1 - mix) * audio + mix * wet)


def rms(audio):
    return float(np.sqrt(np.mean(audio ** 2)))


def load_recordings(corpus, names):
    manifest = json.loads((corpus / "manifest.json").read_text())
    hashes = {entry["file"]: entry["sha256"] for entry in manifest["recordings"]}
    recordings = {}
    for name in names:
        source = corpus / name
        if hashlib.sha256(source.read_bytes()).hexdigest() != hashes[name]:
            raise ValueError(f"Unexpected recording content: {name}")
        audio, rate = read_wav(source)
        recordings[name] = (audio / np.max(np.abs(audio)), rate)
    return recordings


def render_tests(recordings, model, output):
    rows = []
    for name, (audio, rate) in recordings.items():
        for level in TEST_LEVELS:
            for drive in TEST_DRIVES:
                dry = audio * level
                reference = saturate(dry, drive)
                neural = predict(dry, drive, model)
                rows.append({
                    "source": name, "rate": rate, "input_peak": level, "drive": drive,
                    "neural": metrics(reference, neural), "dry": metrics(reference, dry),
                    "neural_minus_dsp_rms_db": 20 * np.log10(rms(neural) / rms(reference)),
                })
                if level == 0.4 and drive == 3.0:
                    # Match RMS across all versions and leave common peak headroom.
                    versions = {"dry": dry, "dsp": reference, "neural": neural}
                    unit = {key: value / rms(value) for key, value in versions.items()}
                    target = min(0.1, 0.95 / max(np.max(np.abs(x)) for x in unit.values()))
                    for key, value in unit.items():
                        write_wav(output / f"{Path(name).stem}-{key}.wav", value * target, rate)
                    rows[-1]["listening_rms_dbfs"] = float(20 * np.log10(target))
    return rows


def benchmark(model):
    rows = []
    rng = np.random.default_rng(SEED)
    for frames in BLOCKS:
        chunk = rng.uniform(-0.85, 0.85, (frames, 2))
        reference = predict(chunk, 3.0, model)
        np.testing.assert_allclose(
            np.concatenate([predict(part, 3.0, model) for part in np.array_split(chunk, 3)]),
            reference, rtol=0, atol=1e-10,
        )
        for algorithm, process in (("dsp", saturate),
                                   ("neural", lambda x, drive: predict(x, drive, model))):
            for _ in range(100):
                process(chunk, 3.0)
            timings = []
            for _ in range(1000):
                start = perf_counter_ns()
                process(chunk, 3.0)
                timings.append((perf_counter_ns() - start) / 1000)
            rows.append({
                "algorithm": algorithm, "frames": frames, "channels": 2,
                "median_us": float(np.median(timings)), "p99_us": float(np.percentile(timings, 99)),
                "max_us": max(timings),
                "deadline_us": {str(rate): 1e6 * frames / rate for rate in (44100, 48000)},
            })
    return rows


def alias_probe(model):
    rows = []
    for rate in (44100, 48000):
        frequency = 10000
        folded = abs(3 * frequency - rate)
        dry = (0.85 * np.sin(2 * np.pi * frequency * np.arange(rate) / rate))[:, None]
        row = {"rate": rate, "input_hz": frequency, "folded_third_hz": folded}
        for key, signal in (("dry", dry), ("dsp", saturate(dry, 3.0)),
                            ("neural", predict(dry, 3.0, model))):
            amplitude = 2 * np.abs(np.fft.rfft(signal[:, 0])) / rate
            row[key + "_folded_third_dbc"] = float(
                20 * np.log10(max(1e-15, amplitude[folded] / amplitude[frequency]))
            )
        rows.append(row)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--corpus", type=Path, default=Path(__file__).resolve().parent.parent
                        / "sample-analysis" / "corpus")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    recordings = load_recordings(args.corpus, TRAIN + VALIDATION + TEST)
    training = [recordings[name][0] for name in TRAIN]
    validation = recordings[VALIDATION[0]][0] * 0.7
    target = saturate(validation, 2.5)
    models = {width: fit(training, width) for width in WIDTHS}
    scores = {width: metrics(target, predict(validation, 2.5, model))
              for width, model in models.items()}
    width = min(scores, key=lambda candidate: scores[candidate]["mse"])
    model = models[width]
    np.savez(args.output / "model.npz", slopes=model[0], biases=model[1], weights=model[2])
    report = {
        "platform": platform.platform(), "python": platform.python_version(),
        "numpy": np.__version__, "seed": SEED,
        "train": TRAIN, "validation": VALIDATION, "test": TEST,
        "train_levels": TRAIN_LEVELS, "test_levels": TEST_LEVELS,
        "train_drives": TRAIN_DRIVES, "test_drives": TEST_DRIVES,
        "validation_level": 0.7, "validation_drive": 2.5, "validation_scores": scores,
        "hidden_width": width, "fitted_weights": width, "total_coefficients": 3 * width,
        "model_bytes_float64": sum(vector.nbytes for vector in model),
        "mix": 1.0, "output_gain": 1.0, "algorithmic_latency_samples": 0,
        "held_out": render_tests({name: recordings[name] for name in TEST}, model, args.output),
        "alias_probe": alias_probe(model), "offline_compute": benchmark(model),
    }
    (args.output / "report.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")


if __name__ == "__main__":
    main()
