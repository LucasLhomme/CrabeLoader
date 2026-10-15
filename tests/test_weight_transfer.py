import tempfile
import unittest
from pathlib import Path

import numpy as np

from src.core import weight_transfer as wt


class TestWeightTransfer(unittest.TestCase):
    def test_addon_not_found(self):
        with tempfile.TemporaryDirectory() as d:
            self.assertIsNone(wt.find_addon(d))
            ok, message = wt.availability(d)
            self.assertFalse(ok)
            self.assertIn("weighttransfer.py", message)

    def test_addon_found_when_file_present(self):
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / "weighttransfer.py").write_text("")
            self.assertEqual(wt.find_addon(d), Path(d))

    def test_barycentric_matches_vertices(self):
        a, b, c = np.array([[0.0, 0, 0]]), np.array([[1.0, 0, 0]]), np.array([[0.0, 1, 0]])
        np.testing.assert_allclose(wt._barycentric_coordinates_tri(b, a, b, c), [[0, 1, 0]], atol=1e-9)
        np.testing.assert_allclose(wt._barycentric_coordinates_tri((a + b + c) / 3, a, b, c), [[1 / 3] * 3], atol=1e-9)


if __name__ == "__main__":
    unittest.main()
