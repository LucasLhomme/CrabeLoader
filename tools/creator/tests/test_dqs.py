import numpy as np

from src.core import dqs


def _rot_z(deg):
    a = np.radians(deg)
    c, s = np.cos(a), np.sin(a)
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]])  # column convention


def test_single_bone_matches_the_matrix():
    m = _rot_z(70).T  # row convention
    t = np.array([0.1, -0.2, 0.3])
    p = np.random.default_rng(0).normal(size=(20, 3))
    out, n = dqs.blend(p, p, np.ones((20, 1)), [(m, t)])
    assert np.allclose(out, p @ m + t, atol=1e-9)
    assert np.allclose(n, p @ m, atol=1e-9)


def test_half_blend_keeps_distance_from_pivot():
    # A point at distance 1 from the pivot, half on a bone rotated 90 degrees: linear blend pulls it to ~0.71,
    # dual quaternions keep it at 1 (no collapse at the joint).
    m = _rot_z(90).T
    p = np.array([[1.0, 0, 0]])
    out, _ = dqs.blend(p, p, np.array([[0.5, 0.5]]), [(np.eye(3), np.zeros(3)), (m, np.zeros(3))])
    assert abs(np.linalg.norm(out[0]) - 1.0) < 1e-9
    linear = 0.5 * p + 0.5 * (p @ m)
    assert np.linalg.norm(linear[0]) < 0.75
