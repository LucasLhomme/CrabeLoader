"""Skinning weights for the custom mesh against the fitted joints."""
import subprocess
from pathlib import Path
from typing import Callable

import numpy as np
from scipy.sparse import coo_matrix
from scipy.sparse.csgraph import connected_components
from scipy.spatial import cKDTree

from src.core.paths import HEAT_SCRIPT
from src.core.skeleton import SIDES, heat_bones

Log = Callable[[str], None]


def weld(positions: np.ndarray, faces: np.ndarray, tolerance: float = 1e-5):
    """Merge coincident corners (UV seams) so weights are computed on one connected surface."""
    keys = np.round(positions / tolerance).astype(np.int64)
    _, first, inverse = np.unique(keys, axis=0, return_index=True, return_inverse=True)
    inverse = inverse.reshape(-1)
    wfaces = inverse[faces]
    ok = (wfaces[:, 0] != wfaces[:, 1]) & (wfaces[:, 1] != wfaces[:, 2]) & (wfaces[:, 0] != wfaces[:, 2])
    return positions[first], wfaces, inverse, ok


def blender_heat(wverts, wfaces, anchors, blender: Path, work: Path, log: Log) -> np.ndarray:
    bones = heat_bones()
    work.mkdir(parents=True, exist_ok=True)
    src, dst = work / "heat_in.npz", work / "heat_out.npz"
    dst.unlink(missing_ok=True)
    np.savez(src, verts=wverts, faces=wfaces,
             bone_names=np.array([b[0] for b in bones]),
             heads=np.array([anchors[b[1]] for b in bones]),
             tails=np.array([anchors[b[2]] for b in bones]),
             parents=np.array([b[3] for b in bones]))
    res = subprocess.run([str(blender), "-b", "--factory-startup", "--python", str(HEAT_SCRIPT), "--", str(src), str(dst)],
                         capture_output=True, text=True, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    for line in (res.stdout + res.stderr).splitlines():
        if "[heat]" in line or "failed" in line.lower() or "Error" in line:
            log(f"  blender: {line.strip()}")
    if res.returncode != 0 or not dst.exists():
        raise RuntimeError(f"Blender failed (exit code {res.returncode})")
    return np.load(dst)["weights"].astype(float)


def distance_weights(wverts, anchors) -> np.ndarray:
    """Fallback without Blender: each vertex follows the bones closest to it (inverse distance to the segment)."""
    bones = heat_bones()
    d = np.zeros((len(wverts), len(bones)))
    for j, (_, head, tail, _) in enumerate(bones):
        a, b = anchors[head], anchors[tail]
        ab = b - a
        t = np.clip(((wverts - a) @ ab) / max(ab @ ab, 1e-12), 0, 1)
        d[:, j] = np.linalg.norm(wverts - (a + t[:, None] * ab), axis=1)

    # Topological zoning to avoid cross-body bleed:
    shoulder_x = abs(anchors["Shoulder_Left"][0])
    crotch_y = anchors["HipsLow"][1]
    neck_y = anchors["Neck"][1]

    for j, (bname, _, _, _) in enumerate(bones):
        is_arm = bname.startswith(("Clavicle_", "Shoulder_", "Elbow_", "Hand_"))
        is_leg = bname.startswith(("Thigh_", "Knee_", "Ankle_", "Ball_"))
        is_head = bname in ("Neck", "Head")

        # Head bones shouldn't reach below chest
        if is_head:
            d[wverts[:, 1] < crotch_y + 0.15 * (neck_y - crotch_y), j] += 10.0
        # Leg bones shouldn't reach above chest
        if is_leg:
            d[wverts[:, 1] > neck_y, j] += 10.0
        # Arm bones shouldn't reach lower legs
        if is_arm:
            d[wverts[:, 1] < crotch_y - 0.1 * (neck_y - crotch_y), j] += 10.0
        # Left arm shouldn't reach far right side
        if is_arm and bname.endswith("_Left"):
            d[wverts[:, 0] < -0.05 * shoulder_x, j] += 10.0
        # Right arm shouldn't reach far left side
        if is_arm and bname.endswith("_Right"):
            d[wverts[:, 0] > 0.05 * shoulder_x, j] += 10.0

    nearest = d.min(1, keepdims=True)
    w = np.where(d <= nearest * 1.5 + 1e-6, 1.0 / (d + 1e-4) ** 4, 0.0)
    return w / w.sum(1, keepdims=True)


def fill_islands(wverts, weights, log: Log) -> np.ndarray:
    empty = weights.sum(1) <= 1e-6
    if empty.any() and (~empty).any():
        _, nearest = cKDTree(wverts[~empty]).query(wverts[empty])
        weights[empty] = weights[~empty][nearest]
        log(f"  {empty.sum()} isolated vertices (eyes, accessories) copied from the nearest weighted surface")
    return weights


def rigid_head(wverts, weights, names, anchors, log: Log) -> np.ndarray:
    shoulder_x = abs(anchors["Shoulder_Left"][0])
    neck_y = anchors["Neck"][1]
    head_y = anchors["Head"][1]
    head_top = anchors.get("HeadTop", np.array([0, head_y + 0.2, 0]))[1]
    # The head is rigid: everything above the head joint (wide hair, hats, ears), and between the neck line and the
    # head joint only what lies over the neck (chin, jaw, back of the skull). The band beside the neck is the
    # collar, the hood and the top of the shoulders: made rigid with the head, they stayed level when the arms come
    # down in the game pose (square shoulders) and moved with every head turn.
    head_span = head_top - neck_y
    wide = np.abs(wverts[:, 0]) < max(shoulder_x * 1.8, 0.45 * head_span)
    narrow = np.abs(wverts[:, 0]) < max(0.6 * shoulder_x, 0.35 * head_span)
    above = ((wverts[:, 1] >= head_y) & wide) | ((wverts[:, 1] >= neck_y - 0.02 * (head_y - neck_y)) & narrow)
    weights[above] = 0.0
    weights[above, names.index("Head")] = 1.0
    log(f"  {above.sum()} vertices of the head made rigid (face, eyes and hat stay 100% intact)")
    return weights


def isolate_sides_and_feet(verts, faces, weights, names, anchors, log: Log) -> np.ndarray:
    """Loose pieces below the knees follow one ankle rigidly; everything else drops opposite-side bones."""
    n = len(verts)
    rows = np.concatenate([faces[:, 0], faces[:, 1], faces[:, 2]])
    cols = np.concatenate([faces[:, 1], faces[:, 2], faces[:, 0]])
    _, label = connected_components(coo_matrix((np.ones(len(rows)), (rows, cols)), shape=(n, n)), directed=False)
    original = weights.copy()
    margin = 0.01 * verts[:, 1].max()
    side_of = np.where(verts[:, 0] > margin, "Left", np.where(verts[:, 0] < -margin, "Right", ""))
    knee_y = min(anchors["Knee_Left"][1], anchors["Knee_Right"][1])
    rigid = np.zeros(n, bool)
    for comp in np.unique(label):
        members = label == comp
        if verts[members, 1].max() >= knee_y:
            continue
        side = "Left" if verts[members, 0].mean() > 0 else "Right"
        side_of[members] = side
        weights[members] = 0.0
        weights[members, names.index(f"Ankle_{side}")] = 1.0
        rigid |= members
    for side, _ in SIDES:
        other = "Right" if side == "Left" else "Left"
        mask = (side_of == side) & ~rigid
        weights[np.ix_(mask, [j for j, name in enumerate(names) if name.endswith(other)])] = 0.0
    totals = weights.sum(1, keepdims=True)
    weights = np.where(totals > 1e-6, weights / np.maximum(totals, 1e-6), original)
    blended = _blend_piece_seams(verts, weights, rigid, side_of)
    log(f"  {rigid.sum()} vertices of loose pieces below the knees bound to their ankle "
        f"({blended} near the trouser hem blended with it); cross-side weights removed")
    return weights


SEAM_REACH = 0.04
"""Height share over which a loose foot piece follows its neighbour (the trouser hem) before turning rigid."""


def _blend_piece_seams(verts, weights, rigid, side_of) -> int:
    """A shoe 100% on the ankle next to a hem skinned to the shin comes apart when the ankle turns.
    Shoe vertices close to the other surface copy that surface's weights, fading to rigid within SEAM_REACH."""
    from scipy.spatial import cKDTree
    if not rigid.any() or rigid.all():
        return 0
    reach = SEAM_REACH * float(verts[:, 1].max())
    changed = 0
    for side in ("Left", "Right"):
        piece = rigid & (side_of == side)
        other = ~rigid & (side_of == side)
        if not piece.any() or not other.any():
            continue
        dist, idx = cKDTree(verts[other]).query(verts[piece])
        t = np.clip(dist / reach, 0.0, 1.0)[:, None]
        theirs = weights[other][idx]
        rows = np.flatnonzero(piece)
        weights[rows] = (1 - t) * theirs + t * weights[rows]
        changed += int((t[:, 0] < 1).sum())
    return changed


ARM_BONES = ("Clavicle_", "Shoulder_", "Elbow_", "Hand_")
LEG_BONES = ("Thigh_", "Knee_", "Ankle_", "Ball_")
TORSO_CHAIN = ("Hips", "Spine2", "Spine3", "Neck", "Head")
MAX_UNWEIGHTED = 0.30
"""Above this share of unweighted vertices the Blender result is discarded."""
ISLAND_SHARE = 0.03
"""Up to this share, unweighted vertices are islands (eyes, accessories) copied from the nearest surface."""
SMOOTH_ITERATIONS = 12


def _components(faces: np.ndarray, n: int) -> tuple[np.ndarray, np.ndarray]:
    rows = np.concatenate([faces[:, 0], faces[:, 1], faces[:, 2]])
    cols = np.concatenate([faces[:, 1], faces[:, 2], faces[:, 0]])
    _, label = connected_components(coo_matrix((np.ones(len(rows)), (rows, cols)), shape=(n, n)), directed=False)
    return label, np.bincount(label)


def torso_height_weights(y: np.ndarray, names, anchors) -> np.ndarray:
    """Weights over the spine chain only (Hips, Spine2, Spine3, Neck, Head), blended by height between joints."""
    chain = [(b[0], anchors[b[1]][1]) for b in heat_bones() if b[0] in TORSO_CHAIN and b[1] in anchors]
    chain.sort(key=lambda c: c[1])
    heights = np.array([c[1] for c in chain])
    out = np.zeros((len(y), len(names)))
    pos = np.clip(np.interp(y, heights, np.arange(len(chain))), 0, len(chain) - 1)
    lo = np.floor(pos).astype(int)
    hi = np.minimum(lo + 1, len(chain) - 1)
    t = pos - lo
    for k, (bone, _) in enumerate(chain):
        col = names.index(bone)
        out[:, col] += np.where(lo == k, 1 - t, 0.0) + np.where((hi == k) & (hi != lo), t, 0.0)
    return out


def rigid_bridging_pieces(verts, faces, weights, names, anchors, log: Log) -> np.ndarray:
    """Loose pieces that link the arms AND the legs (wings, ribbons, long sleeves) follow the spine only.

    Left to the distance/heat weights they get a bit of clavicle at the top and a bit of thigh at the bottom, and
    stretch between shoulder and hip as soon as an arm or a leg moves. Their weights are replaced by a blend of the
    spine bones chosen by height, so they sit exactly where the baking puts the torso."""
    label, sizes = _components(faces, len(verts))
    arm = [j for j, b in enumerate(names) if b.startswith(ARM_BONES)]
    leg = [j for j, b in enumerate(names) if b.startswith(LEG_BONES)]
    if not (arm and leg) or not all(c in names for c in TORSO_CHAIN):
        return weights
    moved = 0
    for comp in np.unique(label):
        if comp == sizes.argmax() or sizes[comp] < 20:
            continue
        members = label == comp
        total = weights[members].sum()
        if total <= 0:
            continue
        if weights[members][:, arm].sum() / total < 0.05 or weights[members][:, leg].sum() / total < 0.05:
            continue
        weights[members] = torso_height_weights(verts[members, 1], names, anchors)
        moved += int(members.sum())
    if moved:
        log(f"  {moved} vertices of pieces linking arms and legs follow the spine only")
    return weights


def smooth_leg_spanning_panels(verts, faces, weights, names, log: Log, iterations: int = SMOOTH_ITERATIONS) -> np.ndarray:
    """Loose panels that straddle both legs (skirt front/back, robe flaps): smooth their weights over the surface.

    Without it the left half follows the left thigh and the right half the right thigh with a hard seam in the
    middle, which tears the cloth open when the legs move apart."""
    n = len(verts)
    rows = np.concatenate([faces[:, 0], faces[:, 1], faces[:, 2], faces[:, 1], faces[:, 2], faces[:, 0]])
    cols = np.concatenate([faces[:, 1], faces[:, 2], faces[:, 0], faces[:, 0], faces[:, 1], faces[:, 2]])
    adj = coo_matrix((np.ones(len(rows)), (rows, cols)), shape=(n, n)).tocsr()
    adj.data[:] = 1.0
    label, sizes = _components(faces, n)
    if "Thigh_Left" not in names or "Thigh_Right" not in names:
        return weights
    left, right = names.index("Thigh_Left"), names.index("Thigh_Right")
    smoothed = 0
    for comp in np.unique(label):
        if comp == sizes.argmax() or sizes[comp] < 50:
            continue
        members = label == comp
        total = weights[members].sum()
        if total <= 0 or weights[members][:, left].sum() / total < 0.05 or weights[members][:, right].sum() / total < 0.05:
            continue
        idx = np.nonzero(members)[0]
        sub = adj[idx][:, idx]
        degree = np.maximum(np.asarray(sub.sum(1)).ravel()[:, None], 1)
        w = weights[idx].copy()
        for _ in range(iterations):
            w = 0.5 * w + 0.5 * (sub @ w) / degree
        weights[idx] = w / np.maximum(w.sum(1, keepdims=True), 1e-9)
        smoothed += len(idx)
    if smoothed:
        log(f"  {smoothed} vertices of leg-spanning panels: weights smoothed")
    return weights


def apply_piece_overrides(verts, faces, weights, names, overrides, log: Log) -> np.ndarray:
    """User choices: [(point, bone)] -> the connected piece nearest to each point follows that bone rigidly."""
    if not overrides:
        return weights
    n = len(verts)
    label, _ = _components(faces, n)
    used = np.zeros(n, bool)
    used[np.unique(faces)] = True
    for point, bone in overrides:
        if bone not in names:
            log(f"  piece override ignored: unknown bone {bone}")
            continue
        d = np.linalg.norm(verts - np.asarray(point, float), axis=1)
        d[~used] = np.inf
        members = label == label[int(np.argmin(d))]
        weights[members] = 0.0
        weights[members, names.index(bone)] = 1.0
        log(f"  piece of {int(members.sum())} vertices locked to {bone}")
    return weights


def compute_weights(wverts, wfaces, anchors, blender: Path | None, work: Path, log: Log,
                    overrides=None, transfer=None, accessory=None) -> tuple[list[str], np.ndarray, str]:
    """`transfer`: optional callable (wverts, wfaces, names) -> weights or None (copy from the base body)."""
    names = [b[0] for b in heat_bones()]
    method = "distance"
    weights = None
    if transfer is not None:
        try:
            weights = transfer(wverts, wfaces, names)
        except Exception as exc:  # noqa: BLE001 - optional feature: never block the import
            log(f"  Robust transfer failed ({exc}) -> bone heat instead")
        if weights is not None:
            method = "robust transfer"
    if weights is None and blender and Path(blender).is_file():
        try:
            log("Computing bone-heat weights in Blender...")
            weights = blender_heat(wverts, wfaces, anchors, Path(blender), work, log)
            method = "blender"
            empty = weights.sum(1) <= 1e-6
            share = float(empty.mean())
            if share > MAX_UNWEIGHTED:
                # Bone heat gives up on open or many-piece meshes: it then weights nothing and the model would collapse.
                log(f"  Blender weighted only {100 * (1 - share):.0f}% of the vertices -> using distance weights instead")
                weights, method = None, "distance"
            elif share > ISLAND_SHARE:
                log(f"  Blender left {100 * share:.0f}% of the vertices unweighted -> distance weights for those")
                weights[empty] = distance_weights(wverts, anchors)[empty]
        except (RuntimeError, OSError) as exc:
            log(f"  {exc} -> falling back to distance weights")
    elif weights is None:
        log("Blender not found: using distance-to-bone weights (less accurate)")
    if weights is None:
        weights = distance_weights(wverts, anchors)
    weights = fill_islands(wverts, weights, log)
    weights = rigid_head(wverts, weights, names, anchors, log)
    weights = isolate_sides_and_feet(wverts, wfaces, weights, names, anchors, log)
    weights = rigid_bridging_pieces(wverts, wfaces, weights, names, anchors, log)
    weights = smooth_leg_spanning_panels(wverts, wfaces, weights, names, log)
    if accessory is not None and accessory.any():
        from src.core.accessories import accessory_weights
        weights = accessory_weights(wverts, weights, accessory, names)
        log(f"  {int(accessory.sum())} accessory vertices follow the torso")
    weights = apply_piece_overrides(wverts, wfaces, weights, names, overrides, log)
    return names, weights, method


def quantize_weights(weights: np.ndarray, bone_cols: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Top 4 influences per vertex as bone indices + weights summing to exactly 255."""
    order = np.argsort(-weights, axis=1)[:, :4]
    top = np.take_along_axis(weights, order, 1)
    top = top / np.maximum(top.sum(1, keepdims=True), 1e-12)
    q = np.floor(top * 255).astype(int)
    rem = 255 - q.sum(1)
    frac = top * 255 - q
    for i in np.nonzero(rem)[0]:
        for j in np.argsort(-frac[i])[:rem[i]]:
            q[i, j] += 1
    idx = bone_cols[order]
    idx[q == 0] = idx[:, :1].repeat(4, 1)[q == 0]
    return idx.astype(np.uint8), q.astype(np.uint8)
