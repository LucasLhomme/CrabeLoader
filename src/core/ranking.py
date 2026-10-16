"""Find which base figures suit a given model best.

Step 1 (cheap, every figure): compare body silhouettes - width and thickness per height slice, measured the same way
on the model and on each figure raised to T-pose. (Comparing the autofit's joints against the figures' joints was
tried first: the two disagree about what a joint is, so the noise was as large as the differences between figures.)
Near-identical figures are collapsed so the list offers different body types instead of five clones.
Step 2 (the best few): build the real rig and measure how much the mesh deforms (quality.py).
The final score mixes the three things that make a base good: shape match, little stretching in the test poses, and
little distortion when the T-pose is baked into the figure's pose (measured on the real rig). The arm angle of the
figure was tried as a predictor of that distortion and does not predict it (proportion mismatch does)."""
import json
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from src.core.base_character import BaseCharacter
from src.core.accessories import strip
from src.core.model import CharacterModel, auto_placement
from src.core.quality import deformation_report
from src.core.rig import build_rig
from src.core.skeleton import build_anchors, measure

SHORTLIST = 12
"""Figures that get the full rig test."""
CACHE_VERSION = 3
SLABS = 40
"""Height slices of the silhouette profile."""
MATCH_SCALE = 0.7
"""Profile distance at which the shape match falls to 37 %."""
DIVERSITY = 0.2
"""Two figures closer than this are the same body type: only the best one is kept in the shortlist."""
BAKE_COST = 800.0
"""Score points lost per unit of baked-triangle distortion share (5 % of the triangles distorted = 40 points)."""
WEIGHT_MATCH, WEIGHT_DEFORMATION, WEIGHT_BAKE = 0.5, 0.3, 0.2


@dataclass
class Candidate:
    actor: str
    display_name: str
    score: int
    level: str
    distance: float
    summary: str
    distortion: float = 0.0
    match: int = 0
    deformation: int = 0
    bake: float = 0.0
    note: str = ""
    tested: bool = True
    """False: only the silhouette was compared (no rig built yet)."""
    franchise: str = ""


def _arm_angle(shoulder_x: float, shoulder_y: float, tip_x: float, tip_y: float) -> float:
    """Degrees the arm hangs below the horizontal (0 = T-pose)."""
    return float(np.degrees(np.arctan2(shoulder_y - tip_y, max(abs(tip_x - shoulder_x), 1e-6))))


def silhouette_profile(vertices: np.ndarray) -> np.ndarray:
    """Body shape of a T-posed figure, comparable between any two meshes: per height slice, the half-width (arms
    included, so the arm span shows at shoulder height) and the front-to-back thickness, as fractions of the height.
    Robust percentiles keep a cape or a weapon from dominating."""
    v = np.asarray(vertices, float)
    low, high = float(v[:, 1].min()), float(v[:, 1].max())
    height = max(high - low, 1e-9)
    slab = np.minimum(((v[:, 1] - low) / height * SLABS).astype(int), SLABS - 1)
    width, depth = np.zeros(SLABS), np.zeros(SLABS)
    for k in range(SLABS):
        pts = v[slab == k]
        if len(pts) < 3:
            continue
        width[k] = np.percentile(np.abs(pts[:, 0]), 90) / height
        depth[k] = 0.5 * (np.percentile(pts[:, 2], 90) - np.percentile(pts[:, 2], 10)) / height
    return np.r_[width, depth]


def base_shape(base: BaseCharacter) -> tuple[np.ndarray, float]:
    """(silhouette profile of the figure raised to T-pose, arm hang angle in its bind pose)."""
    from src.core.rig import base_tpose

    world, _ = base_tpose(base)
    profile = silhouette_profile(np.vstack(base.deform_preview_parts(world)))
    j = base.joints
    shoulder, hand = j["Shoulder_Left"], j["Hand_Left"]
    return profile, _arm_angle(shoulder[0], shoulder[1], hand[0], hand[1])


def usable(base: BaseCharacter) -> bool:
    try:
        return bool(base.supported and base.body_primitive.standard)
    except Exception:
        return False


def shape_distance(a: np.ndarray, b: np.ndarray) -> float:
    return float(np.linalg.norm(a - b))


def shape_match(distance: float) -> int:
    return int(round(100 * np.exp(-((distance / MATCH_SCALE) ** 2))))


def final_score(match: int, deformation: int, bake: float) -> int:
    bake_score = float(np.clip(100 - BAKE_COST * bake, 0, 100))
    return int(round(WEIGHT_MATCH * match + WEIGHT_DEFORMATION * deformation + WEIGHT_BAKE * bake_score))


def diverse_shortlist(scored: list[tuple[float, object, np.ndarray]], count: int = SHORTLIST):
    """Best figures first, skipping the ones that are the same body type as a figure already kept."""
    kept, spare = [], []
    for item in scored:
        if all(shape_distance(item[2], other[2]) >= DIVERSITY for other in kept):
            kept.append(item)
        else:
            spare.append(item)
        if len(kept) == count:
            return kept
    return kept + spare[:count - len(kept)]


def _cache_path(work: Path) -> Path:
    return Path(work) / f"base_features_v{CACHE_VERSION}.json"


def _load_cache(work: Path) -> dict:
    try:
        return json.loads(_cache_path(work).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def figure_features(entries, work: Path, progress=lambda text: None) -> dict[str, tuple[np.ndarray, float, str]]:
    """actor -> (profile, arm angle, model name) for every usable, distinct figure (cached on disk)."""
    cache = _load_cache(work)
    out, seen, changed = {}, set(), False
    for i, entry in enumerate(entries):
        row = cache.get(entry.actor)
        if row is None:
            try:
                base = BaseCharacter.from_pack(entry)
                if not usable(base):
                    row = {"skip": True}
                else:
                    features, angle = base_shape(base)
                    row = {"features": features.tolist(), "angle": angle, "model": base.model,
                           "name": base.display_name}
            except Exception:
                row = {"skip": True}
            cache[entry.actor] = row
            changed = True
            if i % 20 == 0:
                progress(f"Comparing body shapes… {i}/{len(entries)}")
        if row.get("skip") or row["model"] in seen:  # Crystal / Glow variants share the model of their standard figure
            continue
        seen.add(row["model"])
        out[entry.actor] = (np.array(row["features"]), float(row["angle"]), row["name"])
    if changed:
        try:
            Path(work).mkdir(parents=True, exist_ok=True)
            _cache_path(work).write_text(json.dumps(cache), encoding="utf-8")
        except OSError:
            pass
    return out


def rank_by_shape(model_profile: np.ndarray, figures: dict) -> list[tuple[float, str, np.ndarray, float]]:
    """(distance, actor, profile, arm angle) for every figure, closest first."""
    rows = [(shape_distance(f, model_profile), actor, f, angle) for actor, (f, angle, _) in figures.items()]
    rows.sort(key=lambda r: r[0])
    return rows


def rank_bases(source, entries, work: Path, progress=lambda text: None, shortlist: int = SHORTLIST,
               blender: Path | None = None, weight_transfer: bool = False, skip=(), fingers: bool = False,
               ignored=None, removed=None) -> list[Candidate]:
    """Every distinct figure, ranked: the `shortlist` best not in `skip` get the full rig test (best first), the
    others follow in silhouette order, marked untested. `entries` are roster entries."""
    reference = CharacterModel(source, auto_placement(source, 2.0), ignored, removed)
    ref_props, _ = measure(reference.vertices(), reference.faces, None, reference.measure_mask())
    figures = figure_features(entries, work, progress)
    by_actor = {e.actor: e for e in entries}
    ordered = rank_by_shape(silhouette_profile(strip(reference.vertices(), reference.faces, reference.measure_mask())[0]), figures)
    skip = set(skip)
    picks = diverse_shortlist([(d, actor, f) for d, actor, f, _ in ordered if actor not in skip], shortlist)

    ranked = []
    for n, (distance, actor, _) in enumerate(picks, 1):
        name = figures[actor][2]
        progress(f"Testing {name} ({n}/{len(picks)})…")
        try:
            base = BaseCharacter.from_pack(by_actor[actor])
            model = CharacterModel(source, auto_placement(source, base.fit_height), ignored, removed)
            props, _ = measure(model.vertices(), model.faces, base, model.measure_mask())
            rig = build_rig(model, build_anchors(props, base), base, blender, work, lambda text: None, None,
                            weight_transfer, fingers)
            report = deformation_report(rig)
        except Exception as exc:
            progress(f"  {name}: skipped ({exc})")
            continue
        match = shape_match(distance)
        score = final_score(match, report.score, report.bake)
        level = "good" if score >= 70 else "warn" if score >= 50 else "bad"
        note = f"shape {match}%, stretch {report.score}/100, bake {100 * report.bake:.0f}%"
        ranked.append(Candidate(actor, name, score, level, distance, report.summary(), report.distortion, match,
                                report.score, report.bake, note, True, by_actor[actor].franchise))
    ranked.sort(key=lambda c: (-c.score, c.distortion))
    tested = {c.actor for c in ranked} | skip
    for distance, actor, _, _ in ordered:
        if actor not in tested:
            match = shape_match(distance)
            ranked.append(Candidate(actor, figures[actor][2], match, "warn", distance, "", 0.0, match, 0, 0.0,
                                    f"shape {match}%, not tested", False, by_actor[actor].franchise))
    return ranked
