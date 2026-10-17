import unittest

import numpy as np

from src.core import twist


class FakeBase:
    bone_names = ("Hips", "Shoulder_Left", "Shoulder_Spin_Left", "Elbow_Left", "Forearm_OL_Left", "Hand_Left")
    joints = {"Shoulder_Left": np.array([0.2, 1.4, 0.0]), "Elbow_Left": np.array([0.5, 1.4, 0.0]),
              "Hand_Left": np.array([0.8, 1.4, 0.0])}


class TestTwist(unittest.TestCase):
    def setUp(self):
        x = np.linspace(0.2, 0.8, 31)
        self.verts = np.c_[x, np.full_like(x, 1.4), np.zeros_like(x)]
        upper = x < 0.5
        self.merged = np.c_[upper.astype(float), (~upper).astype(float)]  # columns: Shoulder, Elbow
        self.cols = np.array([1, 3])

    def test_weight_is_conserved_and_spread_like_the_game(self):
        merged, cols = twist.split(self.verts, self.merged, self.cols, FakeBase())
        self.assertTrue(np.allclose(merged.sum(1), 1.0))
        spin = merged[:, list(cols).index(2)]
        forearm = merged[:, list(cols).index(4)]
        self.assertGreater(spin[0], 0.5, "the shoulder cap follows Shoulder_Spin")
        self.assertLess(spin[14], 0.1, "near the elbow the upper arm is on Shoulder")
        self.assertGreater(forearm[-1], 0.8, "the wrist end of the forearm is on Forearm_OL")

    def test_missing_helpers_change_nothing(self):
        class Bare(FakeBase):
            bone_names = ("Hips", "Shoulder_Left", "Elbow_Left", "Hand_Left")
        merged, cols = twist.split(self.verts, self.merged, np.array([1, 2]), Bare())
        self.assertTrue(np.allclose(merged, self.merged))


if __name__ == "__main__":
    unittest.main()
