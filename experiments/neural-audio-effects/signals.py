"""Deterministic, normalized test signal generation."""
import numpy as np

KINDS = ('sine', 'harmonic', 'impulse', 'noise', 'silence')


def generate(kind, rate=48000, duration=1.0, channels=1, seed=14):
    if kind not in KINDS or not isinstance(rate, int) or rate <= 0 or not np.isfinite(duration) or duration <= 0 or channels not in (1, 2):
        raise ValueError('Invalid signal configuration')
    frames = round(rate * duration)
    if frames < 1 or frames > rate * 600:
        raise ValueError('Signal must contain 1 frame to 10 minutes')
    t = np.arange(frames) / rate
    if kind == 'sine':
        mono = 0.5 * np.sin(2 * np.pi * 440 * t)
    elif kind == 'harmonic':
        mono = sum((0.5 / k) * np.sin(2 * np.pi * 110 * k * t) for k in range(1, 6))
    elif kind == 'impulse':
        mono = np.zeros(frames)
        mono[0] = 0.8
    elif kind == 'noise':
        mono = np.random.default_rng(seed).uniform(-0.5, 0.5, frames)
    else:
        mono = np.zeros(frames)
    return np.repeat(mono[:, None], channels, axis=1)
