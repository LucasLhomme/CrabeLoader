"""Finger skinning: the model's hand gets its weights from the base figure's finger bones.

The skeleton fitted on the model stops at the wrist ("Hand"), so a whole hand would follow one bone and stay flat
and rigid when the game closes the fingers (fist, grab, blaster). The baked mesh sits in the base figure's bind
pose, where the base's finger bones (Thumb1-3, Index1-2, Middle1-2) lie inside the hand. Each hand vertex beyond the
knuckles is shared between the nearest finger chain and the wrist, by its position along that chain. At bind pose
nothing moves, so this can only change how the hand bends in animations."""
import numpy as np

from src.core.skeleton import SIDES

CHAINS = (("Thumb", ("Thumb1", "Thumb2", "Thumb3")), ("Index", ("Index1", "Index2")),
          ("Middle", ("Middle1", "Middle2")))
REACH = 0.07
"""How far from a finger chain (fraction of the figure's height) a vertex still follows it."""
MIN_HAND_WEIGHT = 0.3


def _smooth(x: np.ndarray, lo: float, hi: float) -> np.ndarray:
    t = np.clip((x - lo) / (hi - lo), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def _chain(base, side: str, names):
    """Joint positions of a finger chain (start of each bone + extrapolated tip), or None if the palette lacks it."""
    try:
        pts = [base.bind_world[base.bone_names.index(f"{n}_{side}"), 3, :3].astype(float) for n in names]
    except ValueError:
        return None
    pts.append(pts[-1] + 0.8 * (pts[-1] - pts[-2]))
    return np.array(pts)


def chain_params(points: np.ndarray, chain: np.ndarray):
    """(distance to the chain, position along it in bone units: <0 before the first joint, n after the tip)."""
    best_d = np.full(len(points), np.inf)
    best_u = np.zeros(len(points))
    for k in range(len(chain) - 1):
        a, b = chain[k], chain[k + 1]
        ab = b - a
        t = ((points - a) @ ab) / max(float(ab @ ab), 1e-12)
        d = np.linalg.norm(points - (a + np.clip(t, 0, 1)[:, None] * ab), axis=1)
        closer = d < best_d
        best_d[closer] = d[closer]
        best_u[closer] = k + t[closer]
    return best_d, best_u


MIN_HAND_VERTICES = 60
"""Below this, the hand is a low-poly block (mitten): curling finger bones would only crumple it, so it stays rigid."""


def add_fingers(baked: np.ndarray, merged: np.ndarray, palette_cols: np.ndarray, base, log=None):
    """Move part of each hand's weight onto the finger bones: (merged, palette_cols) with finger columns added."""
    merged = merged.copy()
    palette_cols = np.asarray(palette_cols)
    added: dict[int, np.ndarray] = {}  # palette bone -> weight added per vertex
    reach = REACH * base.height
    for side, _ in SIDES:
        try:
            hand_col = int(np.flatnonzero(palette_cols == base.bone_index(f"Hand_{side}"))[0])
        except (ValueError, IndexError):
            continue
        sel = np.flatnonzero(merged[:, hand_col] > MIN_HAND_WEIGHT)
        chains = [(names, _chain(base, side, names)) for _, names in CHAINS]
        chains = [(n, c) for n, c in chains if c is not None]
        if len(sel) == 0 or not chains:
            continue
        if len(sel) < MIN_HAND_VERTICES:
            if log:
                log(f"  {side} hand: only {len(sel)} vertices (low-poly), no finger weights: it stays rigid")
            continue
        hand = merged[sel, hand_col]
        params = [chain_params(baked[sel], c) for _, c in chains]
        dist = np.array([d for d, _ in params])
        gate = np.exp(-(dist.min(0) / reach) ** 2)
        share = np.exp(-(dist / (0.5 * reach)) ** 2)
        share /= np.maximum(share.sum(0), 1e-9)
        moved = np.zeros(len(sel))
        for (names, _), (_, u), s_c in zip(chains, params, share):
            f = hand * gate * s_c * _smooth(u, -0.5, 0.1)
            moved += f
            for k, name in enumerate(names):
                tent = np.clip(1 - np.abs(u - (k + 0.5)), 0, 1)
                if k == 0:
                    tent = np.where(u < 0.5, 1.0, tent)
                if k == len(names) - 1:
                    tent = np.where(u > k + 0.5, 1.0, tent)
                column = added.setdefault(base.bone_index(f"{name}_{side}"), np.zeros(len(merged)))
                column[sel] += f * tent
        merged[sel, hand_col] -= moved
    for bone, column in added.items():
        where = np.flatnonzero(palette_cols == bone)
        if len(where):
            merged[:, where[0]] += column
        else:
            palette_cols = np.append(palette_cols, bone)
            merged = np.c_[merged, column]
    return merged, palette_cols
