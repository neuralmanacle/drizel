import importlib.util
import unittest
from pathlib import Path

import numpy as np

spec = importlib.util.spec_from_file_location("step12", Path(__file__).with_name("train.py"))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class RegionMLTests(unittest.TestCase):
    def test_unannotated_is_not_silence(self):
        recording = {"regions": [], "attack_sample": None}
        self.assertIsNone(module.label_for(recording, {"center_sample": 100}, 48000))

    def test_attack_precedence(self):
        recording = {"attack_sample": 100, "regions": [
            {"label": "Guitar E2 decay", "start_sample": 0, "end_sample_exclusive": 9000}]}
        self.assertEqual(module.label_for(recording, {"center_sample": 101}, 48000), "attack")

    def test_scaler_training_only(self):
        train = np.array([[0., 1.], [2., 3.]])
        test = np.array([[100., 100.]])
        _, transformed, mean, _ = module.standardize(train, test)
        np.testing.assert_array_equal(mean, [1., 2.])
        self.assertGreater(transformed[0, 0], 50)

    def test_models_predict_finite(self):
        x = np.array([[0., 0.], [1., 1.], [2., 2.], [3., 3.]])
        y = np.array([0, 1, 2, 3])
        for trainer in (module.train_linear, module.train_mlp):
            predict, _ = trainer(x, y, epochs=5)
            self.assertTrue(np.isfinite(predict(x)).all())

    def test_ambiguous_regions_abstain(self):
        for label in ("Quiet claves tail", "Claves decay", "Unknown mixed texture"):
            recording = {"regions": [{"label": label, "start_sample": 0,
                                      "end_sample_exclusive": 1000}]}
            self.assertIsNone(module.label_for(recording, {"center_sample": 100}, 48000))

    def test_balanced_weights(self):
        y = np.array([0, 1, 1, 1])
        weights = module.class_weights(y)
        self.assertAlmostEqual(weights[0], 2.0)
        self.assertAlmostEqual(weights[1], 2.0 / 3.0)
        self.assertEqual(weights[2], 0)

    def test_temporal_features_and_zcr(self):
        frames = [
            {"rms": 0.1, "onset_strength": None, "centroid_hz": 100, "flatness": .2},
            {"rms": 0.2, "onset_strength": .4, "centroid_hz": 150, "flatness": .3},
        ]
        x = module.feature_vectors(frames, [0.1, 0.2])
        self.assertEqual(len(x[0]), len(module.FEATURES))
        self.assertAlmostEqual(x[0][4], 0)
        self.assertAlmostEqual(x[1][4], .1)
        self.assertAlmostEqual(x[1][5], .4)
        self.assertAlmostEqual(x[1][-1], .2)

    def test_pooled_metrics(self):
        matrix = np.zeros((4, 4), dtype=int)
        matrix[1, 1] = 2
        matrix[2, 1] = 1
        report = module.metrics_from_matrix(matrix)
        self.assertAlmostEqual(report["accuracy"], 2 / 3)
        self.assertEqual(report["per_class"]["noise"]["recall"], 0)

    def test_metrics_missing_class_explicit(self):
        result = module.metrics(np.array([1, 1]), np.array([1, 0]))
        self.assertEqual(result["per_class"]["attack"]["support"], 0)
        self.assertEqual(result["per_class"]["tonal"]["recall"], .5)


if __name__ == "__main__":
    unittest.main()
