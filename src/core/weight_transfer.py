"""Optional skin-weight transfer from the base figure's own body (Robust Weight Transfer).

The base figure's body mesh already carries game-quality weights. Instead of guessing weights with bone heat, the
body is brought into the model's T-pose space (inverse of the bake) and its weights are copied onto the model
where the surfaces agree, then inpainted elsewhere ("Robust Skin Weights Transfer via Weight Inpainting",
Abdrashitov et al., SIGGRAPH Asia 2023).

The algorithm comes from the *Robust Weight Transfer* Blender add-on (GPL, https://github.com/sentfromspacevr/
robust-weight-transfer). Its code is NOT bundled here: the user points Creator at the add-on folder and its
`weighttransfer.py` is loaded at run time. It also needs `libigl`, `robust-laplacian` and `scipy` in the Python
that runs Creator (pip install libigl robust-laplacian scipy).
"""
import importlib.util
import math
import os
from pathlib import Path
from typing import Callable

import numpy as np

from src.core.base_character import S1_STRIDE, BaseCharacter

Log = Callable[[str], None]
MAX_DISTANCE = 0.06  # fraction of the model height
MAX_NORMAL_ANGLE = 30.0  # degrees
MIN_MATCHED = 0.05  # below this share of matched vertices the transfer is not trusted


def default_addon_dirs() -> list[Path]:
    """Places where the add-on usually lives (Blender add-ons folders, Downloads)."""
    roots = []
    appdata = os.environ.get("APPDATA")
    if appdata:
        blender = Path(appdata) / "Blender Foundation" / "Blender"
        if blender.is_dir():
            for version in sorted(blender.iterdir(), reverse=True):
                roots += [version / "scripts" / "addons", version / "extensions" / "user_default"]
    roots.append(Path.home() / "Downloads")
    found = []
    for root in roots:
        if root.is_dir():
            for sub in root.glob("*"):
                if sub.is_dir() and (sub / "weighttransfer.py").is_file():
                    found.append(sub)
    return found


def find_addon(configured: str = "") -> Path | None:
    for candidate in ([Path(configured)] if configured else []) + default_addon_dirs():
        if (candidate / "weighttransfer.py").is_file():
            return candidate
    return None


def _barycentric_coordinates_tri(p, a, b, c):
    """Barycentric coordinates of points p in triangles (a, b, c) (rows), like igl.barycentric_coordinates_tri."""
    v0, v1, v2 = b - a, c - a, p - a
    d00, d01, d11 = (v0 * v0).sum(1), (v0 * v1).sum(1), (v1 * v1).sum(1)
    d20, d21 = (v2 * v0).sum(1), (v2 * v1).sum(1)
    den = d00 * d11 - d01 * d01
    den = np.where(np.abs(den) < 1e-20, 1e-20, den)
    v = (d11 * d20 - d01 * d21) / den
    w = (d00 * d21 - d01 * d20) / den
    return np.stack([1.0 - v - w, v, w], axis=1)


def load_module(addon_dir: Path):
    """Import the add-on's pure-numpy `weighttransfer.py` (no Blender needed). Raises ImportError if deps are missing."""
    import igl

    # The add-on pins libigl 2.5.1; newer builds renamed/changed two calls it uses. Adapt them so any libigl works.
    if not hasattr(igl, "barycentric_coordinates_tri"):
        igl.barycentric_coordinates_tri = _barycentric_coordinates_tri
    if not getattr(igl.min_quad_with_fixed, "_creator_wrapped", False):
        original = igl.min_quad_with_fixed

        def min_quad_with_fixed(A, B, known, Y, Aeq, Beq, pd):
            try:
                return original(A, B, known, Y, Aeq, Beq, pd)
            except TypeError:  # libigl >= 2.6: returns the solution only, Beq is a matrix
                return True, original(A, B, known, Y, Aeq, np.zeros((0, 0)), pd)

        min_quad_with_fixed._creator_wrapped = True
        igl.min_quad_with_fixed = min_quad_with_fixed
    spec = importlib.util.spec_from_file_location("rwt_weighttransfer", Path(addon_dir) / "weighttransfer.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def availability(configured: str = "") -> tuple[bool, str]:
    """(usable, short message) for the settings panel."""
    addon = find_addon(configured)
    if addon is None:
        return False, "add-on folder not found (it must contain weighttransfer.py)"
    try:
        load_module(addon)
    except ImportError as exc:
        return False, f"missing Python package ({exc.name}): pip install libigl robust-laplacian scipy"
    except Exception as exc:  # noqa: BLE001
        return False, f"add-on could not be loaded ({exc})"
    return True, f"ready ({addon.name})"


def _canonical_columns(base: BaseCharacter, names: list[str]) -> np.ndarray:
    """Canonical bone column (index into `names`) for every palette bone: its own, else the nearest segment."""
    from src.core.skeleton import heat_bones

    joints = base.joints
    seg = {}
    for bone, head, tail, _ in heat_bones():
        h = joints.get(head if head in joints else bone)
        t = joints.get(tail if tail in joints else bone)
        if h is not None and t is not None:
            seg[bone] = (np.asarray(h, float), np.asarray(t, float))
    own = {}
    for n in names:
        try:
            own[base.bone_index(n)] = names.index(n)
        except ValueError:
            pass
    positions = base.bind_world[:, 3, :3]
    out = np.zeros(len(base.bone_names), int)
    for i, p in enumerate(positions):
        if i in own:
            out[i] = own[i]
            continue
        best, best_d = 0, 1e18
        for bone, (a, b) in seg.items():
            ab = b - a
            t = np.clip(np.dot(p - a, ab) / max(np.dot(ab, ab), 1e-12), 0, 1)
            d = np.linalg.norm(p - (a + t * ab))
            if d < best_d:
                best, best_d = names.index(bone), d
        out[i] = best
    return out


def donor_mesh(base: BaseCharacter, anchors: dict, names: list[str]):
    """The base body in the model's T-pose space, with weights over the canonical bones `names`.

    Returns (vertices, triangles, normals, weights (V, len(names)))."""
    from src.core.rig import bake_transforms, vertex_normals

    verts, tris, _ = base.body_mesh
    prim = base.body_primitive
    n = prim.vertex_count
    s1 = np.frombuffer(base._vbuf[prim.s1_offset:prim.s1_offset + n * S1_STRIDE], np.uint8).reshape(n, S1_STRIDE)
    idx, wq = s1[:, 0:4].astype(int), s1[:, 4:8] / 255.0
    cols = _canonical_columns(base, names)
    weights = np.zeros((n, len(names)))
    for k in range(4):
        np.add.at(weights, (np.arange(n), cols[idx[:, k]]), wq[:, k])
    weights /= np.maximum(weights.sum(1, keepdims=True), 1e-9)

    bake = bake_transforms(anchors, base.joints)
    back = np.zeros_like(verts, dtype=float)
    for col, bone in enumerate(names):
        m, t = bake[bone]
        back += weights[:, col:col + 1] * ((verts - t) @ np.linalg.inv(m))
    return back, np.asarray(tris), vertex_normals(back, np.asarray(tris)), weights


def transfer(wverts, wfaces, base: BaseCharacter, anchors: dict, names: list[str], addon_dir: Path,
             log: Log) -> np.ndarray | None:
    """Weights (len(wverts), len(names)) copied from the base body, or None if the transfer is not trustworthy."""
    from src.core.rig import vertex_normals

    rwt = load_module(addon_dir)
    dv, dt, dn, dw = donor_mesh(base, anchors, names)
    height = float(wverts[:, 1].max() - wverts[:, 1].min())
    normals = vertex_normals(wverts, wfaces)
    matched, copied = rwt.find_matches_closest_surface(
        dv.astype(np.float32), dt.astype(np.int64), dn.astype(np.float32), wverts.astype(np.float32),
        normals.astype(np.float32), dw.astype(np.float32), (MAX_DISTANCE * height) ** 2, MAX_NORMAL_ANGLE, True)
    share = float(matched.mean())
    log(f"  Robust transfer: {100 * share:.0f}% of the vertices match the base body")
    if share < MIN_MATCHED:
        log("  too few matches -> not used")
        return None
    ok, weights = rwt.inpaint(wverts.astype(np.float64), wfaces.astype(np.int64), copied, matched, True)
    if not ok:
        log("  inpainting failed (loose parts without any match?) -> not used")
        return None
    weights = np.clip(np.asarray(weights, float), 0, None)
    total = weights.sum(1, keepdims=True)
    bad = total[:, 0] < 1e-6
    if bad.any():
        weights[bad] = copied[bad]
        total = weights.sum(1, keepdims=True)
    return weights / np.maximum(total, 1e-9)
