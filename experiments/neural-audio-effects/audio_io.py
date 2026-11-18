"""Lossless floating-point WAV I/O; rejects unsupported channels and invalid samples."""
from pathlib import Path
import numpy as np
import soundfile as sf


def read_wav(path):
    if Path(path).suffix.lower() != '.wav':
        raise ValueError('Only WAV input is supported')
    audio, rate = sf.read(path, dtype='float64', always_2d=True)
    if rate <= 0 or audio.shape[1] not in (1, 2) or not audio.size:
        raise ValueError('Expected nonempty mono/stereo WAV')
    if not np.all(np.isfinite(audio)) or np.max(np.abs(audio)) > 1:
        raise ValueError('Input WAV must contain finite normalized samples')
    return audio, rate


def write_wav(path, audio, rate):
    if Path(path).suffix.lower() != '.wav' or rate <= 0:
        raise ValueError('Expected WAV output path and positive sample rate')
    if audio.ndim != 2 or audio.shape[1] not in (1, 2) or not audio.size or not np.all(np.isfinite(audio)):
        raise ValueError('Invalid audio')
    if np.max(np.abs(audio)) > 1:
        raise ValueError('Output exceeds [-1,1]; reduce output gain')
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    sf.write(path, audio, rate, subtype='FLOAT')
