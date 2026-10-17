"""Twist and volume bones: share limb weights with the helper bones every official figure uses.

All 37 standard figures of the game split each upper arm, forearm and thigh between two or three bones: the main
joint and helpers (Shoulder_Spin / Shoulder_OL, Forearm_OL, Thigh_OL). The engine drives the helpers itself (they
take part of the twist and keep the volume of the shoulder, forearm and hip), so a model weighted only on the main
joints twists its whole shoulder cap or hip with the arm or leg. This module copies the game's split, measured on
those figures (median share of each bone by position along the limb), onto the model's weights.

At bind pose nothing moves (both bones sit at the same place), so this only changes how limbs deform in game."""
import numpy as np

from src.core.skeleton import SIDES

T = np.array([-0.15, -0.05, 0.05, 0.15, 0.25, 0.35, 0.45, 0.55, 0.65, 0.75, 0.85, 0.95])
"""Position along the limb (0 = first joint, 1 = next joint) of each row of the profiles below."""
PROFILES = {
    # group: ((main bone, helpers...), (share of each bone at each T)), measured with tools/twist_profiles.py
    "upper_arm": (['Shoulder', 'Shoulder_Spin', 'Shoulder_OL'],
       (
        (0.05, 0.69, 0.24),
        (0.10, 0.63, 0.28),
        (0.17, 0.57, 0.24),
        (0.25, 0.53, 0.20),
        (0.41, 0.38, 0.13),
        (0.58, 0.10, 0.10),
        (0.71, 0.09, 0.06),
        (0.72, 0.01, 0.05),
        (0.94, 0.00, 0.00),
        (0.99, 0.00, 0.00),
        (1.00, 0.00, 0.00),
        (1.00, 0.00, 0.00),
       )),
    "forearm": (['Elbow', 'Forearm_OL', 'Elbow_OL'],
       (
        (1.00, 0.00, 0.00),
        (1.00, 0.00, 0.00),
        (0.91, 0.00, 0.00),
        (0.82, 0.04, 0.00),
        (0.79, 0.08, 0.00),
        (0.70, 0.27, 0.00),
        (0.65, 0.35, 0.00),
        (0.54, 0.46, 0.00),
        (0.33, 0.67, 0.00),
        (0.23, 0.77, 0.00),
        (0.09, 0.91, 0.00),
        (0.01, 0.99, 0.00),
       )),
    "thigh": (['Thigh', 'Thigh_OL'],
       (
        (0.10, 0.91),
        (0.00, 1.00),
        (0.02, 0.98),
        (0.06, 0.94),
        (0.12, 0.88),
        (0.18, 0.82),
        (0.26, 0.74),
        (0.40, 0.60),
        (0.60, 0.40),
        (0.71, 0.29),
        (0.90, 0.10),
        (0.94, 0.06),
       )),
}
SEGMENTS = {"upper_arm": ("Shoulder", "Elbow"), "forearm": ("Elbow", "Hand"), "thigh": ("Thigh", "Knee")}


def split(baked: np.ndarray, merged: np.ndarray, palette_cols, base, log=None):
    """Move part of each limb's main-bone weight onto its helper bones: (merged, palette_cols), columns added."""
    merged = merged.copy()
    palette_cols = list(np.asarray(palette_cols))
    names = list(base.bone_names)
    joints = base.joints
    moved = []
    for side, _ in SIDES:
        for group, (bones, shares) in PROFILES.items():
            main = f"{bones[0]}_{side}"
            a, e = (f"{j}_{side}" for j in SEGMENTS[group])
            if main not in names or a not in joints or e not in joints:
                continue
            main_bone = names.index(main)
            if main_bone not in palette_cols:
                continue
            helpers = [(k, names.index(f"{h}_{side}")) for k, h in enumerate(bones) if k and f"{h}_{side}" in names]
            if not helpers:
                continue
            col = palette_cols.index(main_bone)
            w = merged[:, col]
            if not w.any():
                continue
            start, end = np.asarray(joints[a], float), np.asarray(joints[e], float)
            seg = end - start
            t = ((baked - start) @ seg) / max(float(seg @ seg), 1e-12)
            table = np.asarray(shares, float)
            share = {k: np.interp(t, T, table[:, k]) for k in range(len(bones))}
            total = sum(share[k] for k in [0] + [k for k, _ in helpers])
            total = np.maximum(total, 1e-9)
            for k, bone in helpers:
                part = w * share[k] / total
                if bone not in palette_cols:
                    palette_cols.append(bone)
                    merged = np.c_[merged, np.zeros(len(merged))]
                merged[:, palette_cols.index(bone)] += part
                moved.append(float(part.sum()))
            merged[:, col] = w * share[0] / total
    if log and moved:
        log(f"  twist bones: {100 * sum(moved) / max(float(merged.sum()), 1e-9):.1f}% of the weight on the game's helper bones")
    return merged, np.asarray(palette_cols)
