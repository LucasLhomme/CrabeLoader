import numpy as np

from src.core import rigging


def test_shoe_top_follows_the_hem_then_turns_rigid():
    verts = np.array([[0.2, 2.0, 0], [0.2, 0.30, 0], [0.2, 0.28, 0], [0.2, 0.10, 0]], float)
    weights = np.zeros((4, 2))
    weights[:2, 0] = 1   # hem: bone 0
    weights[2:, 1] = 1   # shoe: bone 1 (rigid)
    rigid = np.array([0, 0, 1, 1], bool)
    side = np.array(["Left"] * 4)
    assert rigging._blend_piece_seams(verts, weights, rigid, side) == 1
    assert weights[2, 0] > 0.5            # next to the hem: mostly the hem's weights
    assert weights[3].tolist() == [0, 1]  # sole: still rigid on the ankle
