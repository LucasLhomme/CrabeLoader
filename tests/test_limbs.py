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


def test_torso_scale_widens_chest_only():
    anchors = {"Hips": np.array([0, 1.0, 0]), "Spine2": np.array([0, 1.2, 0]), "Spine3": np.array([0, 1.4, 0]),
               "Neck": np.array([0, 1.6, 0])}
    v = np.array([[0.15, 1.3, 0], [0.15, 1.7, 0]])  # chest, beside the head (no torso weight)
    names = ["Spine2", "Head"]
    w = np.array([[1.0, 0.0], [0.0, 1.0]])
    out = limbs.inflate(v, w, names, anchors, torso_scale=1.4)
    assert np.allclose(out[0], [0.21, 1.3, 0]) and np.allclose(out[1], v[1])


def test_head_scale_grows_from_the_bottom_of_the_head():
    anchors = {"Neck": np.array([0, 1.6, 0]), "Head": np.array([0, 1.7, 0])}
    head = np.array([[0, 1.65 + 0.01 * i, 0.05] for i in range(20)])  # chin at 1.65 up to the top at 1.84
    hip = np.array([[0.2, 1.0, 0]])
    v = np.vstack([head, hip])
    names = ["Head", "Hips"]
    w = np.zeros((len(v), 2))
    w[:20, 0] = 1
    w[20, 1] = 1
    out = limbs.inflate(v, w, names, anchors, head_scale=1.2)
    assert out[:20, 1].min() >= 1.648          # the chin stays put (no sinking into the collar)
    assert out[19, 1] > v[19, 1] + 0.02        # the top of the head grows
    assert np.allclose(out[20], v[20])         # the rest of the body does not move


def test_forearm_scale_is_separate():
    v = _arm(0.03)
    names = ["Shoulder_Left", "Elbow_Left"]
    w = np.zeros((len(v), 2))
    w[v[:, 0] < 0.45, 0] = 1
    w[v[:, 0] >= 0.45, 1] = 1
    out = limbs.inflate(v, w, names, ANCHORS, arm_scale=1.0, forearm_scale=2.0)
    r = np.hypot(out[:, 1] - 1.4, out[:, 2])
    assert np.allclose(r[v[:, 0] < 0.45], 0.03) and np.allclose(r[v[:, 0] >= 0.45], 0.06)


def test_v_shape_widens_narrow_chest_not_wide_jacket():
    anchors = {"Hips": np.array([0, 1.0, 0]), "Spine2": np.array([0, 1.15, 0]), "Spine3": np.array([0, 1.3, 0]),
               "Neck": np.array([0, 1.6, 0]), "Head": np.array([0, 1.7, 0]), "HeadTop": np.array([0, 1.9, 0]),
               "Shoulder_Left": np.array([0.3, 1.5, 0]), "Shoulder_Right": np.array([-0.3, 1.5, 0])}
    for side, sx in (("Left", 1), ("Right", -1)):  # arms out sideways, legs down: far from the test points
        anchors.update({f"Elbow_{side}": np.array([sx * 0.6, 1.5, 0]), f"Hand_{side}": np.array([sx * 0.9, 1.5, 0]),
                        f"HandTip_{side}": np.array([sx * 1.0, 1.5, 0]), f"Clavicle_{side}": np.array([sx * 0.03, 1.5, 0]),
                        f"Thigh_{side}": np.array([sx * 0.1, 0.95, 0]), f"Knee_{side}": np.array([sx * 0.1, 0.5, 0]),
                        f"Ankle_{side}": np.array([sx * 0.1, 0.1, 0]), f"ToeTip_{side}": np.array([sx * 0.1, 0.0, 0.2])})
    ys = np.linspace(1.0, 1.6, 25)
    narrow = np.array([[sx * 0.1, y, 0] for y in ys for sx in (-1, 1)])
    names = ["Spine3"]
    w = np.ones((len(narrow), 1))
    out = limbs.inflate(narrow, w, names, anchors, v_shape=1.0, v_target=0.2)
    chest = np.abs(narrow[:, 1] - 1.4) < 0.02
    waist = np.abs(narrow[:, 1] - 1.05) < 0.03
    assert np.all(np.abs(out[chest, 0]) > 0.17)          # chest pushed out towards the target
    assert np.allclose(out[waist, 0], narrow[waist, 0])  # waist untouched
    wide = narrow.copy()
    wide[:, 0] *= 3  # already wider than the target: left alone
    assert np.allclose(limbs.inflate(wide, w, names, anchors, v_shape=1.0, v_target=0.2), wide)
