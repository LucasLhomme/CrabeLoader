import unittest

import numpy as np

from src.core import ranking as R


def body(width: float, thickness: float, n: int = 4000):
    rng = np.random.default_rng(1)
    y = rng.random(n) * 2.0
    return np.c_[(rng.random(n) - 0.5) * 2 * width, y, (rng.random(n) - 0.5) * 2 * thickness]


class TestRanking(unittest.TestCase):
    def test_profile_is_scale_free_and_sees_width(self):
        slim, wide = R.silhouette_profile(body(0.2, 0.1)), R.silhouette_profile(body(0.4, 0.1))
        self.assertAlmostEqual(R.shape_distance(slim, R.silhouette_profile(body(0.2, 0.1) * 3.0)), 0.0, places=1)
        self.assertGreater(R.shape_distance(slim, wide), 0.3)

    def test_shortlist_collapses_clones(self):
        a = np.zeros(4)
        scored = [(0.0, "a", a), (0.01, "a_clone", a + 0.01), (0.5, "b", a + 1.0), (0.6, "c", a + 2.0)]
        picks = [p[1] for p in R.diverse_shortlist(scored, 3)]
        self.assertEqual(picks, ["a", "b", "c"])

    def test_score_rewards_match_and_low_bake_distortion(self):
        self.assertGreater(R.final_score(90, 95, 0.01), R.final_score(90, 95, 0.10))
        self.assertGreater(R.final_score(90, 95, 0.03), R.final_score(40, 95, 0.03))
        self.assertEqual(R.shape_match(0.0), 100)


if __name__ == "__main__":
    unittest.main()
