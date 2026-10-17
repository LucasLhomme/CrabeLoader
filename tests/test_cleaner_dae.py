import glob
from pathlib import Path

import numpy as np
import pytest

from src.core.cleaner import dae_to_glb
from src.core.model import extract_archive, load_source


def test_dae_to_glb_keeps_shape_and_colours(tmp_path):
    zips = glob.glob('/tmp/batch/*Sonia*.zip')
    if not zips:
        pytest.skip("sample .dae not available")
    _, ranked = extract_archive(Path(zips[0]))
    dae = next(f for f in ranked if f.suffix == ".dae")
    a, b = load_source(dae), load_source(dae_to_glb(dae, tmp_path / "x.glb"))
    assert len(a.vertices) == len(b.vertices) and np.abs(a.vertices - b.vertices).max() < 1e-5
    ta, tb = (np.asarray(s.texture.convert("RGB"), float).mean() for s in (a, b))
    assert abs(ta - tb) < 1
