"""Skin-weight transfer from the base figure's own body (robust weight inpainting).

The base figure's body mesh already carries game-quality weights. Instead of guessing weights with bone heat, the
body is brought into the model's T-pose space (inverse of the bake) and its weights are copied onto the model
where the surfaces agree (close, similar normal), then smoothly inpainted everywhere else.

Method: "Robust Skin Weights Transfer via Weight Inpainting" (Abdrashitov, Raichstat, Monsen, Hill, SIGGRAPH Asia
2023; reference code MIT, https://github.com/rin-23/RobustSkinWeightsTransferCode). This is a NumPy/SciPy
re-implementation with no compiled dependency (no libigl, no robust-laplacian, no Blender add-on): the inpainting
minimises a bi-Laplacian energy on a nearest-neighbour graph of the vertices, which also bridges loose parts.
"""
import math
from typing import Callable

import numpy as np

from src.core.base_character import S1_STRIDE, BaseCharacter

Log = Callable[[str], None]
MAX_DISTANCE = 0.06  # fraction of the model height
MAX_NORMAL_ANGLE = 30.0  # degrees
MIN_MATCHED = 0.05  # below this share of matched vertices the transfer is not trusted


def availability(configured: str = "") -> tuple[bool, str]:
    """(usable, short message) for the settings panel."""
    try:
        import scipy.sparse.linalg  # noqa: F401
        import scipy.spatial  # noqa: F401
    except ImportError:
        return False, "SciPy is missing: pip install scipy"
    return True, "ready"


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


K_NEIGHBOURS = 10


def _graph_laplacian(points: np.ndarray, known: np.ndarray, k: int = K_NEIGHBOURS):
    """Gaussian-weighted k-nearest-neighbour graph Laplacian (symmetric, PSD) of a point set."""
    from scipy.sparse import coo_matrix, diags
    from scipy.sparse.csgraph import connected_components
    from scipy.spatial import cKDTree

    k = min(k + 1, len(points))
    dist, idx = cKDTree(points).query(points, k=k)
    sigma = max(float(np.median(dist[:, 1:])), 1e-9)
    rows = np.repeat(np.arange(len(points)), k - 1)
    cols = idx[:, 1:].reshape(-1)
    w = np.exp(-(dist[:, 1:].reshape(-1) / sigma) ** 2)
    adjacency = coo_matrix((w, (rows, cols)), shape=(len(points),) * 2).tocsr()
    adjacency = adjacency.maximum(adjacency.T).tolil()
    # A loose piece far from the body forms its own island: link it to the closest known vertex, otherwise it
    # would get no weights at all.
    count, label = connected_components(adjacency, directed=False)
    if known.any():
        known_points, known_index = points[known], np.flatnonzero(known)
        tree = cKDTree(known_points)
        for c in range(count):
            members = np.flatnonzero(label == c)
            if known[members].any():
                continue
            d, j = tree.query(points[members])
            best = int(np.argmin(d))
            adjacency[members[best], known_index[j[best]]] = 1.0
            adjacency[known_index[j[best]], members[best]] = 1.0
    adjacency = adjacency.tocsr()
    return diags(np.asarray(adjacency.sum(1)).ravel()) - adjacency


def inpaint(points: np.ndarray, weights: np.ndarray, known: np.ndarray) -> np.ndarray | None:
    """Fill the rows of `weights` where `known` is False by minimising a bi-Laplacian energy (known rows fixed)."""
    from scipy.sparse import csc_matrix, diags
    from scipy.sparse.linalg import splu

    laplacian = _graph_laplacian(points, known)
    inv_mass = diags(1.0 / np.maximum(laplacian.diagonal(), 1e-9))
    energy = csc_matrix(laplacian + laplacian @ inv_mass @ laplacian)
    free = ~known
    if not free.any():
        return weights
    q_ff = energy[free][:, free]
    q_fk = energy[free][:, known]
    try:
        solved = splu(csc_matrix(q_ff + 1e-9 * diags(np.ones(q_ff.shape[0])))).solve(-(q_fk @ weights[known]))
    except RuntimeError:
        return None
    out = weights.copy()
    out[free] = solved
    return out


def transfer(wverts, wfaces, base: BaseCharacter, anchors: dict, names: list[str], log: Log) -> np.ndarray | None:
    """Weights (len(wverts), len(names)) copied from the base body, or None if the transfer is not trustworthy."""
    from scipy.spatial import cKDTree

    from src.core.rig import vertex_normals

    dv, dt, dn, dw = donor_mesh(base, anchors, names)
    height = float(wverts[:, 1].max() - wverts[:, 1].min())
    normals = vertex_normals(wverts, wfaces)
    distance, nearest = cKDTree(dv).query(wverts)
    cosine = np.einsum("ij,ij->i", normals, dn[nearest])
    # A normal pointing the opposite way still matches: thin or double-sided garments are common.
    matched = (distance <= MAX_DISTANCE * height) & (np.abs(cosine) >= math.cos(math.radians(MAX_NORMAL_ANGLE)))
    share = float(matched.mean())
    log(f"  Robust transfer: {100 * share:.0f}% of the vertices match the base body")
    if share < MIN_MATCHED:
        log("  too few matches -> not used")
        return None
    copied = dw[nearest]
    weights = inpaint(wverts, copied, matched)
    if weights is None:
        log("  inpainting failed -> not used")
        return None
    weights = np.clip(weights, 0, None)
    total = weights.sum(1, keepdims=True)
    bad = total[:, 0] < 1e-6
    if bad.any():
        weights[bad] = copied[bad]
        total = weights.sum(1, keepdims=True)
    return weights / np.maximum(total, 1e-9)
