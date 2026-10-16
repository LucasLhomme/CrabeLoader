import numpy as np

from src.core import limbs


def _arm(radius):
    t = np.linspace(0.1, 0.9, 40)
    ang = np.linspace(0, 2 * np.pi, 12, endpoint=False)
    pts = [[0.2 + 0.5 * x, 1.4 + radius * np.cos(a), radius * np.sin(a)] for x in t for a in ang]
    return np.array(pts)


ANCHORS = {"Shoulder_Left": np.array([0.2, 1.4, 0]), "Elbow_Left": np.array([0.45, 1.4, 0]),
           "Hand_Left": np.array([0.7, 1.4, 0])}


def test_inflate_scales_radius_not_length():
    v = _arm(0.03)
    names = ["Shoulder_Left", "Elbow_Left"]
    w = np.zeros((len(v), 2))
    w[v[:, 0] < 0.45, 0] = 1
    w[v[:, 0] >= 0.45, 1] = 1
    out = limbs.inflate(v, w, names, ANCHORS, arm_scale=1.5)
    r = np.hypot(out[:, 1] - 1.4, out[:, 2])
    assert np.allclose(r, 0.045, atol=1e-6)
    assert np.allclose(out[:, 0], v[:, 0])


def test_radius_and_suggest():
    assert abs(limbs.limb_radius(_arm(0.03), ANCHORS["Shoulder_Left"], ANCHORS["Elbow_Left"]) - 0.03) < 1e-6
