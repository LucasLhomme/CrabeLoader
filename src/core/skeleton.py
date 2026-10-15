"""Joint placement on the custom model.

The custom model is expected in T-pose (A-pose is tolerated). `measure` reads a handful of proportions off the
mesh (arm line, shoulder width, crotch, knees, ankles...), the UI lets the user nudge each of them, and
`build_anchors` turns them into the joint positions that drive both the preview and the export: bone-heat weights
are computed against these joints, then every body part is moved from its joint onto the base character's joint.
"""
from dataclasses import dataclass, fields, replace

import numpy as np
import trimesh
from scipy.spatial import ConvexHull

from src.core.base_character import BaseCharacter

SIDES = (("Left", 1.0), ("Right", -1.0))
CROTCH_MIN = 0.38  # below this the 'crotch' found is probably the hem of a coat or skirt
NECK_WINDOW = 0.10  # how far above the arm line the neck may be (fraction of the height)


@dataclass
class Proportions:
    """Everything is in game units on the placed model (feet at y=0, facing +Z, left hand on +X)."""
    height: float
    arm_y: float
    arm_z: float
    shoulder_x: float
    tip_x: float
    tip_y: float
    wrist: float
    """Wrist position along shoulder -> hand tip, 0..1."""
    neck_y: float
    head_y: float
    crotch_y: float
    torso_z: float
    leg_x: float
    knee: float
    """Knee position along ankle -> thigh, 0..1."""
    knee_z: float
    ankle_y: float
    ankle_z: float
    toe_z: float
    elbow: float = 0.5
    """Elbow position along shoulder -> hand, 0..1."""

    def adjusted(self, **changes) -> "Proportions":
        return replace(self, **changes)


@dataclass
class FitReport:
    detected: list[str]
    estimated: list[str]

    def summary(self) -> str:
        parts = []
        if self.detected:
            parts.append("detected: " + ", ".join(self.detected))
        if self.estimated:
            parts.append("estimated (check these): " + ", ".join(self.estimated))
        return " | ".join(parts)


def _sections(mesh: trimesh.Trimesh, y: float) -> np.ndarray:
    """Endpoints (K, 3) of the cross-section of the mesh with the horizontal plane at height y."""
    try:
        seg = trimesh.intersections.mesh_plane(mesh, plane_normal=[0, 1, 0], plane_origin=[0, y, 0])
    except (ValueError, IndexError):
        return np.zeros((0, 3))
    return np.asarray(seg, float).reshape(-1, 3) if len(seg) else np.zeros((0, 3))


SIGNIFICANT = 0.4


def _body_loop(mesh: trimesh.Trimesh, y: float, keep=None) -> np.ndarray:
    """Points (K, 3) of the biggest connected outline of the cross-section at height y.

    Hair strands, capes, skirts and wings that hang free next to the body give their own separate outlines; taking
    the whole section (min/max of everything) drags the measured centre towards them. `keep` filters the segment
    midpoints (e.g. one side of the body)."""
    try:
        seg = trimesh.intersections.mesh_plane(mesh, plane_normal=[0, 1, 0], plane_origin=[0, y, 0])
    except (ValueError, IndexError):
        return np.zeros((0, 3))
    if len(seg) == 0:
        return np.zeros((0, 3))
    seg = np.asarray(seg, float)
    if keep is not None:
        seg = seg[keep(seg.mean(axis=1))]
        if len(seg) == 0:
            return np.zeros((0, 3))
    ends = np.round(seg.reshape(-1, 3), 5)
    _, inverse = np.unique(ends, axis=0, return_inverse=True)
    inverse = inverse.reshape(-1, 2)
    parent = np.arange(inverse.max() + 1)

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for a, b in inverse:
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[ra] = rb
    label = np.array([find(a) for a in inverse[:, 0]])
    outlines = []
    for lab in np.unique(label):
        pts = seg[label == lab].reshape(-1, 3)
        try:
            area = float(ConvexHull(pts[:, [0, 2]]).volume)  # 2-D hull: .volume is the area
        except Exception:  # degenerate (flat or collinear) outline: a cape edge-on, a strand
            area = 0.0
        outlines.append((area, pts))
    top = max(o[0] for o in outlines)
    if top <= 0:
        return np.zeros((0, 3))
    # Layered clothing gives several near-identical outlines for one body: keep every outline that is big enough
    # compared with the biggest, drop the thin or small ones (hair strands, capes edge-on, wing tips).
    return np.vstack([pts for area, pts in outlines if area >= SIGNIFICANT * top])


def _crosses_midline(mesh: trimesh.Trimesh, y: float, x_limit: float) -> bool:
    try:
        seg = trimesh.intersections.mesh_plane(mesh, plane_normal=[0, 1, 0], plane_origin=[0, y, 0])
    except (ValueError, IndexError):
        return False
    if len(seg) == 0:
        return False
    seg = np.asarray(seg, float)
    x0, x1 = seg[:, 0, 0], seg[:, 1, 0]
    inside = (np.abs(x0) < x_limit) & (np.abs(x1) < x_limit)
    return bool(np.any(inside & (np.sign(x0) != np.sign(x1))))


def measure(vertices: np.ndarray, faces: np.ndarray, base: BaseCharacter) -> tuple[Proportions, FitReport]:
    mesh = trimesh.Trimesh(vertices, faces, process=False)
    x, y, z = vertices.T
    h = float(y.max())
    detected, estimated = [], []

    # Arm line: the vertices furthest out on each side (mirrored and averaged).
    upper = y > 0.3 * h
    tips = []
    for _, sx in SIDES:
        sx_vals = sx * x
        cut = np.percentile(sx_vals[upper], 99.5)
        sel = upper & (sx_vals >= cut)
        tips.append((sx_vals[sel].mean(), y[sel].mean(), z[sel].mean()))
    tip_x, tip_y, tip_z = np.mean(tips, axis=0)

    # Shoulder: walking in from the hand tip, the torso starts where the slab reaches far below the arm.
    step = h / 80
    shoulder_edges, thickness_by_side = [], []
    for _, sx in SIDES:
        side = (sx * x > 0) & (y > 0.25 * h)
        xs = sx * x
        edge, profile = None, []
        b = tip_x
        curr_arm_y = tip_y
        while b > step:
            sel = side & (xs <= b) & (xs > b - step)
            if sel.any():
                lo, hi = y[sel].min(), y[sel].max()
                yc = (hi + lo) / 2
                if lo < curr_arm_y - 0.2 * h:
                    edge = b
                    break
                curr_arm_y = yc
                profile.append((b - step / 2, hi - lo, yc, z[sel].mean()))
            b -= step
        shoulder_edges.append(edge)
        thickness_by_side.append(profile)
    if all(e is not None for e in shoulder_edges) and all(len(p) >= 4 for p in thickness_by_side):
        edge = float(np.mean(shoulder_edges))
        near = [t for prof in thickness_by_side for (bx, t, _, _) in prof if bx < edge + 0.35 * (tip_x - edge)]
        arm_thick = float(np.median(near)) if near else 0.05 * h
        shoulder_inward = min(0.12 * arm_thick, 0.025 * h)
        shoulder_x = float(np.clip(edge - shoulder_inward, 0.08 * h, 0.8 * tip_x))
        near_y = [yc for prof in thickness_by_side for (bx, _, yc, _) in prof if bx < edge + 0.25 * (tip_x - edge)]
        arm_y = float(np.mean(near_y)) if near_y else float(tip_y)
        arm_z = float(np.mean([zz for prof in thickness_by_side for (_, _, _, zz) in prof]))
        detected.append("shoulders")
        # Wrist: the narrowest slab in the outer part of the arm.
        span = tip_x - shoulder_x
        best = None
        for prof in thickness_by_side:
            for bx, t, _, _ in prof:
                f = (bx - shoulder_x) / span if span > 0 else 0
                if 0.4 <= f <= 0.85 and (best is None or t < best[1]):
                    best = (f, t)
        wrist = float(best[0]) if best else 0.55
        (detected if best else estimated).append("wrists")
    else:
        shoulder_x, arm_y, arm_z, wrist = 0.12 * h, float(tip_y), float(tip_z), 0.55
        estimated += ["shoulders", "wrists"]

    # Neck: narrowest central cross-section just above the arm line.
    neck_y, best_w = arm_y + 0.01 * h, None
    for yy in np.linspace(arm_y + 0.01 * h, min(arm_y + NECK_WINDOW * h, 0.95 * h), 25):
        pts = _sections(mesh, yy)
        pts = pts[np.abs(pts[:, 0]) < shoulder_x] if len(pts) else pts
        if len(pts) >= 3:
            w = np.ptp(pts[:, 0])
            if best_w is None or w < best_w:
                neck_y, best_w = float(yy), w
    (detected if best_w is not None else estimated).append("neck")
    head_y = neck_y + 0.3 * (h - neck_y)

    # Crotch: walking down from the chest, the last height where the cross-section still joins across the
    # midline (walking down rather than up ignores shoes or knees that touch lower down).
    crotch_y = None
    misses = 0
    for yy in np.arange(arm_y - 0.1 * h, 0.08 * h, -h / 200):
        if _crosses_midline(mesh, yy, shoulder_x):
            crotch_y, misses = float(yy), 0
        else:
            misses += 1
            if misses >= 3:
                break
    # Where the base figure's legs start, as a fraction of its height: the fallback when the cross-sections lie
    # (a long coat or skirt joins across the midline down to its hem, so "the last joined section" is the hem).
    thigh = base.joints.get("Thigh_Left") if base is not None else None
    prior = float(np.clip(thigh[1] / base.height, 0.40, 0.56)) * h if thigh is not None else 0.47 * h
    if crotch_y is None or crotch_y < 0.15 * h or crotch_y > arm_y - 0.12 * h:
        crotch_y = prior
        estimated.append("hips")
    elif crotch_y < CROTCH_MIN * h and crotch_y < prior - 0.08 * h:
        crotch_y = prior
        estimated.append("hips (long coat or skirt?)")
    else:
        detected.append("hips")

    # Torso depth centre, halfway between crotch and arm line.
    mid_y = (crotch_y + arm_y) / 2
    pts = _body_loop(mesh, mid_y, keep=lambda m: np.abs(m[:, 0]) < shoulder_x)
    if len(pts) < 3:
        pts = _sections(mesh, mid_y)
        pts = pts[np.abs(pts[:, 0]) < shoulder_x] if len(pts) else pts
    torso_z = float((pts[:, 2].min() + pts[:, 2].max()) / 2) if len(pts) else 0.0

    # Legs: centre of each leg's cross-section at knee height.
    knee_probe = 0.55 * crotch_y
    pts = _sections(mesh, knee_probe)
    legs = []
    for _, sx in SIDES:
        p = _body_loop(mesh, knee_probe, keep=lambda m, sx=sx: sx * m[:, 0] > 0)
        if len(p) < 3:
            p = pts[sx * pts[:, 0] > 0] if len(pts) else pts
        if len(p):
            legs.append(((np.abs(p[:, 0]).min() + np.abs(p[:, 0]).max()) / 2, (p[:, 2].min() + p[:, 2].max()) / 2))
    if len(legs) == 2:
        leg_x = float(np.mean([l[0] for l in legs]))
        knee_z = float(np.mean([l[1] for l in legs]))
        detected.append("legs")
    else:
        leg_x, knee_z = 0.05 * h, torso_z
        estimated.append("legs")

    # Ankle: where the foot's front-to-back depth drops to about half of the sole's.
    def leg_depth(yy):
        p = _sections(mesh, yy)
        p = p[(p[:, 0] > 0) & (np.abs(p[:, 0] - leg_x) < 0.15 * h)] if len(p) else p
        return (np.ptp(p[:, 2]), (p[:, 2].min() + p[:, 2].max()) / 2) if len(p) >= 3 else (None, None)

    sole_depth, _ = leg_depth(0.02 * h)
    ankle_y, ankle_z = 0.06 * h, knee_z
    found_ankle = False
    if sole_depth:
        for yy in np.linspace(0.03 * h, 0.2 * h, 35):
            d, zc = leg_depth(yy)
            if d is not None and d < 0.55 * sole_depth:
                ankle_y, ankle_z, found_ankle = float(yy), float(zc), True
                break
    (detected if found_ankle else estimated).append("ankles")
    feet = (y < max(ankle_y, 0.05 * h)) & (np.abs(np.abs(x) - leg_x) < 0.2 * h)
    toe_z = float(z[feet].max()) if feet.any() else ankle_z + 0.1 * h

    props = Proportions(
        height=h, arm_y=float(arm_y), arm_z=arm_z, shoulder_x=shoulder_x, tip_x=float(tip_x), tip_y=float(tip_y),
        wrist=wrist, neck_y=neck_y, head_y=head_y, crotch_y=crotch_y, torso_z=torso_z,
        leg_x=leg_x, knee=0.48, knee_z=knee_z, ankle_y=ankle_y, ankle_z=ankle_z, toe_z=toe_z)
    return props, FitReport(detected, estimated)


def build_anchors(p: Proportions, base: BaseCharacter) -> dict[str, np.ndarray]:
    h = p.height
    a: dict[str, np.ndarray] = {}
    thigh_y = p.crotch_y + 0.03 * h
    a["HipsLow"] = np.array([0.0, p.crotch_y, p.torso_z])
    a["Hips"] = np.array([0.0, p.crotch_y + 0.06 * h, p.torso_z])
    a["Neck"] = np.array([0.0, p.neck_y, p.torso_z])
    a["Head"] = np.array([0.0, max(p.head_y, p.neck_y + 0.01 * h), p.torso_z])
    a["HeadTop"] = np.array([0.0, max(h, a["Head"][1] + 0.02 * h), p.torso_z])

    joints = base.joints
    lh, ln = joints["Hips"][1], joints["Neck"][1]

    def along_torso(name, x=0.0, z=None):
        t = (joints[name][1] - lh) / (ln - lh)
        q = a["Hips"] + t * (a["Neck"] - a["Hips"])
        q[0] = x
        if z is not None:
            q[2] = z
        return q

    a["Spine2"] = along_torso("Spine2")
    a["Spine3"] = along_torso("Spine3")
    knee_y = p.ankle_y + p.knee * (thigh_y - p.ankle_y)
    # The feet are moved rigidly by (base ankle - this anchor): keeping the anchor at the base's ankle height
    # leaves the soles on the ground whatever the shin length.
    ankle_anchor_y = min(joints["Ankle_Left"][1], 0.5 * knee_y)
    for side, sx in SIDES:
        shoulder = np.array([sx * p.shoulder_x, p.arm_y, p.arm_z])
        tip = np.array([sx * max(p.tip_x, p.shoulder_x + 0.05 * h), p.tip_y, p.arm_z])
        hand = shoulder + p.wrist * (tip - shoulder)
        a[f"Shoulder_{side}"] = shoulder
        a[f"Hand_{side}"] = hand
        a[f"Elbow_{side}"] = shoulder + p.elbow * (hand - shoulder)
        a[f"HandTip_{side}"] = tip
        a[f"Clavicle_{side}"] = along_torso(f"Clavicle_{side}", sx * 0.017 * h, p.torso_z - 0.015 * h)
        a[f"ClavicleHeat_{side}"] = np.array([sx * 0.017 * h, p.arm_y - 0.02 * h, p.torso_z - 0.015 * h])
        a[f"Thigh_{side}"] = np.array([sx * p.leg_x, thigh_y, p.torso_z])
        a[f"Knee_{side}"] = np.array([sx * p.leg_x, knee_y, p.knee_z])
        a[f"AnkleTrue_{side}"] = np.array([sx * p.leg_x, p.ankle_y, p.ankle_z])
        a[f"Ankle_{side}"] = np.array([sx * p.leg_x, ankle_anchor_y, p.ankle_z])
        a[f"Ball_{side}"] = np.array([sx * p.leg_x, 0.015 * h, (p.ankle_z + p.toe_z) / 2])
        a[f"ToeTip_{side}"] = np.array([sx * p.leg_x, 0.015 * h, p.toe_z])
    return a


def heat_bones() -> list[tuple[str, str, str, str]]:
    """(bone, head anchor, tail anchor, parent) for the bone-heat armature; bone names are the base's."""
    bones = [("Hips", "HipsLow", "Spine2", ""), ("Spine2", "Spine2", "Spine3", "Hips"),
             ("Spine3", "Spine3", "Neck", "Spine2"), ("Neck", "Neck", "Head", "Spine3"),
             ("Head", "Head", "HeadTop", "Neck")]
    for side, _ in SIDES:
        bones += [(f"Clavicle_{side}", f"ClavicleHeat_{side}", f"Shoulder_{side}", "Spine3"),
                  (f"Shoulder_{side}", f"Shoulder_{side}", f"Elbow_{side}", f"Clavicle_{side}"),
                  (f"Elbow_{side}", f"Elbow_{side}", f"Hand_{side}", f"Shoulder_{side}"),
                  (f"Hand_{side}", f"Hand_{side}", f"HandTip_{side}", f"Elbow_{side}"),
                  (f"Thigh_{side}", f"Thigh_{side}", f"Knee_{side}", "Hips"),
                  (f"Knee_{side}", f"Knee_{side}", f"AnkleTrue_{side}", f"Thigh_{side}"),
                  (f"Ankle_{side}", f"AnkleTrue_{side}", f"Ball_{side}", f"Knee_{side}"),
                  (f"Ball_{side}", f"Ball_{side}", f"ToeTip_{side}", f"Ankle_{side}")]
    return bones


def skeleton_segments(anchors: dict[str, np.ndarray]) -> np.ndarray:
    """Line list (2K, 3) of the preview skeleton: the heat bones plus the torso links."""
    pairs = [(h, t) for _, h, t, _ in heat_bones()]
    pairs += [("Spine3", f"Clavicle_{s}") for s, _ in SIDES] + [("HipsLow", f"Thigh_{s}") for s, _ in SIDES]
    pts = [anchors[k] for pair in pairs for k in pair if pair[0] in anchors and pair[1] in anchors]
    return np.array(pts, dtype=np.float32).reshape(-1, 3)


def joint_points(anchors: dict[str, np.ndarray]) -> np.ndarray:
    skip = ("HeadTop", "HandTip", "ToeTip", "ClavicleHeat", "HipsLow", "AnkleTrue")
    return np.array([p for k, p in anchors.items() if not k.startswith(skip)], dtype=np.float32)


def base_skeleton_segments(joints: dict[str, np.ndarray]) -> np.ndarray:
    """Line list (2K, 3) representing the base figure's canonical reference skeleton in native bind pose."""
    pairs = [
        ("Hips", "Spine2"), ("Spine2", "Spine3"), ("Spine3", "Neck"), ("Neck", "Head")
    ]
    for s, _ in SIDES:
        pairs += [
            ("Spine3", f"Clavicle_{s}"),
            (f"Clavicle_{s}", f"Shoulder_{s}"),
            (f"Shoulder_{s}", f"Elbow_{s}"),
            (f"Elbow_{s}", f"Hand_{s}"),
            (f"Hand_{s}", f"Middle1_{s}"),
            ("Hips", f"Thigh_{s}"),
            (f"Thigh_{s}", f"Knee_{s}"),
            (f"Knee_{s}", f"Ankle_{s}"),
            (f"Ankle_{s}", f"Ball_{s}"),
        ]
    pts = [joints[k] for pair in pairs for k in pair if pair[0] in joints and pair[1] in joints]
    return np.array(pts, dtype=np.float32).reshape(-1, 3)


def base_joint_points(joints: dict[str, np.ndarray]) -> np.ndarray:
    """All 3D joint points (K, 3) for the base figure."""
    return np.array(list(joints.values()), dtype=np.float32)


def apply_apose_to_proportions(p: Proportions, base: BaseCharacter) -> Proportions:
    """Adjust proportions to match the base character's native A-pose arm drop angle."""
    bj = base.joints
    if "Shoulder_Left" in bj and "Hand_Left" in bj:
        sh = bj["Shoulder_Left"]
        hand = bj["Hand_Left"]
        base_dx = max(abs(hand[0] - sh[0]), 0.05)
        base_dy = hand[1] - sh[1]
        slope = base_dy / base_dx  # negative for downward drop
        arm_span = max(p.tip_x - p.shoulder_x, 0.1 * p.height)
        target_tip_y = p.arm_y + slope * arm_span
        target_tip_y = float(np.clip(target_tip_y, 0.25 * p.height, p.arm_y))
        target_wrist = float(np.clip(np.linalg.norm(hand - sh) / max(np.linalg.norm(bj.get("Middle1_Left", hand) - sh), 1e-3), 0.4, 0.85)) if "Middle1_Left" in bj else p.wrist
        return p.adjusted(tip_y=target_tip_y, wrist=target_wrist)
    return p.adjusted(tip_y=p.arm_y - 0.25 * p.height)


def apply_tpose_to_proportions(p: Proportions) -> Proportions:
    """Straighten proportions to horizontal T-pose."""
    return p.adjusted(tip_y=p.arm_y, wrist=0.55)


PROPORTION_FIELDS = tuple(f.name for f in fields(Proportions))



def fit_advice(p: Proportions, base: BaseCharacter) -> list[str]:
    """Where the model's proportions differ from the base figure's by 20 % or more, and what to do about it."""
    out = []
    j, bh = base.joints, max(base.height, 0.1)
    h = max(p.height, 0.1)
    name = base.display_name
    try:
        middle = base.bind_world[base.bone_names.index("Middle2_Left"), 3, :3]
        base_arm = float(np.linalg.norm(middle - j["Shoulder_Left"])) / bh
    except (ValueError, KeyError):
        base_arm = None
    arm = (max(p.tip_x, p.shoulder_x) - p.shoulder_x) / h
    if base_arm and abs(arm / base_arm - 1) > 0.2:
        out.append(f"Arms are {abs(arm / base_arm - 1):.0%} {'longer' if arm > base_arm else 'shorter'} than "
                   f"{name}'s: hands and elbows get stretched or squeezed when baked. Check 'Wrist position' (the dot "
                   "must sit on the wrist) or pick a base with similar arms.")
    base_sh = abs(float(j["Shoulder_Left"][0])) / bh
    if base_sh > 0.02 and abs(p.shoulder_x / h / base_sh - 1) > 0.25:
        out.append(f"Shoulders are {abs(p.shoulder_x / h / base_sh - 1):.0%} "
                   f"{'wider' if p.shoulder_x / h > base_sh else 'narrower'} than {name}'s: shoulders deform most "
                   "here. Check 'Shoulder width' (the dot must sit on the arm joint, not the torso edge).")
    base_hips = float(j["Hips"][1]) / bh
    if abs(p.crotch_y / h - base_hips) > 0.12:
        out.append(f"Legs: {name} has {base_hips:.0%} of its height below the hips, your model {p.crotch_y / h:.0%}. "
                   "Check 'Hips (crotch)'; for a cartoon body, pick a short-legged base.")
    return out
