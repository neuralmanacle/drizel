import unittest

import numpy as np

from study import CORPUS, pitch_candidate, study_corpus


class PitchStudyTests(unittest.TestCase):
    def test_low_fundamental_with_a_stronger_harmonic_in_stereo(self):
        rate = 48000
        phase = 2 * np.pi * 82.406889 * np.arange(3840) / rate
        mono = 0.12 * np.sin(phase) + 0.5 * np.sin(2 * phase)
        hz, score = pitch_candidate(np.column_stack((mono, -mono)), rate)
        self.assertAlmostEqual(hz, 82.406889, delta=0.2)
        self.assertGreater(score, 0.95)

    def test_pitch_gate_uses_ac_rms(self):
        phase = 2 * np.pi * 220 * np.arange(3840) / 48000
        # A DC offset cannot make a very quiet periodic signal eligible.
        quiet = (0.1 + 0.0001 * np.sin(phase))[:, None]
        self.assertEqual(pitch_candidate(quiet, 48000), (None, None))


class CorpusStudyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = study_corpus(CORPUS)
        cls.regions = {region["label"]: region for recording in cls.report["recordings"]
                       for region in recording["regions"]}

    def test_known_fundamentals_and_guitar_attack(self):
        for label in ("Guitar E2 decay", "Tenor sax C4 sustain", "750 Hz sine"):
            with self.subTest(label=label):
                region = self.regions[label]
                result = region["threshold_sweep"][-1]
                self.assertGreater(region["pitch_frames"], 0)
                self.assertEqual(result["within_tolerance"], region["pitch_frames"])
                self.assertLess(result["median_absolute_error_cents"], 5)
        guitar = next(recording for recording in self.report["recordings"]
                      if recording["file"] == "guitar-e2.wav")
        self.assertTrue(guitar["attack"]["window_contains_annotation"])

    def test_chord_noise_and_quiet_controls(self):
        chord = self.regions["Guitar chord"]
        self.assertGreater(chord["threshold_sweep"][0]["accepted"], 0)
        self.assertEqual(chord["threshold_sweep"][1]["accepted"], 0)
        for label in ("Rain field recording", "Seeded white noise", "Quiet claves tail", "Digital silence"):
            with self.subTest(label=label):
                self.assertEqual(self.regions[label]["threshold_sweep"][0]["accepted"], 0)
        quiet = self.regions["Quiet claves tail"]
        self.assertGreater(quiet["feature_summary"]["silent_frames"], 0)
        self.assertLess(quiet["feature_summary"]["silent_frames"], quiet["feature_frames"])
        self.assertIsNone(self.regions["Digital silence"]["feature_summary"]["centroid_hz"])
