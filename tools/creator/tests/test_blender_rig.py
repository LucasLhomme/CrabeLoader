import json
import unittest
from pathlib import Path
import tempfile
import numpy as np

from src.core.blender_rig import (
    load_rig_file,
    parse_json_rig,
    map_and_validate_rig,
    BoneNode,
    RigParsingError,
    UnsupportedRigError,
    _slerp,
    compose_transform,
)


class TestBlenderRig(unittest.TestCase):
    def test_slerp_interpolation(self):
        q0 = np.array([0.0, 0.0, 0.0, 1.0], dtype=np.float32)
        s = np.sin(np.pi / 4)
        c = np.cos(np.pi / 4)
        q1 = np.array([0.0, 0.0, s, c], dtype=np.float32)

        mid = _slerp(q0, q1, 0.5)
        exp_s = np.sin(np.pi / 8)
        exp_c = np.cos(np.pi / 8)
        self.assertTrue(np.isclose(mid[2], exp_s, atol=1e-3))
        self.assertTrue(np.isclose(mid[3], exp_c, atol=1e-3))

    def test_compose_transform(self):
        trans = np.array([1.0, 2.0, 3.0], dtype=np.float32)
        rot = np.array([0.0, 0.0, 0.0, 1.0], dtype=np.float32)
        scale = np.array([1.0, 1.0, 1.0], dtype=np.float32)
        mat = compose_transform(trans, rot, scale)
        self.assertTrue(np.allclose(mat[:3, 3], trans))
        self.assertTrue(np.allclose(mat[:3, :3], np.eye(3)))

    def test_unsupported_rig_missing_essential_bones(self):
        bones = [
            BoneNode(name="RandomBone1", index=0),
            BoneNode(name="RandomBone2", index=1),
        ]
        with self.assertRaises(UnsupportedRigError) as ctx:
            map_and_validate_rig(bones)
        self.assertIn("missing essential bones", str(ctx.exception))

    def test_json_rig_parsing_and_evaluation(self):
        rig_data = {
            "name": "TestHumanoid",
            "bones": [
                {"name": "Hips", "position": [0, 1, 0]},
                {"name": "Spine", "parent": "Hips", "position": [0, 0.3, 0]},
                {"name": "Head", "parent": "Spine", "position": [0, 0.4, 0]},
                {"name": "UpperArm_L", "parent": "Spine", "position": [-0.3, 0.2, 0]},
                {"name": "UpperArm_R", "parent": "Spine", "position": [0.3, 0.2, 0]},
                {"name": "Thigh_L", "parent": "Hips", "position": [-0.15, -0.1, 0]},
                {"name": "Thigh_R", "parent": "Hips", "position": [0.15, -0.1, 0]},
            ],
            "animations": [
                {
                    "name": "Idle",
                    "duration": 1.0,
                    "fps": 30.0,
                    "tracks": {
                        "Head": {
                            "positions": [
                                {"time": 0.0, "value": [0, 0.4, 0]},
                                {"time": 0.5, "value": [0, 0.42, 0]},
                                {"time": 1.0, "value": [0, 0.4, 0]}
                            ]
                        }
                    }
                }
            ]
        }
        with tempfile.TemporaryDirectory() as tmp_dir:
            json_file = Path(tmp_dir) / "rig.json"
            json_file.write_text(json.dumps(rig_data), encoding="utf-8")

            rig = load_rig_file(json_file)
            self.assertEqual(rig.name, "TestHumanoid")
            self.assertEqual(len(rig.bones), 7)
            self.assertIn("Hips", rig.mapped_canonical)
            self.assertIn("Head", rig.mapped_canonical)
            self.assertEqual(len(rig.animations), 1)

            anim = rig.animations[0]
            self.assertEqual(anim.name, "Idle")
            self.assertEqual(anim.duration, 1.0)
            head_track = anim.tracks["Head"]
            pos, rot, scale = head_track.evaluate(
                0.5,
                np.array([0, 0, 0], dtype=np.float32),
                np.array([0, 0, 0, 1], dtype=np.float32),
                np.array([1, 1, 1], dtype=np.float32)
            )
            self.assertTrue(np.isclose(pos[1], 0.42, atol=1e-3))


if __name__ == "__main__":
    unittest.main()

