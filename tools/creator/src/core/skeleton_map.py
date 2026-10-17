"""Map Creator's canonical joints onto any figure's skinning palette.

Creator places and bakes a fixed set of joints (Hips, Spine2, Spine3, Neck, Head and, per side, Clavicle, Shoulder,
Elbow, Hand, Middle1, Thigh, Knee, Ankle, Ball). Figures name their bones mostly the same way, but a palette only
holds the bones its mesh is skinned to: a dress replaces the thighs, a boot has no ankle bone, and so on.

For each canonical joint:
  skin bone  the first candidate present in the palette: the weights painted for that joint go to it;
  position   the bone's own bind position when the candidate sits at the same place (the canonical bone, or an
             overlay like Thigh_OL), otherwise transferred from Luke's proportions onto two resolved neighbours.
"""
from dataclasses import dataclass, field

import numpy as np

SIDES = (("Left", 1.0), ("Right", -1.0))

# (candidate, colocated): {S} is the side. Order matters: the first candidate in the palette wins. "@Joint" is the
# skin bone already chosen for another canonical joint (a figure without foot bones moves its feet with the knee).
_CANDIDATES: dict[str, tuple[tuple[str, bool], ...]] = {
    "Hips": (("Hips", True), ("Body", True), ("Hips1", True)),
    "Head": (("Head", True),),
    "Shoulder_{S}": (("Shoulder_{S}", True),),
    "Hand_{S}": (("Hand_{S}", True),),
    "Knee_{S}": (("Knee_{S}", True), ("Knee_Break_{S}", True), ("Knee_Dress_{S}", True)),
    "Spine2": (("Spine2", True), ("Spine1", False), ("Spine3", False), ("Hips", False), ("Body", False)),
    "Spine3": (("Spine3", True), ("Spine2", False), ("Spine4", False), ("Spine1", False), ("Hips", False),
               ("Body", False)),
    "Neck": (("Neck", True), ("Spine3", False), ("Head", False)),
    "Clavicle_{S}": (("Clavicle_{S}", True), ("Clavicle{S}", True), ("Shoulder_Spin_{S}", False),
                     ("Spine3", False), ("Spine2", False)),
    "Elbow_{S}": (("Elbow_{S}", True), ("Elbow_OL_{S}", True), ("Forearm_OL_{S}", False), ("Shoulder_{S}", False)),
    "Middle1_{S}": (("Middle1_{S}", True), ("Index1_{S}", True), ("Hand_{S}", False)),
    "Thigh_{S}": (("Thigh_{S}", True), ("Thigh_OL_{S}", True), ("Thigh_Dress_{S}", True), ("DressThigh{S}", True),
                  ("Thigh_OL_{S}_Half", True), ("Thigh_Pivot_{S}", True), ("Hips", False), ("Body", False)),
    "Ankle_{S}": (("Ankle_{S}", True), ("Ball_{S}", False), ("Toes_{S}", False), ("Ball_Scale_{S}", False),
                  ("@Knee_{S}", False)),
    "Ball_{S}": (("Ball_{S}", True), ("Ball_Scale_{S}", True), ("Toes_{S}", False), ("Ankle_{S}", False),
                 ("@Knee_{S}", False)),
}

# Resolution order, and the (anchor, scale reference) pairs used to transfer a missing position.
_ORDER: tuple[tuple[str, tuple[tuple[str, str], ...]], ...] = (
    ("Hips", ()), ("Head", ()), ("Shoulder_{S}", ()), ("Hand_{S}", ()), ("Knee_{S}", ()),
    ("Neck", (("Head", "Hips"),)),
    ("Spine2", (("Hips", "Neck"),)),
    ("Spine3", (("Hips", "Neck"),)),
    ("Clavicle_{S}", (("Shoulder_{S}", "Hips"),)),
    ("Elbow_{S}", (("Shoulder_{S}", "Hand_{S}"),)),
    ("Middle1_{S}", (("Hand_{S}", "Elbow_{S}"),)),
    ("Thigh_{S}", (("Knee_{S}", "Hips"),)),
    ("Ball_{S}", (("Knee_{S}", "Hips"),)),
    ("Ankle_{S}", (("Ball_{S}", "Knee_{S}"), ("Knee_{S}", "Thigh_{S}"))),
)

# Joints a figure must really have (not estimated) to take a humanoid mesh.
CORE = ("Hips", "Head", "Shoulder_Left", "Shoulder_Right", "Hand_Left", "Hand_Right", "Knee_Left", "Knee_Right")

# Luke Skywalker's bind pose (left side; the right side mirrors x): the proportions missing joints are rebuilt from.
TEMPLATE = {
    "Hips": (0.0, 1.1586, -0.0209), "Spine2": (0.0, 1.2292, -0.0210), "Spine3": (0.0, 1.3086, -0.0205),
    "Neck": (0.0, 1.7043, -0.0398), "Head": (0.0, 1.8577, 0.0180),
    "Clavicle_{S}": (0.0200, 1.5828, 0.0424), "Shoulder_{S}": (0.2806, 1.5293, -0.0561),
    "Elbow_{S}": (0.3415, 1.2249, -0.0678), "Hand_{S}": (0.3911, 0.9713, 0.0605),
    "Middle1_{S}": (0.3852, 0.8370, 0.1435), "Thigh_{S}": (0.1006, 1.0919, -0.0209),
    "Knee_{S}": (0.1006, 0.6465, -0.0441), "Ankle_{S}": (0.1006, 0.1263, -0.1236),
    "Ball_{S}": (0.1006, 0.0003, 0.0451),
}


def _sided(pattern: str, side: str) -> str:
    return pattern.replace("{S}", side)


def _template(pattern: str, sx: float) -> np.ndarray:
    p = np.array(TEMPLATE[pattern], float)
    if "{S}" in pattern:
        p[0] *= sx
    return p


@dataclass
class SkeletonMap:
    skin_bone: dict[str, str] = field(default_factory=dict)
    joints: dict[str, np.ndarray] = field(default_factory=dict)
    status: dict[str, str] = field(default_factory=dict)  # exact | substitute | estimated | missing

    @property
    def missing_core(self) -> list[str]:
        return [j for j in CORE if self.status.get(j) not in ("exact", "substitute")]

    @property
    def supported(self) -> bool:
        return not self.missing_core and all(j in self.skin_bone for j in self.status)

    @property
    def fallbacks(self) -> list[str]:
        return [j for j, s in self.status.items() if s != "exact"]

    def summary(self) -> str:
        if not self.supported:
            return "not a humanoid skeleton: missing " + ", ".join(self.missing_core or
                                                                   [j for j in self.status if j not in self.skin_bone])
        if not self.fallbacks:
            return "standard skeleton: every joint matches"
        parts = [f"{j} -> {self.skin_bone[j]}" + (" (estimated)" if self.status[j] == "estimated" else "")
                 for j in self.fallbacks]
        return "adapted skeleton: " + ", ".join(parts)


def map_skeleton(palette: dict[str, np.ndarray]) -> SkeletonMap:
    """palette: bone name -> bind-pose position, for the bones the body mesh is skinned to."""
    result = SkeletonMap()
    for pattern, references in _ORDER:
        for side, sx in (SIDES if "{S}" in pattern else (("", 1.0),)):
            joint = _sided(pattern, side)
            for candidate, colocated in _CANDIDATES[pattern]:
                name = _sided(candidate, side)
                if name.startswith("@"):
                    name = result.skin_bone.get(name[1:], "")
                if name in palette:
                    result.skin_bone[joint] = name
                    if colocated:
                        result.joints[joint] = np.array(palette[name], float)
                        result.status[joint] = "exact" if name == joint else "substitute"
                    break
            if joint in result.joints:
                continue
            for anchor, scale_ref in references:
                a, b = _sided(anchor, side), _sided(scale_ref, side)
                if a in result.joints and b in result.joints:
                    ta, tb = _template(anchor, sx), _template(scale_ref, sx)
                    s = np.linalg.norm(result.joints[b] - result.joints[a]) / max(np.linalg.norm(tb - ta), 1e-9)
                    result.joints[joint] = result.joints[a] + (_template(pattern, sx) - ta) * s
                    result.status[joint] = "estimated"
                    break
            else:
                result.status[joint] = "missing"
    return result
