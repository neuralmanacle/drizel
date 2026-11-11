"""Compare offline sample features and pitch periodicity with annotated WAVs."""

import argparse
import json
from pathlib import Path
import tempfile

import numpy as np
from scipy.io import wavfile
from scipy.signal import fftconvolve

from analyze import analyze, load_wav
from fixture import SAMPLE_RATE, fixture_samples


FRAME_SIZE = 2048
HOP_SIZE = 512
PITCH_WINDOW_SECONDS = 0.080
PITCH_RANGE_HZ = (60, 1000)
PITCH_RMS_FLOOR = 0.001
NEAR_PEAK_RATIO = 0.95
SCORE_THRESHOLDS = (0.60, 0.80, 0.95)
NOTE_TOLERANCE_CENTS = 50
CORPUS = Path(__file__).with_name("corpus") / "manifest.json"


def pitch_candidate(samples, sample_rate):
    """Return (Hz, normalized autocorrelation) or (None, None)."""
    length = len(samples)
    min_lag = max(2, int(np.ceil(sample_rate / PITCH_RANGE_HZ[1])))
    max_lag = min(length // 3, sample_rate // PITCH_RANGE_HZ[0])
    if min_lag > max_lag:
        return None, None

    centered = samples - samples.mean(axis=0)
    energy = np.cumsum(np.sum(centered**2, axis=1))
    if np.sqrt(energy[-1] / centered.size) < PITCH_RMS_FLOOR:
        return None, None

    # The extra lags let us compare the edge candidates with their neighbors.
    lags = np.arange(min_lag - 1, max_lag + 2)
    correlation = fftconvolve(centered, centered[::-1], axes=0)
    dot_products = correlation[length - 1 + lags].sum(axis=1)
    normalization = np.sqrt(energy[length - lags - 1] * (energy[-1] - energy[lags - 1]))
    scores = np.divide(dot_products, normalization, out=np.zeros_like(normalization),
                       where=normalization > 0)
    scores = np.clip(scores, -1, 1)
    peaks = np.flatnonzero((scores[1:-1] > scores[:-2]) &
                           (scores[1:-1] >= scores[2:]) & (scores[1:-1] > 0)) + 1
    if peaks.size == 0:
        return None, None

    # Prefer the shortest lag near the best peak to avoid period multiples.
    best = scores[peaks].max()
    peak = peaks[scores[peaks] >= NEAR_PEAK_RATIO * best][0]
    left, middle, right = scores[peak - 1:peak + 2]
    offset = 0.5 * (left - right) / (left - 2 * middle + right)
    return float(sample_rate / (lags[peak] + offset)), float(middle)


def median(values):
    available = [value for value in values if value is not None]
    return float(np.median(available)) if available else None


def study_region(region, frames, samples, sample_rate):
    start = region["start_sample"]
    end = region["end_sample_exclusive"]
    if type(start) is not int or type(end) is not int or not 0 <= start < end <= len(samples):
        raise ValueError(f"Invalid source interval for {region['label']}")

    # Exclude windows that include padding or samples from another annotation.
    half_frame = FRAME_SIZE // 2
    interior = [frame for frame in frames if
                start + half_frame <= frame["center_sample"] <= end - half_frame]
    if not interior:
        raise ValueError(f"No full feature frames for {region['label']}")
    features = {key: median(frame[key] for frame in interior)
                for key in ("rms", "centroid_hz", "flatness")}
    features["onset_strength"] = median(frame["onset_strength"] for frame in interior
                                        if frame["center_sample"] >= start + half_frame + HOP_SIZE)
    features["silent_frames"] = sum(frame["is_silent"] for frame in interior)

    pitch_radius = round(PITCH_WINDOW_SECONDS * sample_rate / 2)
    candidates = [pitch_candidate(samples[center - pitch_radius:center + pitch_radius], sample_rate)
                  for center in range(0, len(samples), HOP_SIZE)
                  if start + pitch_radius <= center <= end - pitch_radius]
    scored = [(hz, score) for hz, score in candidates if score is not None]
    reference_hz = region.get("expected_f0_hz")
    if reference_hz is not None and (not np.isfinite(reference_hz) or reference_hz <= 0):
        raise ValueError(f"Invalid reference frequency for {region['label']}")
    sweep = []
    for threshold in SCORE_THRESHOLDS:
        accepted = [hz for hz, score in scored if score >= threshold]
        errors = [] if reference_hz is None else [abs(1200 * np.log2(hz / reference_hz))
                                                  for hz in accepted]
        sweep.append({
            "threshold": threshold,
            "accepted": len(accepted),
            "within_tolerance": None if reference_hz is None else
                sum(bool(error <= NOTE_TOLERANCE_CENTS) for error in errors),
            "median_hz": median(accepted),
            "median_absolute_error_cents": median(errors),
        })
    return {
        **region,
        "feature_frames": len(interior),
        "feature_summary": features,
        "pitch_frames": len(candidates),
        "median_periodicity": median(score for _, score in scored),
        "threshold_sweep": sweep,
    }


def study_attack(sample, frames, sample_rate):
    if type(sample) is not int or not 0 <= sample < frames[-1]["end_sample_exclusive"]:
        raise ValueError("Attack annotation is outside the source")
    peak = max((frame for frame in frames if frame["onset_strength"] is not None
                and abs(frame["center_sample"] - sample) <= FRAME_SIZE),
               key=lambda frame: frame["onset_strength"], default=None)
    if peak is None:
        return None
    return {
        "annotated_sample": sample,
        "peak_center_sample": peak["center_sample"],
        "offset_ms": 1000 * (peak["center_sample"] - sample) / sample_rate,
        "window_contains_annotation": peak["start_sample"] <= sample < peak["end_sample_exclusive"],
        "peak_strength": peak["onset_strength"],
    }


def study_corpus(manifest):
    recordings = []
    for recording in json.loads(manifest.read_text(encoding="utf-8"))["recordings"]:
        name = recording["file"]
        if recording.get("generated_fixture"):
            with tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / "fixture.wav"
                wavfile.write(path, SAMPLE_RATE, fixture_samples())
                sample_rate, samples, digest = load_wav(path)
        else:
            sample_rate, samples, digest = load_wav(manifest.parent / name)
        if digest != recording["sha256"]:
            raise ValueError(f"SHA-256 mismatch for {name}")

        frames, _ = analyze(samples, sample_rate, FRAME_SIZE, HOP_SIZE)
        entry = {"file": name, "sha256": digest, "sample_rate_hz": sample_rate}
        if "attack_sample" in recording:
            entry["attack"] = study_attack(recording["attack_sample"], frames, sample_rate)
        entry["regions"] = [study_region(region, frames, samples, sample_rate)
                            for region in recording["regions"]]
        recordings.append(entry)
    return {
        "frame_size_samples": FRAME_SIZE,
        "hop_size_samples": HOP_SIZE,
        "pitch_window_seconds": PITCH_WINDOW_SECONDS,
        "pitch_range_hz": PITCH_RANGE_HZ,
        "pitch_rms_floor": PITCH_RMS_FLOOR,
        "near_peak_ratio": NEAR_PEAK_RATIO,
        "note_tolerance_cents": NOTE_TOLERANCE_CENTS,
        "recordings": recordings,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=CORPUS,
                        help="annotation manifest (defaults to the Step 10 corpus)")
    args = parser.parse_args()
    try:
        print(json.dumps(study_corpus(args.manifest), indent=2, allow_nan=False))
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
