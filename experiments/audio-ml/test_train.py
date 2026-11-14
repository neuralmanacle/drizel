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

    def test_metrics_missing_class_explicit(self):
        result = module.metrics(np.array([1, 1]), np.array([1, 0]))
        self.assertEqual(result["per_class"]["attack"]["support"], 0)
        self.assertEqual(result["per_class"]["tonal"]["recall"], .5)


if __name__ == "__main__":
    unittest.main()
