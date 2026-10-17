"""Developer script: learn where the game puts its joints, to calibrate the skeleton auto-fit (skeleton.CALIBRATION).

Every humanoid figure of the base pack whose body is one piece is raised into a T-pose with its own skeleton
(tools/roundtrip.py), measured by `skeleton.measure` with the calibration switched off, and compared with its real
joints. For each joint a small linear model (raw measurements -> real position, fractions of the height) is fitted
and scored by leave-one-out; the coefficients printed at the end are the ones to copy into CALIBRATION.

Usage: python tools/fit_calibration.py
"""
import json
import sys
import warnings
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
warnings.filterwarnings("ignore")
from PIL import Image  # noqa: E402

import src.core.skeleton as skeleton  # noqa: E402
from src.core.base_character import BaseCharacter  # noqa: E402
from src.core.base_pack import INDEX_PATH  # noqa: E402
from src.core.model import CharacterModel, Placement, SourceModel  # noqa: E402
from src.core.roster import find_entry  # noqa: E402
from tools.roundtrip import ARM_JOINTS, official_skin, t_pose  # noqa: E402

# Shapes too far from a human to teach anything about one (ball, round-bodied, short-legged cartoon figures).
NOT_HUMAN = {"LAS_Stitch", "OUT_Disgust", "OUT_Fear", "OUT_Joy", "OUT_Sadness", "OUT_Anger", "TBX_Minnie",
             "TB_DonaldDuck", "DNO_Spot", "MU_Mike", "MU_Sully", "MU_SullyInfinite", "BHS_Baymax", "GOG_Groot"}
MIN_BODY_COVER = 0.9
"""The body primitive must span this share of the figure's height (some figures keep their head apart)."""

MODELS = {  # joint value -> raw measurements it is predicted from
    "shoulder_x": ("shoulder_x", "tip_x"),
    "hand_x": ("shoulder_x", "tip_x", "wrist"),
    "arm_z": ("arm_z",),
    "knee_z": ("knee_z",),
    "neck_y": ("neck_y",),
}


def samples():
    raw_calibrate = skeleton.calibrate
    skeleton.calibrate = lambda p: p  # measure the raw values
    try:
        for actor, row in json.loads(INDEX_PATH.read_text(encoding="utf-8")).items():
            if row["status"]["state"] == "unsupported" or actor in NOT_HUMAN:
                continue
            base = BaseCharacter.from_pack(find_entry(actor))
            bind, tris, uvs, idx, w = official_skin(base)
            if np.ptp(bind[:, 1]) / base.height < MIN_BODY_COVER:
                continue
            tposed, turns = t_pose(base, bind, idx, w)
            model = CharacterModel(SourceModel(actor, tposed, tris, uvs, Image.new("RGB", (8, 8)), base.winding_sign),
                                   Placement(height=base.fit_height))
            placed = model.vertices()
            k = np.ptp(placed[:, 1]) / np.ptp(tposed[:, 1])
            shift = placed.mean(0) - tposed.mean(0) * k
            props, _ = skeleton.measure(placed, model.faces, base)
            joints, exact = {}, {j for j, s in base.skeleton.status.items() if s == "exact"}
            for name, p in base.joints.items():
                p = np.asarray(p, float)
                head, _, side = name.partition("_")
                if side in turns and head in ARM_JOINTS:
                    pivot, r = turns[side]
                    p = (p - pivot) @ r + pivot
                joints[name] = p * k + shift
            yield actor, props, joints, exact
    finally:
        skeleton.calibrate = raw_calibrate


def truth(props, joints, exact):
    h = props.height
    both = lambda n: (joints[f"{n}_Left"] + joints[f"{n}_Right"] * [-1, 1, 1]) / 2  # noqa: E731
    have = lambda *n: all(f"{x}_Left" in exact and f"{x}_Right" in exact for x in n)  # noqa: E731
    out = {}
    if have("Shoulder"):
        s = both("Shoulder")
        out["shoulder_x"], out["arm_z"] = s[0] / h, s[2] / h
        if have("Hand"):
            out["hand_x"] = both("Hand")[0] / h
    if "Neck" in exact:
        out["neck_y"] = joints["Neck"][1] / h
    if have("Knee"):
        out["knee_z"] = both("Knee")[2] / h
    return out


def main():
    rows = []
    for actor, props, joints, exact in samples():
        h = props.height
        raw = {"shoulder_x": props.shoulder_x / h, "tip_x": props.tip_x / h, "wrist": props.wrist,
               "arm_z": props.arm_z / h, "knee_z": props.knee_z / h, "neck_y": props.neck_y / h}
        raw_hand = (props.shoulder_x + props.wrist * (max(props.tip_x, props.shoulder_x + 0.05 * h) - props.shoulder_x)) / h
        rows.append((actor, raw, raw_hand, truth(props, joints, exact)))
    print(f"{len(rows)} figures")
    for target, feats in MODELS.items():
        data = [(r, rh, t[target]) for _, r, rh, t in rows if target in t]
        x = np.array([[1.0] + [r[f] for f in feats] for r, _, _ in data])
        y = np.array([t for _, _, t in data])
        current = np.array([rh if target == "hand_x" else r[target] for r, rh, _ in data])
        loo = []
        for i in range(len(y)):
            m = np.ones(len(y), bool)
            m[i] = False
            c = np.linalg.lstsq(x[m], y[m], rcond=None)[0]
            loo.append(abs(x[i] @ c - y[i]))
        c = np.linalg.lstsq(x, y, rcond=None)[0]
        print(f"{target:10s} n={len(y):2d}  raw {100 * np.abs(current - y).mean():.2f}% -> leave-one-out "
              f"{100 * np.mean(loo):.2f}%   {feats} {tuple(round(float(v), 4) for v in c)}")


if __name__ == "__main__":
    main()
