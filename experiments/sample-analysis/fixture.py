"""Generate a 2-second WAV with known silence, impulse, tone and noise regions."""

import argparse
from pathlib import Path

import numpy as np
from scipy.io import wavfile


SAMPLE_RATE = 48000
IMPULSE_SAMPLE = SAMPLE_RATE // 4


def fixture_samples():
    signal = np.zeros(2 * SAMPLE_RATE)
    signal[IMPULSE_SAMPLE] = 0.8
    tone_time = np.arange(SAMPLE_RATE // 2) / SAMPLE_RATE
    signal[SAMPLE_RATE // 2:SAMPLE_RATE] = 0.5 * np.sin(2 * np.pi * 750 * tone_time)
    noise = np.random.default_rng(0).normal(0.0, 0.1, SAMPLE_RATE // 2)
    signal[5 * SAMPLE_RATE // 4:7 * SAMPLE_RATE // 4] = noise
    return np.rint(signal * 32768).astype(np.int16)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wav", type=Path, help="path for the generated WAV")
    args = parser.parse_args()
    try:
        args.wav.parent.mkdir(parents=True, exist_ok=True)
        with args.wav.open("xb") as output:
            wavfile.write(output, SAMPLE_RATE, fixture_samples())
    except OSError as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
