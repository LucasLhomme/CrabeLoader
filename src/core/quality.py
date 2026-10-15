"""Deformation score of a built rig: how badly the mesh stretches when the figure moves.

A well-skinned character keeps its edge lengths in the preview poses. An edge that grows more than three times
longer than at rest is a tear or a spike: those edges are counted, and drawn in red in the viewport."""
from dataclasses import dataclass, field

import numpy as np

STRETCH_LIMIT = 3.0
MIN_REST_LENGTH = 0.004
"""Edges shorter than this at rest (welded seams, tiny details) are ignored: their ratio is noise."""
COLLAPSE_AREA, COLLAPSE_HEIGHT = 0.15, 0.5
"""Baked/original width x depth, or height, below which the mesh counts as collapsed (a healthy rig stays above 0.6)."""
GOOD_SCORE, WARN_SCORE = 75, 50
"""Score thresholds for the 'good' / 'acceptable' ratings."""
BROKEN_RATIO = 2.0
"""An edge more than 2x longer (or shorter than 1/2) than at rest counts as visibly deformed."""
MEAN_LIMIT, BROKEN_LIMIT, TORN_LIMIT = 0.08, 0.035, 0.03
"""Values at which each penalty is maxed out (calibrated on 59 test rigs: the middle rigs score ~60-70, the worst ~20)."""
QUALITY_POSES = (("arms", "Arms up"), ("walk", "Walk"))


@dataclass
class DeformationReport:
    score: int
    level: str  # "good" | "warn" | "bad"
    per_pose: dict = field(default_factory=dict)  # pose -> (stretched edges, worst ratio)
    total_edges: int = 0
    distortion: float = 0.0
    """Mean |ln(posed/rest length)| over all edges and both poses: 0 = no squash or stretch at all."""
    collapsed: bool = False
    """True when the baked mesh is flattened or crushed (bad weights): shown as a failure whatever the stretch says."""
    broken: float = 0.0
    """Fraction of edges stretched/squashed more than 2x in the worst pose."""

    def summary(self) -> str:
        label = {"good": "Good", "warn": "Acceptable", "bad": "Heavy deformation"}[self.level]
        if self.collapsed:
            return "Deformation: FAILED - the model is collapsed (flattened or crushed). The skinning weights are broken."
        lines = [f"Deformation: {label} ({self.score}/100) - {100 * self.broken:.1f}% of edges changed more than 2x"]
        for pose, title in QUALITY_POSES:
            count, worst = self.per_pose.get(pose, (0, 0.0))
            lines.append(f"  {title}: {count} stretched edge(s), worst ×{worst:.1f}")
        return "\n".join(lines)


def unique_edges(faces: np.ndarray) -> np.ndarray:
    e = np.concatenate([faces[:, [0, 1]], faces[:, [1, 2]], faces[:, [2, 0]]])
    return np.unique(np.sort(e, axis=1), axis=0)


def stretched_edges(rest: np.ndarray, posed: np.ndarray, edges: np.ndarray):
    """(mask of stretched edges, worst ratio) between a rest and a posed vertex set."""
    l0 = np.linalg.norm(rest[edges[:, 0]] - rest[edges[:, 1]], axis=1)
    l1 = np.linalg.norm(posed[edges[:, 0]] - posed[edges[:, 1]], axis=1)
    usable = l0 > MIN_REST_LENGTH
    ratio = np.where(usable, l1 / np.maximum(l0, 1e-9), 0.0)
    return ratio > STRETCH_LIMIT, float(ratio.max()) if usable.any() else 0.0


def edge_ratios(rest: np.ndarray, posed: np.ndarray, edges: np.ndarray) -> np.ndarray:
    """Posed/rest length of every usable edge (rest length above MIN_REST_LENGTH)."""
    l0 = np.linalg.norm(rest[edges[:, 0]] - rest[edges[:, 1]], axis=1)
    l1 = np.linalg.norm(posed[edges[:, 0]] - posed[edges[:, 1]], axis=1)
    usable = l0 > MIN_REST_LENGTH
    return l1[usable] / l0[usable]


def deformation_report(rig, amount: float = 1.0) -> DeformationReport:
    """Score 0-100 mixing three things: average squash/stretch of all edges, share of edges changed more than 2x,
    and share of torn edges (more than 3x). The old score only counted the last one, so it sat at 100 for most rigs."""
    # Weights that failed squash the model into a needle: compare the baked size with the size before baking.
    before, after = np.ptp(rig.tpose, axis=0), np.ptp(rig.baked, axis=0)
    ratio = after / np.maximum(before, 1e-9)
    if not np.isfinite(after).all() or ratio[0] * ratio[2] < COLLAPSE_AREA or ratio[1] < COLLAPSE_HEIGHT:
        return DeformationReport(0, "bad", {}, 0, 0.0, collapsed=True, broken=1.0)
    edges = unique_edges(rig.faces)
    per_pose, means, brokens, torns = {}, [], [], []
    for pose, _ in QUALITY_POSES:
        mask, worst = stretched_edges(rig.baked, rig.posed(pose, amount), edges)
        ratio = edge_ratios(rig.baked, rig.posed(pose, amount), edges)
        if len(ratio) == 0:
            ratio = np.ones(1)
        means.append(float(np.abs(np.log(np.clip(ratio, 1e-3, 1e3))).mean()))
        brokens.append(float(((ratio > BROKEN_RATIO) | (ratio < 1 / BROKEN_RATIO)).mean()))
        torns.append(float((ratio > STRETCH_LIMIT).mean()))
        per_pose[pose] = (int(mask.sum()), worst)
    penalty = (0.5 * min(1.0, max(means) / MEAN_LIMIT) + 0.3 * min(1.0, max(brokens) / BROKEN_LIMIT)
               + 0.2 * min(1.0, max(torns) / TORN_LIMIT))
    score = int(round(100 * (1.0 - penalty)))
    level = "good" if score >= GOOD_SCORE else "warn" if score >= WARN_SCORE else "bad"
    return DeformationReport(score, level, per_pose, len(edges), float(np.mean(means)), broken=max(brokens))


def stretched_segments(rig, pose: str, amount: float = 1.0) -> np.ndarray:
    """Line list (2K, 3) of the stretched edges in a pose, in the posed vertex positions (for the red overlay)."""
    if pose not in dict(QUALITY_POSES):
        return np.zeros((0, 3), np.float32)
    posed = rig.posed(pose, amount)
    edges = unique_edges(rig.faces)
    mask, _ = stretched_edges(rig.baked, posed, edges)
    return posed[edges[mask]].reshape(-1, 3).astype(np.float32)
