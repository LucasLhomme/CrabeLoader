"""Turn a placed T-pose model into a mesh skinned on the base character's own skeleton.

The base skeleton and its inverse-bind matrices are kept byte-exact, so every shipped animation plays on the new
mesh. Instead of rigging the mesh to a new skeleton, the mesh is baked into the base's bind pose: each body part
is rotated/stretched from its fitted joint segment onto the base's segment (arms come down, joints line up).
"""
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

import numpy as np

from src.core.base_character import BaseCharacter
from src.core.model import CharacterModel
from src.core.rigging import compute_weights, quantize_weights, weld
from src.core.skeleton import SIDES

MAX_VERTICES = 65536 - 4
HEAVY_TRIANGLES = 10000


@dataclass
class RigResult:
    base: BaseCharacter
    tpose: np.ndarray
    baked: np.ndarray
    faces: np.ndarray
    uvs: np.ndarray
    normals: np.ndarray
    tangents: np.ndarray
    tangent_sign: np.ndarray
    bone_idx: np.ndarray
    bone_w: np.ndarray
    method: str
    anchors: dict

    def posed(self, pose: str, amount: float = 1.0) -> np.ndarray:
        """Vertices in a preview pose; `amount` scales the pose angle (1.0 = the standard test pose)."""
        world = self.base.bind_world
        if pose == "tpose":
            return self.tpose
        if pose == "bind":
            return self.baked
        posed = POSES[pose](self.base, world, amount)
        return skin(self.baked, self.bone_idx, self.bone_w, world, posed)

    def bind_roundtrip_error(self) -> float:
        world = self.base.bind_world
        return float(np.abs(skin(self.baked, self.bone_idx, self.bone_w, world, world) - self.baked).max())


def rotation_between(a, b):
    a = a / np.linalg.norm(a)
    b = b / np.linalg.norm(b)
    v = np.cross(a, b)
    c = float(np.dot(a, b))
    if c < -0.9999:
        axis = np.cross(a, [1.0, 0, 0] if abs(a[0]) < 0.9 else [0, 1.0, 0])
        axis /= np.linalg.norm(axis)
        return 2 * np.outer(axis, axis) - np.eye(3)
    vx = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + vx + vx @ vx / (1.0 + c)


def segment_transform(js, cs, jl, cl, axial=False, thicken=False):
    """Row-vector affine map taking segment js->cs onto jl->cl (stretches along it if axial, limbs thickened by sqrt(k))."""
    ds, dl = cs - js, cl - jl
    rot_row = rotation_between(ds, dl).T
    k = float(np.clip(np.linalg.norm(dl) / np.linalg.norm(ds), 0.85, 1.15)) if axial else 1.0
    r = float(np.sqrt(k)) if thicken and k > 1.0 else 1.0
    u = ds / np.linalg.norm(ds)
    m = (r * np.eye(3) + (k - r) * np.outer(u, u)) @ rot_row
    return m, jl - js @ m


def bake_transforms(anchors, base_pos, preserve_proportions=True) -> dict[str, tuple[np.ndarray, np.ndarray]]:
    stretch = not preserve_proportions
    specs = {"Hips": ("Hips", "Spine2", "Hips", "Spine2", False),
             "Spine2": ("Spine2", "Spine3", "Spine2", "Spine3", stretch),
             "Spine3": ("Spine3", "Neck", "Spine3", "Neck", stretch),
             "Neck": ("Neck", "Head", "Neck", "Head", stretch)}
    for side, _ in SIDES:
        specs[f"Clavicle_{side}"] = (f"Clavicle_{side}", f"Shoulder_{side}", f"Clavicle_{side}", f"Shoulder_{side}", stretch)
        specs[f"Shoulder_{side}"] = (f"Shoulder_{side}", f"Elbow_{side}", f"Shoulder_{side}", f"Elbow_{side}", stretch)
        specs[f"Elbow_{side}"] = (f"Elbow_{side}", f"Hand_{side}", f"Elbow_{side}", f"Hand_{side}", stretch)
        specs[f"Hand_{side}"] = (f"Hand_{side}", f"HandTip_{side}", f"Hand_{side}", f"Middle1_{side}", False)
        specs[f"Thigh_{side}"] = (f"Thigh_{side}", f"Knee_{side}", f"Thigh_{side}", f"Knee_{side}", stretch)
        specs[f"Knee_{side}"] = (f"Knee_{side}", f"Ankle_{side}", f"Knee_{side}", f"Ankle_{side}", stretch)
    out = {}
    for bone, (sj, sc, lj, lc, axial) in specs.items():
        limb = bone.startswith(("Shoulder_", "Elbow_", "Thigh_", "Knee_")) and stretch
        out[bone] = segment_transform(anchors[sj], anchors[sc], base_pos[lj], base_pos[lc], axial, thicken=limb)
    out["Head"] = (np.eye(3), base_pos["Head"] - anchors["Head"])
    for side, _ in SIDES:
        foot = (np.eye(3), base_pos[f"Ankle_{side}"] - anchors[f"Ankle_{side}"])
        out[f"Ankle_{side}"] = foot
        out[f"Ball_{side}"] = foot
    return out


def vertex_normals(verts, faces):
    fn = np.cross(verts[faces[:, 1]] - verts[faces[:, 0]], verts[faces[:, 2]] - verts[faces[:, 0]])
    n = np.zeros_like(verts)
    for c in range(3):
        np.add.at(n, faces[:, c], fn)
    return n / np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)


def vertex_tangents(verts, uvs, normals, faces):
    p0, p1, p2 = verts[faces[:, 0]], verts[faces[:, 1]], verts[faces[:, 2]]
    t0, t1, t2 = uvs[faces[:, 0]], uvs[faces[:, 1]], uvs[faces[:, 2]]
    e1, e2, d1, d2 = p1 - p0, p2 - p0, t1 - t0, t2 - t0
    r = d1[:, 0] * d2[:, 1] - d2[:, 0] * d1[:, 1]
    r = np.where(np.abs(r) < 1e-12, 1e-12, r)
    sdir = (e1 * d2[:, 1:2] - e2 * d1[:, 1:2]) / r[:, None]
    tdir = (e2 * d1[:, 0:1] - e1 * d2[:, 0:1]) / r[:, None]
    tan, bit = np.zeros_like(verts), np.zeros_like(verts)
    for c in range(3):
        np.add.at(tan, faces[:, c], sdir)
        np.add.at(bit, faces[:, c], tdir)
    tan = tan - normals * (tan * normals).sum(1, keepdims=True)
    bad = np.linalg.norm(tan, axis=1) < 1e-9
    tan[bad] = np.cross(normals[bad], [0.0, 0.0, 1.0]) + 1e-6
    tan /= np.linalg.norm(tan, axis=1, keepdims=True)
    w = np.where((np.cross(normals, tan) * bit).sum(1) < 0, -1.0, 1.0)
    return tan, w


def build_rig(model: CharacterModel, anchors: dict, base: BaseCharacter, blender: Path | None, work: Path,
              log: Callable[[str], None], piece_overrides=None, weight_transfer: bool = False, fingers: bool = True) -> RigResult:
    """`piece_overrides`: [(source vertex indices, bone name)] - the pieces holding those vertices follow that bone.

    `weight_transfer`: when True, the rig is built both ways (bone heat and weights copied from the base body) and
    the one that deforms less is kept."""
    standard = _build_rig(model, anchors, base, blender, work, log, piece_overrides, None, fingers)
    if not weight_transfer:
        return standard
    from src.core.quality import deformation_report

    robust_lines = lambda text: log(text) if text.strip().startswith(("Robust", "too few", "inpainting")) else None  # noqa: E731
    try:
        robust = _build_rig(model, anchors, base, blender, work, robust_lines, piece_overrides, weight_transfer, fingers)
    except Exception as exc:  # noqa: BLE001 - optional feature
        log(f"  Robust transfer unavailable ({exc}) -> standard weights")
        return standard
    if robust.method != "robust transfer":
        return standard
    a, b = deformation_report(standard), deformation_report(robust)
    keep_robust = (b.score, -b.distortion) > (a.score, -a.distortion)
    log(f"  Weights: standard {a.score}/100, robust transfer {b.score}/100 -> "
        f"{'robust transfer' if keep_robust else 'standard'} kept")
    return robust if keep_robust else standard


def _build_rig(model: CharacterModel, anchors: dict, base: BaseCharacter, blender: Path | None, work: Path,
               log: Callable[[str], None], piece_overrides, weight_transfer: bool, finger_weights: bool = True) -> RigResult:
    positions = model.vertices()
    faces = model.faces
    uvs = model.uvs

    log(f"Welding UV seams ({len(positions)} vertices, {len(faces)} triangles)...")
    wverts, wfaces, corner_to_weld, ok = weld(positions, faces)
    faces = faces[ok]
    overrides = [(positions[i], bone) for indices, bone in (piece_overrides or []) for i in indices
                 if 0 <= i < len(positions)]
    transfer = None
    if weight_transfer:
        from src.core import weight_transfer as wt
        transfer = lambda v, f, n: wt.transfer(v, f, base, anchors, n, log)  # noqa: E731
    accessory = None
    if model.ignored is not None and model.ignored.any():
        accessory = np.zeros(len(wverts), bool)
        accessory[corner_to_weld[np.flatnonzero(model.ignored)]] = True
    names, weights, method = compute_weights(wverts, wfaces[ok], anchors, blender, work, log, overrides, transfer,
                                             accessory)

    log("Building the final vertex set...")
    uv_key = np.round(uvs * 65536).astype(np.int64)
    keys = np.c_[corner_to_weld, uv_key]
    _, first, remap = np.unique(keys, axis=0, return_index=True, return_inverse=True)
    remap = remap.reshape(-1)
    vweld, vuv, vfaces = corner_to_weld[first], uvs[first], remap[faces]
    used = np.unique(vfaces)  # removed parts leave vertices that no face uses
    if len(used) < len(vweld):
        compact = np.full(len(vweld), -1)
        compact[used] = np.arange(len(used))
        vweld, vuv, vfaces = vweld[used], vuv[used], compact[vfaces]
    if len(vweld) > MAX_VERTICES:
        raise ValueError(f"model too detailed: {len(vweld)} vertices (max {MAX_VERTICES}). "
                         "Decimate it in Blender (Decimate modifier) and import it again.")
    sign = model.source.winding_sign
    if base.winding_sign * sign < 0:
        vfaces = vfaces[:, ::-1]
        log("  triangle winding flipped to match the game")

    log(f"Baking into {base.display_name}'s bind pose...")
    bake = bake_transforms(anchors, base.joints)
    w = weights[vweld]
    tpose = wverts[vweld]
    if abs(model.arm_scale - 1) > 1e-3 or abs(model.leg_scale - 1) > 1e-3:
        from src.core.limbs import inflate
        tpose = inflate(tpose, w, list(names), anchors, model.arm_scale, model.leg_scale)
        log(f"  limbs thickened: arms x{model.arm_scale:.2f}, legs x{model.leg_scale:.2f}")
    tnormals = sign * vertex_normals(wverts, wfaces[ok])[vweld]
    baked = np.zeros_like(tpose)
    bnormals = np.zeros_like(tpose)
    for col, bone in enumerate(names):
        m, t = bake[bone]
        baked += w[:, col:col + 1] * (tpose @ m + t)
        bnormals += w[:, col:col + 1] * (tnormals @ np.linalg.inv(m).T)
    bnormals /= np.maximum(np.linalg.norm(bnormals, axis=1, keepdims=True), 1e-12)
    tangents, tsign = vertex_tangents(baked, vuv, bnormals, vfaces)

    # Several canonical joints can share a palette bone on adapted skeletons (feet on the ball bone...): merge
    # their columns so each vertex keeps four distinct influences.
    palette_cols, column = np.unique([base.bone_index(n) for n in names], return_inverse=True)
    merged = np.zeros((len(w), len(palette_cols)))
    for j, col in enumerate(column):
        merged[:, col] += w[:, j]
    if finger_weights:
        from src.core.fingers import add_fingers
        merged, palette_cols = add_fingers(baked, merged, palette_cols, base)
    idx, wq = quantize_weights(merged, palette_cols)
    log(f"  {len(baked)} vertices, {len(vfaces)} triangles, weights: {method}")
    if len(vfaces) > HEAVY_TRIANGLES:
        log(f"  WARNING: {len(vfaces)} triangles is heavy for this engine (typical characters: 3000-8000) and "
            "lowers the frame rate in game. Re-import with a Triangle budget (e.g. 8000).")
    return RigResult(base, tpose, baked, vfaces, vuv, bnormals, tangents, tsign, idx, wq, method, anchors)


def skin(verts, idx, wq, bind_world, posed_world):
    mats = np.array([np.linalg.inv(b) @ p for b, p in zip(bind_world, posed_world)])
    hom = np.c_[verts, np.ones(len(verts))]
    out = np.zeros_like(verts)
    for k in range(4):
        out += (wq[:, k:k + 1] / 255.0) * np.einsum("ni,nij->nj", hom, mats[idx[:, k]])[:, :3]
    return out


def _rotate_chain(base, world, pivot_bone, chain, rot3):
    posed = world.copy()
    for side, sx in SIDES:
        pivot = world[base.bone_index(f"{pivot_bone}_{side}")][3, :3]
        r = rot3(sx)
        rot = np.eye(4)
        rot[:3, :3] = r
        rot[3, :3] = pivot - pivot @ r
        for i, n in enumerate(base.bone_names):
            if n.endswith(side) and n.startswith(chain):
                posed[i] = world[i] @ rot
    return posed


def _arms_up(base, world, amount=1.0):
    def rz(sx):
        a = np.radians(70 * amount) * sx
        return np.array([[np.cos(a), np.sin(a), 0], [-np.sin(a), np.cos(a), 0], [0, 0, 1]])
    return _rotate_chain(base, world, "Shoulder", ("Shoulder", "Elbow", "Forearm", "Hand", "Thumb", "Index", "Middle"), rz)


def _walk(base, world, amount=1.0):
    def rx(sx):
        a = np.radians(35 * amount) * sx
        return np.array([[1, 0, 0], [0, np.cos(a), np.sin(a)], [0, -np.sin(a), np.cos(a)]])
    return _rotate_chain(base, world, "Thigh", ("Thigh", "Knee", "Ankle", "Ball", "Toes"), rx)


def _overhead(base, world, amount=1.0):
    """Arms raised above the head (150 degrees from hanging): the hardest case for shoulder skinning."""
    def rz(sx):
        a = np.radians(150 * amount) * sx
        return np.array([[np.cos(a), np.sin(a), 0], [-np.sin(a), np.cos(a), 0], [0, 0, 1]])
    return _rotate_chain(base, world, "Shoulder", ("Shoulder", "Elbow", "Forearm", "Hand", "Thumb", "Index", "Middle"), rz)


def _rx(deg):
    a = np.radians(deg)
    return np.array([[1, 0, 0], [0, np.cos(a), np.sin(a)], [0, -np.sin(a), np.cos(a)]])


def _reach(base, world, amount=1.0):
    """Arms swung straight forward (punch / grab)."""
    return _rotate_chain(base, world, "Shoulder", ("Shoulder", "Elbow", "Forearm", "Hand", "Thumb", "Index", "Middle"),
                         lambda sx: _rx(85 * amount))


def _elbow(base, world, amount=1.0):
    """Arms halfway forward with the elbows bent at a right angle (guard, carrying)."""
    arms = _rotate_chain(base, world, "Shoulder", ("Shoulder", "Elbow", "Forearm", "Hand", "Thumb", "Index", "Middle"),
                         lambda sx: _rx(60 * amount))
    return _rotate_chain(base, arms, "Elbow", ("Elbow", "Forearm", "Hand", "Thumb", "Index", "Middle"),
                         lambda sx: _rx(100 * amount))


def _twist(base, world, amount=1.0):
    """Torso turned 40 degrees around the spine (swing, throw): chest, arms and head turn, the hips stay."""
    a = np.radians(40 * amount)
    r = np.array([[np.cos(a), 0, -np.sin(a)], [0, 1, 0], [np.sin(a), 0, np.cos(a)]])
    pivot = world[base.bone_index("Spine2")][3, :3]
    rot = np.eye(4)
    rot[:3, :3] = r
    rot[3, :3] = pivot - pivot @ r
    posed = world.copy()
    upper = ("Spine3", "Spine4", "Neck", "Head", "Clavicle", "Shoulder", "Elbow", "Forearm", "Hand", "Thumb", "Index",
             "Middle")
    for i, n in enumerate(base.bone_names):
        if n.startswith(upper):
            posed[i] = world[i] @ rot
    return posed


def _fist(base, world, amount=1.0):
    """Fingers curled into a fist (grab, punch, blaster grip): shows how the hands bend."""
    from src.core.fingers import CHAINS

    posed = world.copy()
    for side, _ in SIDES:
        tips = {}
        for _, names in CHAINS:
            try:
                tips[names[0]] = [base.bone_index(f"{n}_{side}") for n in names]
            except ValueError:
                pass
        if "Index1" not in tips or "Thumb1" not in tips:
            continue
        palm = world[base.bone_index(f"Hand_{side}")][3, :3]
        d = world[tips["Index1"][0]][3, :3] - palm
        lateral = world[tips["Thumb1"][0]][3, :3] - world[tips["Index1"][0]][3, :3]
        axis = np.cross(d, lateral)
        axis /= max(np.linalg.norm(axis), 1e-9)
        # The curl direction that brings the thumb and index tips together is the closing one.
        for sign in (1.0, -1.0):
            probe = world.copy()
            for names in tips.values():
                for k, bone in enumerate(names):
                    probe = _curl(probe, names[k:], probe[bone][3, :3], axis * sign, 50 * amount)
            if (np.linalg.norm(probe[tips["Index1"][-1]][3, :3] - probe[tips["Thumb1"][-1]][3, :3])
                    < np.linalg.norm(world[tips["Index1"][-1]][3, :3] - world[tips["Thumb1"][-1]][3, :3])):
                break
        for names in tips.values():
            for k, bone in enumerate(names):
                posed = _curl(posed, names[k:], posed[bone][3, :3], axis * sign, 50 * amount)
    return posed


def _curl(posed, bones, pivot, axis, degrees):
    a = np.radians(degrees)
    c, s = np.cos(a), np.sin(a)
    x, y, z = axis
    r = np.array([[c + x * x * (1 - c), x * y * (1 - c) + z * s, x * z * (1 - c) - y * s],
                  [x * y * (1 - c) - z * s, c + y * y * (1 - c), y * z * (1 - c) + x * s],
                  [x * z * (1 - c) + y * s, y * z * (1 - c) - x * s, c + z * z * (1 - c)]])
    rot = np.eye(4)
    rot[:3, :3] = r
    rot[3, :3] = pivot - pivot @ r
    out = posed.copy()
    for bone in bones:
        out[bone] = posed[bone] @ rot
    return out


POSES = {"arms": _arms_up, "walk": _walk, "overhead": _overhead, "reach": _reach, "elbow": _elbow, "twist": _twist, "fist": _fist}


def base_tpose(base: BaseCharacter) -> tuple[np.ndarray, dict]:
    """The base figure with its arms raised to horizontal: (posed bone matrices, canonical joints).

    Only for comparing it with a T-posed model: the real rig always bakes into the native bind pose."""
    world = base.bind_world
    posed = world.copy()
    joints = {k: v.copy() for k, v in base.joints.items()}
    for side, sx in SIDES:
        shoulder = base.joints[f"Shoulder_{side}"]
        d = base.joints[f"Hand_{side}"] - shoulder
        angle = -np.arctan2(d[1], d[0] * sx) * sx
        r = np.array([[np.cos(angle), np.sin(angle), 0], [-np.sin(angle), np.cos(angle), 0], [0, 0, 1]])
        rot = np.eye(4)
        rot[:3, :3] = r
        rot[3, :3] = shoulder - shoulder @ r
        for i, name in enumerate(base.bone_names):
            if name.endswith(side) and name.startswith(("Shoulder", "Elbow", "Forearm", "Hand", "Thumb", "Index", "Middle")):
                posed[i] = world[i] @ rot
        for joint in (f"Elbow_{side}", f"Hand_{side}", f"Middle1_{side}", f"HandTip_{side}"):
            if joint in joints:
                joints[joint] = (joints[joint] - shoulder) @ r + shoulder
    return posed, joints
