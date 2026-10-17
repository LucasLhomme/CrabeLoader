import numpy as np

from src.core import animation_audit as audit


def test_region_of_bone():
    assert audit.region_of_bone("Shoulder_Left") == "shoulder"
    assert audit.region_of_bone("Thumb2_Right") == "hand"
    assert audit.region_of_bone("Ankle_Left") == "foot"
    assert audit.region_of_bone("Cape") == "other"


def test_summary_flags_bad_clips():
    good = audit.ClipAudit("Idle", 10, 0, 1.1, 0.0, 0.0, {})
    bad = audit.ClipAudit("Roll", 10, 4, 9.0, 0.08, 0.01, {"leg": 30, "hand": 5})
    text = audit.summary([bad, good])
    assert "Roll" in text.splitlines()[1] and "<-- check" in text.splitlines()[1]
    assert "<-- check" not in text.splitlines()[2]
