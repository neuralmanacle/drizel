"""Offline normalized tanh saturation; arrays have shape (frames, channels)."""
import numpy as np


def saturate(audio, drive=2.0, mix=1.0, output_gain=1.0):
    for name, value in (("drive", drive), ("mix", mix), ("output_gain", output_gain)):
        if not np.isfinite(value):
            raise ValueError(f"{name} must be finite")
    if drive < 0 or not 0 <= mix <= 1 or output_gain < 0:
        raise ValueError("drive and output_gain must be nonnegative; mix must be in [0,1]")
    x = np.asarray(audio, dtype=np.float64)
    if x.ndim != 2 or x.shape[1] not in (1, 2) or not x.size or not np.all(np.isfinite(x)):
        raise ValueError("audio must be finite, nonempty mono/stereo frames x channels")
    if np.max(np.abs(x)) > 1.0:
        raise ValueError("input samples must be normalized to [-1,1]")
    wet = x if drive == 0 else np.tanh(drive * x) / np.tanh(drive)
    return ((1 - mix) * x + mix * wet) * output_gain
