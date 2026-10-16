"""Limb thickness: fan models often have much thinner arms and legs than the chunky Disney Infinity figures, and the
game's skinning pinches thin limbs even more when they twist. The limbs can be inflated around their bone axis
(length unchanged), blended by the skinning weights so the shoulders and hips stay smooth."""
import numpy as np

ARM_SEGMENTS = (("Shoulder", "Elbow"), ("Elbow", "Hand"))
LEG_SEGMENTS = (("Thigh", "Knee"), ("Knee", "Ankle"))
SIDES = ("Left", "Right")
MAX_SUGGEST = 1.6
REACH = 0.6
"""Vertices farther from the axis than REACH x segment length (capes, wings) are left alone."""


def _segment_offsets(verts, a, b):
    ab = b - a
    length = float(np.linalg.norm(ab))
    t = np.clip((verts - a) @ ab / max(length * length, 1e-12), 0.0, 1.0)
    perp = verts - (a + np.outer(t, ab))
    return perp, length


def inflate(verts: np.ndarray, weights: np.ndarray, names: list, anchors: dict,
            arm_scale: float = 1.0, leg_scale: float = 1.0) -> np.ndarray:
    """T-pose vertices pushed away from their limb axes by (scale - 1), weighted by each limb bone's weight."""
    out = verts.copy()
    for segments, scale in ((ARM_SEGMENTS, arm_scale), (LEG_SEGMENTS, leg_scale)):
        if abs(scale - 1.0) < 1e-3:
            continue
        for side in SIDES:
            for head, tail in segments:
                bone = f"{head}_{side}"
                if bone not in names:
                    continue
                w = weights[:, names.index(bone)]
                if not w.any():
                    continue
                perp, length = _segment_offsets(verts, anchors[bone], anchors[f"{tail}_{side}"])
                near = np.linalg.norm(perp, axis=1) < REACH * length
                out += ((scale - 1.0) * w * near)[:, None] * perp
    return out


def limb_radius(verts: np.ndarray, a: np.ndarray, b: np.ndarray) -> float | None:
    """Median distance to the axis of the vertices around the middle of a segment (0 when nothing is there)."""
    ab = b - a
    length = float(np.linalg.norm(ab))
    if length < 1e-6:
        return None
    t = (verts - a) @ ab / (length * length)
    perp = np.linalg.norm(verts - a - np.outer(t, ab), axis=1)
    sel = (t > 0.2) & (t < 0.8) & (perp < 0.3 * length)
    return float(np.median(perp[sel])) if sel.sum() >= 8 else None


def suggest(model_verts, anchors, base_verts, base_joints) -> tuple[float, float]:
    """(arm, leg) scales that bring the model's limbs to the base figure's thickness (never thinner, at most 1.6)."""
    out = []
    for segments in (ARM_SEGMENTS, LEG_SEGMENTS):
        ratios = []
        for side in SIDES:
            for head, tail in segments:
                m = limb_radius(model_verts, anchors[f"{head}_{side}"], anchors[f"{tail}_{side}"])
                b = limb_radius(base_verts, base_joints[f"{head}_{side}"], base_joints[f"{tail}_{side}"])
                if m and b:
                    ratios.append(b / m)
        value = float(np.median(ratios)) if ratios else 1.0
        out.append(round(float(np.clip(value, 1.0, MAX_SUGGEST)) * 20) / 20)
    return out[0], out[1]
