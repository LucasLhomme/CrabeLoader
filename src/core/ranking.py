"""Find which base figure suits a given model best.

Step 1 compares body proportions (cheap, every figure). Step 2 builds the real rig on the closest few and scores
how much each one deforms (see quality.py). The result is a short ranked list."""
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from src.core.base_character import BaseCharacter
from src.core.model import CharacterModel, auto_placement
from src.core.quality import deformation_report
from src.core.rig import build_rig
from src.core.skeleton import build_anchors, measure

SHORTLIST = 200
DISTANCE_PENALTY = 25.0
"""Score points lost per unit of proportion distance: a rig can deform little yet still look wrong (a slim model on
a bulky figure), so the body-proportion match counts too."""
FEATURE_WEIGHTS = np.array([1.5, 0.5, 1.0, 1.0, 1.0])  # hips, shoulder width, shoulder height, neck, leg spacing


@dataclass
class Candidate:
    actor: str
    display_name: str
    score: int
    level: str
    distance: float
    summary: str
    distortion: float = 0.0


def model_features(props) -> np.ndarray:
    h = max(props.height, 1e-6)
    return np.array([(props.crotch_y + 0.06 * h) / h, props.shoulder_x / h, props.arm_y / h, props.neck_y / h,
                     props.leg_x / h])


def base_features(base: BaseCharacter) -> np.ndarray:
    j, h = base.joints, max(base.height, 1e-6)
    return np.array([j["Hips"][1] / h, abs(j["Shoulder_Left"][0]) / h, j["Shoulder_Left"][1] / h,
                     j["Neck"][1] / h, abs(j["Thigh_Left"][0]) / h])


def usable(base: BaseCharacter) -> bool:
    try:
        return bool(base.supported and base.body_primitive.standard)
    except Exception:
        return False


def rank_bases(source, entries, work: Path, progress=lambda text: None, shortlist: int = SHORTLIST,
               blender: Path | None = None) -> list[Candidate]:
    """Ranked candidates (best first). `entries` are roster entries; `progress` gets short status lines."""
    reference = CharacterModel(source, auto_placement(source, 2.0))
    ref_props, _ = measure(reference.vertices(), reference.faces, None)
    target = model_features(ref_props)

    scored, seen_models = [], set()
    for i, entry in enumerate(entries):
        try:
            base = BaseCharacter.from_pack(entry)
        except Exception:
            continue
        if not usable(base):
            continue
        if base.model in seen_models:  # Crystal / Glow variants share the model of their standard figure
            continue
        seen_models.add(base.model)
        distance = float(np.sqrt((FEATURE_WEIGHTS * (base_features(base) - target) ** 2).sum()))
        scored.append((distance, entry, base))
        if i % 20 == 0:
            progress(f"Comparing proportions… {i}/{len(entries)}")
    scored.sort(key=lambda t: t[0])

    ranked = []
    for n, (distance, entry, base) in enumerate(scored[:shortlist], 1):
        progress(f"Testing {base.display_name} ({n}/{min(shortlist, len(scored))})…")
        try:
            model = CharacterModel(source, auto_placement(source, base.fit_height))
            props, _ = measure(model.vertices(), model.faces, base)
            rig = build_rig(model, build_anchors(props, base), base, blender, work, lambda text: None)
            report = deformation_report(rig)
        except Exception as exc:
            progress(f"  {base.display_name}: skipped ({exc})")
            continue
        ranked.append(Candidate(entry.actor, base.display_name, report.score, report.level, distance,
                                report.summary(), report.distortion))
    ranked.sort(key=lambda c: -(c.score - DISTANCE_PENALTY * c.distance - 100 * c.distortion))
    return ranked
