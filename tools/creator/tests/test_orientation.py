import unittest

import numpy as np
from PIL import Image

from src.core.model import SourceModel, Placement, auto_placement, place, rank_model_files


def figure(n: int = 6000) -> np.ndarray:
    """A crude T-pose figure, Y-up, facing +Z: legs with feet pointing forward, torso, arms and a head."""
    rng = np.random.default_rng(0)

    def box(lo, hi, k):
        return lo + rng.random((k, 3)) * (np.array(hi) - np.array(lo))
    parts = [box([-0.25, 0, -0.08], [-0.08, 0.9, 0.08], n // 6), box([0.08, 0, -0.08], [0.25, 0.9, 0.08], n // 6),
             box([-0.25, 0, 0.08], [-0.08, 0.06, 0.25], n // 20), box([0.08, 0, 0.08], [0.25, 0.06, 0.25], n // 20),
             box([-0.28, 0.9, -0.12], [0.28, 1.45, 0.12], n // 5), box([-0.95, 1.33, -0.05], [0.95, 1.43, 0.05], n // 6),
             box([-0.12, 1.5, -0.12], [0.12, 1.8, 0.12], n // 8)]
    return np.vstack(parts)


def source(v):
    return SourceModel("t", v, np.zeros((0, 3), int), np.zeros((len(v), 2)), Image.new("RGB", (4, 4)), 1.0)


class TestOrientation(unittest.TestCase):
    def check(self, v):
        p = auto_placement(source(v), 2.0)
        placed = place(v, p)
        h = placed[:, 1].max()
        head = placed[placed[:, 1] > 0.9 * h]
        self.assertLess(np.ptp(head[:, 0]), 0.4 * h, "the head must be on top")
        self.assertGreater(np.ptp(placed[:, 0]), np.ptp(placed[:, 2]), "the arm span must face the camera")

    def test_native(self):
        self.check(figure())

    def test_z_up_and_upside_down(self):
        v = figure()
        self.check(v[:, [0, 2, 1]] * [1, -1, 1])  # Z-up
        self.check(v[:, [0, 2, 1]] * [1, 1, -1])  # Z-down (stands on its head once stood up)

    def test_lying_with_arms_up(self):
        v = figure()
        self.check(v[:, [1, 0, 2]])  # on its side: the arm span becomes vertical

    def test_ranking_prefers_main_model(self):
        import tempfile
        from pathlib import Path
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "sub").mkdir()
            files = [root / "Zelda_Phantom.dae", root / "Zelda.dae", root / "sub" / "Zelda_eyes.dae"]
            for f in files:
                f.write_bytes(b"x" * 100)
            self.assertEqual(rank_model_files(files, root, "Zelda")[0].name, "Zelda.dae")


if __name__ == "__main__":
    unittest.main()
