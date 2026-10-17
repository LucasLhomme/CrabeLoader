import numpy as np

from src.core.accessories import accessory_weights, back_zone, strip


def _body_with_cape():
    rng = np.random.default_rng(1)
    y = rng.uniform(0, 2, 4000)
    body = np.c_[rng.uniform(-0.15, 0.15, 4000), y, rng.uniform(-0.08, 0.08, 4000)]
    cape = np.c_[rng.uniform(-0.2, 0.2, 400), rng.uniform(0.3, 1.2, 400), rng.uniform(-0.7, -0.4, 400)]
    return np.vstack([body, cape])


def test_back_zone_finds_the_cape_not_the_body():
    v = _body_with_cape()
    zone = back_zone(v)
    assert zone[4000:].mean() > 0.95 and zone[:4000].mean() < 0.01


def test_strip_drops_vertices_and_their_faces():
    v = np.arange(12, dtype=float).reshape(4, 3)
    f = np.array([[0, 1, 2], [1, 2, 3]])
    nv, nf = strip(v, f, np.array([False, False, False, True]))
    assert len(nv) == 3 and nf.tolist() == [[0, 1, 2]]


def test_accessory_follows_torso_not_legs():
    names = ["Hips", "Spine3", "Thigh_Left", "Knee_Left"]
    verts = np.array([[0, 0, 0], [0.01, 0, 0], [0, 1, 0.0]], float)
    weights = np.array([[0, 0, 1.0, 0], [0, 0, 0.5, 0.5], [0, 1.0, 0, 0]])
    out = accessory_weights(verts, weights, np.array([False, True, False]), names)
    assert out[1, 0] == 1.0 and np.allclose(out.sum(1), 1)
