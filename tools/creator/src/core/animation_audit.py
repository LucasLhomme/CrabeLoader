"""Audit of a built rig against every animation clip: where and when the mesh stretches.

Each clip is sampled frame by frame; the posed mesh is compared with the rest (baked) mesh edge by edge, like the
static score does. The result names, per clip, the worst frame and the body regions that suffer."""
from dataclasses import dataclass, field

import numpy as np

from src.core.animation_player import follow_hands
from src.core.quality import BROKEN_RATIO, STRETCH_LIMIT, MIN_REST_LENGTH, unique_edges
from src.core.rig import skin

REGIONS = (("Shoulder", "shoulder"), ("Clavicle", "shoulder"), ("Elbow", "elbow"), ("Hand", "hand"),
           ("Wrist", "hand"), ("Finger", "hand"), ("Thumb", "hand"), ("Index", "hand"), ("Middle", "hand"),
           ("Neck", "neck"), ("Head", "head"), ("Spine", "torso"), ("Hips", "hips"), ("Thigh", "leg"),
           ("Knee", "leg"), ("Ankle", "foot"), ("Foot", "foot"), ("Toe", "foot"))


def region_of_bone(name: str) -> str:
    for key, region in REGIONS:
        if key.lower() in name.lower():
            return region
    return "other"


@dataclass
class ClipAudit:
    name: str
    frames: int
    worst_frame: int
    worst_ratio: float
    broken_share: float
    """Share of edges changed more than 2x at the worst frame."""
    torn_share: float
    regions: dict = field(default_factory=dict)
    """Region -> broken edges at the worst frame."""


def dominant_regions(rig) -> np.ndarray:
    """Region name of the heaviest bone of every vertex."""
    names = rig.base.bone_names
    top = np.argmax(rig.bone_w, axis=1)
    bones = rig.bone_idx[np.arange(len(top)), top]
    return np.array([region_of_bone(names[b]) for b in bones])


def pose_world(clip, base, t: float) -> np.ndarray:
    world = base.bind_world
    if clip.evaluate_world is not None:
        return follow_hands(base, world, clip.evaluate_world(base, world, t))
    return world


def audit_clip(rig, clip, edges=None, regions=None, step: int = 2) -> ClipAudit:
    base = rig.base
    edges = unique_edges(rig.faces) if edges is None else edges
    regions = dominant_regions(rig) if regions is None else regions
    rest = rig.baked
    l0 = np.linalg.norm(rest[edges[:, 0]] - rest[edges[:, 1]], axis=1)
    usable = l0 > MIN_REST_LENGTH
    best = None
    total = clip.total_frames
    for frame in list(range(0, total, max(1, step))) + [total - 1]:
        t = min(clip.duration, frame / clip.fps)
        posed = skin(rest, rig.bone_idx, rig.bone_w, base.bind_world, pose_world(clip, base, t))
        l1 = np.linalg.norm(posed[edges[:, 0]] - posed[edges[:, 1]], axis=1)
        ratio = np.where(usable, l1 / np.maximum(l0, 1e-9), 1.0)
        broken = usable & ((ratio > BROKEN_RATIO) | (ratio < 1 / BROKEN_RATIO))
        score = int(broken.sum())
        if best is None or score > best[0]:
            best = (score, frame, float(ratio[usable].max()) if usable.any() else 1.0,
                    int((usable & (ratio > STRETCH_LIMIT)).sum()), broken.copy())
    score, frame, worst, torn, broken = best
    counts = {}
    for region in regions[edges[broken][:, 0]]:
        counts[region] = counts.get(region, 0) + 1
    n = max(1, int(usable.sum()))
    return ClipAudit(clip.name, total, frame, worst, score / n, torn / n,
                     dict(sorted(counts.items(), key=lambda kv: -kv[1])))


def audit_rig(rig, clips: dict, progress=None, step: int = 2) -> list[ClipAudit]:
    edges, regions = unique_edges(rig.faces), dominant_regions(rig)
    results = []
    for i, clip in enumerate(clips.values()):
        results.append(audit_clip(rig, clip, edges, regions, step))
        if progress:
            progress(i + 1, len(clips))
    return sorted(results, key=lambda r: -r.broken_share)


def summary(results: list[ClipAudit]) -> str:
    lines = ["Animation audit (worst clips first): share of edges changed more than 2x at the worst frame"]
    for r in results:
        where = ", ".join(f"{k} {v}" for k, v in list(r.regions.items())[:3]) or "-"
        flag = "  <-- check" if r.broken_share > 0.02 or r.torn_share > 0.005 else ""
        lines.append(f"  {r.name}: {100 * r.broken_share:.1f}% broken, worst x{r.worst_ratio:.1f}, "
                     f"frame {r.worst_frame}/{r.frames}; mostly: {where}{flag}")
    return "\n".join(lines)
