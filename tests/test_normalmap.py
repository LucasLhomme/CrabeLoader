import json
import unittest
from pathlib import Path

import numpy as np
from PIL import Image

from src.core import normalmap

ASSETS = Path(__file__).resolve().parents[1] / "src" / "assets" / "bases" / "index.json"


class TestNormalMap(unittest.TestCase):
    def test_flat_texture_gives_a_flat_map_in_the_game_layout(self):
        n = np.asarray(normalmap.normal_map(Image.new("RGB", (128, 128), (120, 60, 30))))
        self.assertTrue((n[..., 0] == 255).all())
        self.assertTrue(np.abs(n[..., 1].astype(int) - 128).max() <= 1)
        self.assertTrue(np.abs(n[..., 3].astype(int) - 128).max() <= 1)

    def test_a_dark_line_becomes_a_groove(self):
        img = Image.new("RGB", (256, 256), (180, 180, 180))
        arr = np.asarray(img).copy()
        arr[:, 126:130] = 40  # vertical dark seam
        n = normalmap.decode(normalmap.normal_map(Image.fromarray(arr)))
        self.assertGreater(n[128, 125, 0], 0.3, "left wall of the groove faces right (+X)")
        self.assertLess(n[128, 130, 0], -0.3, "right wall faces left")
        self.assertLess(abs(n[128, 60, 0]), 0.02, "flat away from the seam")

    def test_dark_band_on_top_tilts_the_wall_below_upwards(self):
        arr = np.full((256, 256, 3), 180, np.uint8)
        arr[126:130] = 40  # horizontal dark seam
        n = normalmap.decode(normalmap.normal_map(Image.fromarray(arr)))
        self.assertGreater(n[130, 128, 1], 0.3, "the wall below the groove faces up the image (+Y = G)")

    @unittest.skipUnless(ASSETS.exists(), "needs the base pack (src/assets)")
    def test_tangent_handedness_matches_the_shipped_figures(self):
        from src.core.base_character import S1_STRIDE, BaseCharacter
        from src.core.rig import vertex_tangents
        from src.core.roster import find_entry
        b = BaseCharacter.from_pack(find_entry("TCW_Anakin"))
        prim, n = b.body_primitive, b.body_primitive.vertex_count
        v, t, uv = b.primitive_geometry(prim)
        s1 = np.frombuffer(b._vbuf[prim.s1_offset:prim.s1_offset + n * S1_STRIDE], np.uint8).reshape(n, S1_STRIDE)
        nrm = s1[:, 8:14].copy().view("<f2").astype(float)
        stored = np.sign(s1[:, 16:24].copy().view("<f2").astype(float)[:, 3])
        _, w = vertex_tangents(v, uv, nrm / np.linalg.norm(nrm, axis=1, keepdims=True), t)
        self.assertGreater((w == stored).mean(), 0.95)


if __name__ == "__main__":
    unittest.main()
