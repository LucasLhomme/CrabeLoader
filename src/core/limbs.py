"""Limb thickness: fan models often have much thinner arms and legs than the chunky Disney Infinity figures, and the
game's skinning pinches thin limbs even more when they twist. The limbs can be inflated around their bone axis
(length unchanged), blended by the skinning weights so the shoulders and hips stay smooth."""
import numpy as np

ARM_SEGMENTS = (("Shoulder", "Elbow"), ("Elbow", "Hand"))
LEG_SEGMENTS = (("Thigh", "Knee"), ("Knee", "Ankle"))
SIDES = ("Left", "Right")
TORSO_SEGMENTS = (("Hips", "Spine2", 0.5), ("Spine2", "Spine3", 1.0), ("Spine3", "Neck", 1.0))
"""Torso bones (head, tail, share of the effect): the pelvis only follows halfway, the neck and head not at all."""
MAX_TORSO = 1.4
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
            arm_scale: float = 1.0, leg_scale: float = 1.0, torso_scale: float = 1.0) -> np.ndarray:
    """T-pose vertices pushed away from their limb (or spine) axes by (scale - 1), weighted by each bone's weight."""
    out = verts.copy()
    if abs(torso_scale - 1.0) >= 1e-3:
        for head, tail, share in TORSO_SEGMENTS:
            if head not in names or head not in anchors or tail not in anchors:
                continue
            w = weights[:, names.index(head)]
            perp, length = _segment_offsets(verts, anchors[head], anchors[tail])
            out += ((torso_scale - 1.0) * share * w)[:, None] * perp
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


def torso_radius(verts: np.ndarray, joints: dict) -> float | None:
    """Median distance to the spine of the chest/belly vertices, arms left out (they hang or stick out sideways)."""
    a, b = joints["Hips"], joints["Neck"]
    ab = b - a
    length = float(np.linalg.norm(ab))
    reach = 0.8 * abs(joints["Shoulder_Left"][0] - joints["Shoulder_Right"][0]) / 2
    if length < 1e-6 or reach < 1e-6:
        return None
    t = (verts - a) @ ab / (length * length)
    perp = np.linalg.norm(verts - a - np.outer(t, ab), axis=1)
    sel = (t > 0.25) & (t < 0.75) & (perp < reach)
    return float(np.median(perp[sel])) if sel.sum() >= 8 else None


def suggest_torso(model_verts, anchors, base_verts, base_joints) -> float:
    m, b = torso_radius(model_verts, anchors), torso_radius(base_verts, base_joints)
    value = b / m if m and b else 1.0
    return round(float(np.clip(value, 1.0, MAX_TORSO)) * 20) / 20


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


_OTHER = (("Hips", "Spine2"), ("Spine2", "Spine3"), ("Spine3", "Neck"), ("Neck", "Head"), ("Head", "HeadTop"))
_OTHER_SIDED = (("Clavicle", "Shoulder"), ("Hand", "HandTip"), ("Ankle", "ToeTip"), ("Hips", "Thigh"))


def preview_weights(verts: np.ndarray, anchors: dict) -> tuple[np.ndarray, list]:
    """Rough limb weights from the skeleton alone (no rig needed), for showing the thickness live.

    Every vertex is shared between the bone segments by inverse distance (power 6), so it is close to 1 in the middle
    of a limb and fades out around the shoulders, hips, hands and feet like real weights do."""
    segments, names = [], []
    for head, tail in _OTHER:
        if head in anchors and tail in anchors:
            segments.append((anchors[head], anchors[tail]))
            names.append(head)  # Hips, Spine2, Spine3: the torso thickness reads these
    for side in SIDES:
        for head, tail in ARM_SEGMENTS + LEG_SEGMENTS:
            segments.append((anchors[f"{head}_{side}"], anchors[f"{tail}_{side}"]))
            names.append(f"{head}_{side}")
        for head, tail in _OTHER_SIDED:
            a = anchors.get(f"{head}_{side}", anchors.get(head))
            b = anchors.get(f"{tail}_{side}")
            if a is not None and b is not None:
                segments.append((a, b))
                names.append(f"_{head}_{side}")
    dist = np.stack([np.linalg.norm(_segment_offsets(verts, a, b)[0], axis=1) for a, b in segments], axis=1)
    inv = 1.0 / np.maximum(dist, 1e-4) ** 6
    return inv / inv.sum(axis=1, keepdims=True), names


def preview(verts: np.ndarray, anchors: dict, arm_scale: float, leg_scale: float,
            torso_scale: float = 1.0) -> np.ndarray:
    if max(abs(arm_scale - 1), abs(leg_scale - 1), abs(torso_scale - 1)) < 1e-3:
        return verts
    weights, names = preview_weights(verts, anchors)
    return inflate(verts, weights, names, anchors, arm_scale, leg_scale, torso_scale)
