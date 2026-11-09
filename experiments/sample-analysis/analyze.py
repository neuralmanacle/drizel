"""Offline WAV analysis and the version 1 Drizel sample map."""

import argparse
import hashlib
import io
import json
from pathlib import Path
from struct import error as StructError
import tempfile
import warnings

import numpy as np
from scipy.io import wavfile


SILENCE_RMS = 1e-4
FLATNESS_POWER_FLOOR = 1e-12


def load_wav(path):
    # Decode and hash the same bytes so the map identifies the input we measured.
    source_bytes = path.read_bytes()
    try:
        with warnings.catch_warnings():
            warnings.filterwarnings(
                "error", message="Reached EOF prematurely", category=wavfile.WavFileWarning
            )
            sample_rate, raw = wavfile.read(io.BytesIO(source_bytes))
    except (EOFError, StructError, UnboundLocalError, ValueError,
            ZeroDivisionError, wavfile.WavFileWarning) as error:
        raise ValueError(f"Invalid WAV data ({error})") from error

    if raw.ndim == 1:
        raw = raw[:, None]
    if raw.shape[0] == 0 or sample_rate <= 0:
        raise ValueError("WAV must contain audio at a positive sample rate")
    if raw.shape[1] not in (1, 2):
        raise ValueError("WAV must be mono or stereo")

    samples = raw.astype(np.float64)
    if raw.dtype.kind == "u":
        midpoint = (np.iinfo(raw.dtype).max + 1) / 2
        samples = (samples - midpoint) / midpoint
    elif raw.dtype.kind == "i":
        # SciPy left-aligns packed PCM24 in int32, so scale by the container.
        samples /= -float(np.iinfo(raw.dtype).min)
    elif raw.dtype.kind != "f" or raw.dtype.itemsize != 4:
        raise ValueError("WAV must contain integer PCM or IEEE float32")

    if not np.isfinite(samples).all():
        raise ValueError("WAV contains non-finite audio samples")
    return sample_rate, samples, hashlib.sha256(source_bytes).hexdigest()


def analyze(samples, sample_rate, frame_size, hop_size, *, include_spectrogram=False):
    if not 32 <= frame_size <= 16384 or frame_size & (frame_size - 1):
        raise ValueError("Frame size must be a power of two between 32 and 16384")
    if not 1 <= hop_size <= frame_size // 2:
        raise ValueError("Hop size must be between 1 and half the frame size")

    half_frame = frame_size // 2
    frequencies = np.fft.rfftfreq(frame_size, d=1 / sample_rate)
    window = np.hanning(frame_size + 1)[:-1]
    power_normalization = frame_size * np.dot(window, window)
    windowed = np.zeros((frame_size, samples.shape[1]))
    centers = range(0, len(samples), hop_size)
    spectra = np.empty((len(centers), len(frequencies))) if include_spectrogram else None
    frames = []
    previous_magnitude = None

    for index, center in enumerate(centers):
        window_start = center - half_frame
        start = max(0, window_start)
        end = min(len(samples), center + half_frame)
        segment = samples[start:end]
        window_slice = slice(start - window_start, end - window_start)
        windowed.fill(0.0)
        windowed[window_slice] = segment * window[window_slice, None]

        rms = float(np.sqrt(np.mean(segment**2)))
        silent = rms <= SILENCE_RMS
        fft = np.fft.rfft(windowed, axis=0)
        power = np.mean(np.abs(fft)**2, axis=1) / power_normalization
        power[1:-1] *= 2.0  # Fold negative-frequency power into interior bins.
        if spectra is not None:
            spectra[index] = power

        centroid = flatness = None
        magnitude = np.zeros_like(power)
        total_power = power.sum()
        if not silent and total_power > 0.0:
            centroid = float(np.dot(frequencies, power) / total_power)
            floor = power.max() * FLATNESS_POWER_FLOOR
            geometric_mean = np.exp(np.mean(np.log(np.maximum(power, floor))))
            flatness = float(geometric_mean / power.mean())
            magnitude = np.sqrt(power)

        onset = None
        if previous_magnitude is not None:
            increase = np.maximum(magnitude - previous_magnitude, 0.0)
            onset = float(np.linalg.norm(increase))
        previous_magnitude = magnitude

        frames.append({
            "center_sample": center,
            "start_sample": start,
            "end_sample_exclusive": end,
            "time_seconds": center / sample_rate,
            "rms": rms,
            "onset_strength": onset,
            "centroid_hz": centroid,
            "flatness": flatness,
            "is_silent": silent,
        })
    return frames, spectra


def save_plot(path, sample_map, samples, spectra):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    source = sample_map["source"]
    frames = sample_map["frames"]
    sample_rate = source["sample_rate_hz"]
    duration = len(samples) / sample_rate
    times = np.array([frame["time_seconds"] for frame in frames])
    time_edges = np.concatenate(([0.0], (times[:-1] + times[1:]) / 2, [duration]))
    fft_size = sample_map["settings"]["frame_size_samples"]
    frequencies = np.fft.rfftfreq(fft_size, d=1 / sample_rate)
    frequency_edges = np.concatenate((
        [0.0], (frequencies[:-1] + frequencies[1:]) / 2, [sample_rate / 2]
    ))
    power_db = 10 * np.log10(np.maximum(spectra, 1e-12))

    figure = plt.figure(figsize=(10, 10), layout="constrained")
    grid = figure.add_gridspec(6, 2, width_ratios=(1, 0.025))
    waveform = figure.add_subplot(grid[0, 0])
    axes = [waveform] + [figure.add_subplot(grid[row, 0], sharex=waveform) for row in range(1, 6)]
    for axis in axes[:-1]:
        axis.tick_params(labelbottom=False)
    figure.suptitle(source["filename"], parse_math=False)
    axes[0].plot(np.arange(len(samples)) / sample_rate, samples, linewidth=0.7)
    axes[0].set_ylabel("Waveform (FS)")
    image = axes[1].pcolormesh(
        time_edges, frequency_edges, power_db.T, cmap="magma", vmin=-120, vmax=0
    )
    axes[1].set_yscale("symlog", linthresh=frequencies[1])
    axes[1].set_ylabel("Frequency (Hz)")
    figure.colorbar(
        image, cax=figure.add_subplot(grid[1, 1]), label="Bin power (dBFS)",
        ticks=[-120, -80, -40, 0],
    )

    for axis, (feature, label) in zip(axes[2:], (
        ("rms", "RMS (FS)"),
        ("onset_strength", "Onset strength (FS)"),
        ("centroid_hz", "Centroid (Hz)"),
        ("flatness", "Flatness (0–1)"),
    )):
        values = np.array([frame[feature] for frame in frames], dtype=float)
        axis.plot(times, values, linewidth=1.0, marker=".", markersize=2)
        axis.set_ylabel(label)
        axis.grid(alpha=0.25)
    axes[-1].set(xlabel="Source time (s)", xlim=(0, duration))
    figure.savefig(path, dpi=120)
    plt.close(figure)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wav", type=Path, help="mono or stereo input WAV")
    parser.add_argument("output_dir", type=Path, help="new directory for the analysis")
    parser.add_argument("--frame-size", type=int, default=2048, help="FFT size in samples")
    parser.add_argument("--hop-size", type=int, help="frame spacing in samples (default: frame size / 4)")
    parser.add_argument("--plot", action="store_true", help="also write analysis.png")
    args = parser.parse_args()

    hop = args.frame_size // 4 if args.hop_size is None else args.hop_size
    try:
        if args.output_dir.exists() or args.output_dir.is_symlink():
            raise ValueError(f"Choose a new output directory ({args.output_dir} already exists)")
        sample_rate, samples, digest = load_wav(args.wav)
        frames, spectra = analyze(
            samples, sample_rate, args.frame_size, hop, include_spectrogram=args.plot
        )
        sample_map = {
            "schema_version": 1,
            "source": {
                "filename": args.wav.name,
                "sha256": digest,
                "sample_rate_hz": sample_rate,
                "channels": samples.shape[1],
                "sample_frames": len(samples),
            },
            "settings": {
                "frame_size_samples": args.frame_size,
                "hop_size_samples": hop,
                "window": "periodic_hann",
                "alignment": "centered_zero_padded",
                "channel_policy": "mean_channel_power",
                "silence_rms": SILENCE_RMS,
                "flatness_relative_power_floor": FLATNESS_POWER_FLOOR,
            },
            "feature_units": {
                "rms": "full_scale_amplitude",
                "onset_strength": "full_scale_amplitude",
                "centroid_hz": "hertz",
                "flatness": "ratio",
            },
            "frames": frames,
        }

        # Publish the directory only when both requested outputs have been written.
        args.output_dir.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=args.output_dir.parent) as staging:
            result = Path(staging) / "result"
            result.mkdir()
            (result / "sample-map.json").write_text(
                json.dumps(sample_map, indent=2, allow_nan=False) + "\n", encoding="utf-8"
            )
            if spectra is not None:
                save_plot(result / "analysis.png", sample_map, samples, spectra)
            result.rename(args.output_dir)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    print(f"Wrote {len(frames)} analysis frames to {args.output_dir}")


if __name__ == "__main__":
    main()
