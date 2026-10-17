import unittest

import numpy as np

from src.core import weight_transfer as wt


class TestWeightTransfer(unittest.TestCase):
    def test_available(self):
        self.assertTrue(wt.availability()[0])

    def test_inpaint_interpolates_between_known_ends(self):
        line = np.linspace(0, 1, 21)
        points = np.c_[line, np.zeros(21), np.zeros(21)]
        weights = np.zeros((21, 2))
        weights[0] = (1, 0)
        weights[-1] = (0, 1)
        known = np.zeros(21, bool)
        known[[0, 1, 19, 20]] = True
        weights[1], weights[19] = (1, 0), (0, 1)
        out = wt.inpaint(points, weights, known)
        self.assertIsNotNone(out)
        self.assertTrue(np.all(np.diff(out[:, 1]) >= -1e-6))  # monotone blend from bone 0 to bone 1
        self.assertAlmostEqual(float(out[10].sum()), 1.0, places=3)

    def test_loose_part_gets_weights_from_neighbours(self):
        rng = np.random.default_rng(0)
        body = rng.random((200, 3)) * [1, 2, 1]
        loose = rng.random((20, 3)) * 0.2 + [1.3, 1.0, 0.4]  # separate piece next to the body
        points = np.vstack([body, loose])
        weights = np.zeros((220, 2))
        weights[:200, 0] = 1
        known = np.r_[np.ones(200, bool), np.zeros(20, bool)]
        out = wt.inpaint(points, weights, known)
        self.assertIsNotNone(out)
        self.assertGreater(float(out[200:, 0].min()), 0.5)


if __name__ == "__main__":
    unittest.main()
