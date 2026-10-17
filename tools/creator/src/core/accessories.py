"""Accessories: parts of a model that are not the body (cape, tail, wings, weapon, long hair strands).

They disturb the measurements (a cape hanging behind the hips fakes a wide pelvis and a deep torso) and the skin
weights (the legs would drag it). Two ways to find them: a depth zone behind the body (works when the cape is welded
to the body, as on most game rips) and whole pieces that stick out. The user decides what to do with them:
  ignored  - kept in the model, left out of the measurements, and skinned to follow the torso instead of the legs;
  removed  - left out of the model."""
from dataclasses import dataclass

import numpy as np

SLABS = 60
ZONE_LIMIT = 0.2
"""Default depth behind the front surface (fraction of the height) beyond which a vertex counts as an accessory."""
FOLLOW_TORSO = {"Thigh": "Hips", "Knee": "Hips", "Ankle": "Hips", "Ball": "Hips",
                "Clavicle": "Spine3", "Shoulder": "Spine3", "Elbow": "Spine3", "Hand": "Spine3"}


@dataclass
class Finding:
    kind: str
    mask: np.ndarray
    text: str


def strip(vertices: np.ndarray, faces: np.ndarray, mask):
    """(vertices, faces) without the vertices of `mask` (and the faces that use them)."""
    if mask is None or not np.any(mask):
        return vertices, faces
    keep = ~np.asarray(mask, bool)[faces].any(axis=1)
    faces = faces[keep]
    used = np.unique(faces)
    remap = np.full(len(vertices), -1)
    remap[used] = np.arange(len(used))
    return vertices[used], remap[faces]


def back_zone(vertices: np.ndarray, limit: float = ZONE_LIMIT) -> np.ndarray:
    """Vertices further behind the body's front surface than `limit` x height (cape, tail, wings, backpack)."""
    v = np.asarray(vertices, float)
    h = float(v[:, 1].max())
    if h <= 0:
        return np.zeros(len(v), bool)
    slab = np.minimum((v[:, 1] / h * SLABS).astype(int), SLABS - 1)
    front = np.full(SLABS, np.nan)
    for k in range(SLABS):
        p = v[(slab == k) & (np.abs(v[:, 0]) < 0.15 * h)]
        if len(p) > 5:
            front[k] = np.percentile(p[:, 2], 95)
    ok = ~np.isnan(front)
    if not ok.any():
        return np.zeros(len(v), bool)
    front = np.interp(np.arange(SLABS), np.flatnonzero(ok), front[ok])
    return v[:, 2] < front[slab] - limit * h


def outlying_pieces(vertices: np.ndarray, labels: np.ndarray) -> list[Finding]:
    """Whole pieces far from the body: a weapon held out in front, something floating above the head."""
    v = np.asarray(vertices, float)
    h = float(v[:, 1].max())
    out = []
    sizes = np.bincount(labels)
    if len(sizes) < 2:
        return out
    main = int(np.argmax(sizes))
    centre = v[labels == main].mean(axis=0)
    for lab in range(len(sizes)):
        if lab == main or sizes[lab] < 20:
            continue
        p = v[labels == lab]
        lo, hi = p.min(0), p.max(0)
        reach = max(abs(hi[2] - centre[2]), abs(lo[2] - centre[2])) / h
        above = (hi[1] - h * 1.02) / h
        thin_sheet = sorted((hi - lo) / h)[0] < 0.01 and (hi - lo).max() / h > 0.25
        if reach > 0.35 or above > 0.0 or thin_sheet:
            out.append(Finding("piece", labels == lab, f"separate piece of {len(p)} vertices sticking out of the body"))
    return out


def detect(vertices: np.ndarray, labels: np.ndarray, limit: float = ZONE_LIMIT) -> list[Finding]:
    findings = []
    zone = back_zone(vertices, limit)
    if zone.any():
        findings.append(Finding("zone", zone, f"{100 * zone.mean():.1f}% of the vertices lie far behind the body "
                                              "(cape, tail, wings or backpack?)"))
    findings += outlying_pieces(vertices, labels)
    return findings


def accessory_weights(verts: np.ndarray, weights: np.ndarray, mask: np.ndarray, names: list[str]) -> np.ndarray:
    """Accessory vertices take the weights of the nearest body vertex, with the legs and arms folded into the torso:
    a cape follows the hips and chest instead of being torn by the legs and the arms."""
    from scipy.spatial import cKDTree

    mask = np.asarray(mask, bool)
    if not mask.any() or mask.all():
        return weights
    out = weights.copy()
    _, nearest = cKDTree(verts[~mask]).query(verts[mask])
    taken = weights[~mask][nearest]
    folded = np.zeros_like(taken)
    for col, name in enumerate(names):
        target = FOLLOW_TORSO.get(name.split("_")[0], name)
        folded[:, names.index(target) if target in names else col] += taken[:, col]
    out[mask] = folded / np.maximum(folded.sum(1, keepdims=True), 1e-9)
    return out
