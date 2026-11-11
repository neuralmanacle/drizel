import hashlib
from pathlib import Path
import tempfile
import unittest
import wave

import numpy as np
from scipy.io import wavfile

from analyze import analyze, load_wav
from fixture import IMPULSE_SAMPLE, SAMPLE_RATE, fixture_samples


class KnownSignalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        samples = fixture_samples()[:, None] / 32768.0
        cls.frames, cls.spectra = analyze(samples, SAMPLE_RATE, 2048, 512, include_spectrogram=True)

    def frame_at(self, time):
        return min(self.frames, key=lambda frame: abs(frame["time_seconds"] - time))

    def test_tone_level_and_centroid(self):
        tone = self.frame_at(0.75)
        self.assertAlmostEqual(tone["rms"], 0.5 / np.sqrt(2), delta=1e-4)
        self.assertAlmostEqual(tone["centroid_hz"], 750.0, delta=0.1)
        self.assertLess(tone["flatness"], 0.01)
        self.assertLess(tone["onset_strength"], 1e-4)
        power = self.spectra[tone["center_sample"] // 512].sum()
        self.assertAlmostEqual(power, 0.125, delta=1e-4)

    def test_impulse_features_map_to_the_known_sample(self):
        impulse = self.frame_at(0.25)
        self.assertAlmostEqual(impulse["rms"], 0.8 / np.sqrt(2048), delta=1e-6)
        self.assertAlmostEqual(impulse["centroid_hz"], SAMPLE_RATE / 4, delta=0.01)
        self.assertGreater(impulse["flatness"], 0.99)
        neighborhood = [frame for frame in self.frames if 0.15 < frame["time_seconds"] < 0.35]
        peak = max(neighborhood, key=lambda frame: frame["onset_strength"])
        self.assertGreater(peak["onset_strength"], 0.01)
        self.assertLessEqual(peak["start_sample"], IMPULSE_SAMPLE)
        self.assertLess(IMPULSE_SAMPLE, peak["end_sample_exclusive"])

    def test_silence_and_noise(self):
        quiet = self.frame_at(0.125)
        self.assertTrue(quiet["is_silent"])
        self.assertEqual(quiet["rms"], 0.0)
        self.assertEqual(quiet["onset_strength"], 0.0)
        self.assertIsNone(quiet["centroid_hz"])
        self.assertIsNone(quiet["flatness"])
        self.assertIsNone(self.frames[0]["onset_strength"])

        noise = self.frame_at(1.5)
        self.assertFalse(noise["is_silent"])
        self.assertAlmostEqual(noise["rms"], 0.1, delta=0.01)
        self.assertGreater(noise["flatness"], 0.45)

    def test_stereo_opposite_polarity_preserves_power(self):
        mono = np.zeros((512, 1))
        mono[128, 0] = 0.5
        stereo = np.concatenate((mono, -mono), axis=1)
        mono_frames, mono_power = analyze(mono, SAMPLE_RATE, 128, 32, include_spectrogram=True)
        stereo_frames, stereo_power = analyze(stereo, SAMPLE_RATE, 128, 32, include_spectrogram=True)
        np.testing.assert_allclose(stereo_power, mono_power, rtol=1e-12, atol=1e-14)
        np.testing.assert_allclose(
            [frame["rms"] for frame in stereo_frames], [frame["rms"] for frame in mono_frames]
        )
        self.assertTrue(any(not frame["is_silent"] for frame in stereo_frames))

    def test_short_and_partial_frames_keep_exact_source_intervals(self):
        for length in (1, 15, 16, 17, 31, 32, 33):
            with self.subTest(length=length):
                samples = np.full((length, 1), 0.25)
                frames, spectra = analyze(samples, 8000, 32, 16)
                self.assertIsNone(spectra)
                self.assertEqual([frame["center_sample"] for frame in frames], list(range(0, length, 16)))
                self.assertEqual(frames[0]["start_sample"], 0)
                self.assertEqual(frames[-1]["end_sample_exclusive"], length)
                for frame in frames:
                    center = frame["center_sample"]
                    self.assertEqual(frame["start_sample"], max(0, center - 16))
                    self.assertEqual(frame["end_sample_exclusive"], min(length, center + 16))
                    self.assertEqual(frame["time_seconds"], center / 8000)
                    self.assertEqual(frame["rms"], 0.25)

    def test_frame_settings_are_bounded(self):
        for frame, hop in ((0, 1), (48, 12), (32768, 512), (64, 0), (64, 33)):
            with self.subTest(frame=frame, hop=hop), self.assertRaises(ValueError):
                analyze(np.ones((1, 1)), 48000, frame, hop)


class WavTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.path = Path(directory.name) / "audio.wav"

    def test_pcm16_and_hash(self):
        wavfile.write(self.path, 44100, np.array([-32768, 0, 16384], dtype=np.int16))
        rate, samples, digest = load_wav(self.path)
        self.assertEqual(rate, 44100)
        np.testing.assert_array_equal(samples[:, 0], [-1.0, 0.0, 0.5])
        self.assertEqual(digest, hashlib.sha256(self.path.read_bytes()).hexdigest())

    def test_packed_pcm24_and_float32_stereo(self):
        with wave.open(str(self.path), "wb") as output:
            output.setparams((1, 3, 48000, 0, "NONE", "not compressed"))
            output.writeframes(b"".join(
                value.to_bytes(3, "little", signed=True) for value in (-8388608, 0, 4194304, 8388607)
            ))
        _, samples, _ = load_wav(self.path)
        np.testing.assert_array_equal(samples[:, 0], [-1.0, 0.0, 0.5, 1 - 2**-23])

        floating = np.array([[-0.125, 1.25], [0.5, -0.75]], dtype=np.float32)
        wavfile.write(self.path, 48000, floating)
        _, samples, _ = load_wav(self.path)
        np.testing.assert_array_equal(samples, floating)

    def test_reject_invalid_wav_data(self):
        for data in (np.array([], dtype=np.int16), np.zeros((32, 3), dtype=np.int16),
                     np.array([np.nan], dtype=np.float32)):
            with self.subTest(shape=data.shape, dtype=data.dtype):
                wavfile.write(self.path, 48000, data)
                with self.assertRaises(ValueError):
                    load_wav(self.path)

        wavfile.write(self.path, 48000, np.zeros(32, dtype=np.int16))
        self.path.write_bytes(self.path.read_bytes()[:-2])
        with self.assertRaisesRegex(ValueError, "Invalid WAV data"):
            load_wav(self.path)


if __name__ == "__main__":
    unittest.main()
