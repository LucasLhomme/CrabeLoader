import json
import tempfile
import unittest
from pathlib import Path

import numpy as np
from PIL import Image

from src.core.mod_writer import Identity, write_text_files
from src.core.npc_config import EntityConfig
from src.core.skeleton import (
    Proportions,
    apply_apose_to_proportions,
    apply_tpose_to_proportions,
    base_joint_points,
    base_skeleton_segments,
)
from src.core.textures import TextureIds, export_loose_textures


class TestV2Features(unittest.TestCase):
    def test_base_skeleton_segments_and_points(self):
        dummy_joints = {
            "Hips": np.array([0.0, 1.0, 0.0]),
            "Spine2": np.array([0.0, 1.2, 0.0]),
            "Spine3": np.array([0.0, 1.4, 0.0]),
            "Neck": np.array([0.0, 1.6, 0.0]),
            "Head": np.array([0.0, 1.8, 0.0]),
            "Clavicle_Left": np.array([0.05, 1.5, 0.0]),
            "Shoulder_Left": np.array([0.25, 1.5, 0.0]),
            "Elbow_Left": np.array([0.35, 1.2, 0.0]),
            "Hand_Left": np.array([0.45, 0.9, 0.0]),
            "Middle1_Left": np.array([0.48, 0.8, 0.0]),
            "Thigh_Left": np.array([0.1, 0.9, 0.0]),
            "Knee_Left": np.array([0.1, 0.5, 0.0]),
            "Ankle_Left": np.array([0.1, 0.1, 0.0]),
            "Ball_Left": np.array([0.1, 0.0, 0.1]),
        }
        segs = base_skeleton_segments(dummy_joints)
        points = base_joint_points(dummy_joints)
        self.assertGreater(len(segs), 0)
        self.assertEqual(segs.shape[1], 3)
        self.assertEqual(len(points), len(dummy_joints))
        self.assertEqual(points.shape[1], 3)

    def test_apose_and_tpose_presets(self):
        p = Proportions(
            height=2.0, arm_y=1.5, arm_z=0.0, shoulder_x=0.25, tip_x=0.8, tip_y=1.5,
            wrist=0.55, neck_y=1.6, head_y=1.8, crotch_y=0.9, torso_z=0.0,
            leg_x=0.1, knee=0.5, knee_z=0.0, ankle_y=0.1, ankle_z=0.0, toe_z=0.2
        )
        dummy_base = type("DummyBase", (), {
            "joints": {
                "Shoulder_Left": np.array([0.25, 1.5, 0.0]),
                "Hand_Left": np.array([0.45, 0.95, 0.0]),  # A-pose downward slope
            }
        })()

        # Apply A-Pose
        p_apose = apply_apose_to_proportions(p, dummy_base)
        self.assertLess(p_apose.tip_y, p.arm_y)

        # Apply T-Pose
        p_tpose = apply_tpose_to_proportions(p_apose)
        self.assertEqual(p_tpose.tip_y, p_tpose.arm_y)

    def test_export_loose_textures_vfs(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            ids = TextureIds(prefix="a5", diffuse="a5a0000050a40001", normal="a5a0000050a40002",
                             material_hash=b"\x12\x34\x56\x78")
            img = Image.new("RGB", (64, 64), (100, 150, 200))
            files = export_loose_textures(ids, img, tmp_path)
            self.assertEqual(len(files), 2)
            for f in files:
                self.assertTrue(f.exists())
                self.assertTrue(f.name.endswith(".tbody"))
                self.assertGreater(f.stat().st_size, 128)  # Valid DDS header + data

    def test_declarative_manifest_generation(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            ident = Identity(slug="test_hero", display_name="Test Hero", actor="TST_Hero")
            dummy_base = type("DummyBase", (), {
                "actor": "EMP_Luke",
                "actor_lower": "emp_luke",
                "entry": type("Entry", (), {"progression_tree": "EMP_Luke", "dnax_stem": "emp_luke"})(),
                "model": "emp_luke",
                "model_dir": "emp_luke",
            })()
            write_text_files(tmp_path, ident, dummy_base, EntityConfig())
            manifest_file = tmp_path / "mod.json"
            self.assertTrue(manifest_file.exists())
            data = json.loads(manifest_file.read_text(encoding="utf-8"))
            self.assertIn("character", data)
            self.assertEqual(data["character"]["name"], "TST_Hero")
            self.assertEqual(data["character"]["baseCharacter"], "EMP_Luke")
            self.assertEqual(data["character"]["displayName"], "Test Hero")

    def test_stylized_character_proportions(self):
        from src.core.skeleton import measure
        import trimesh
        # Build synthetic stylized mesh: wide cylinder torso (r=0.25, h=1.0) and arms extending out to x=0.6
        torso = trimesh.creation.cylinder(radius=0.25, height=1.0)
        torso.apply_translation([0, 0.5, 0])
        arm_l = trimesh.creation.cylinder(radius=0.08, height=0.35)
        rot_z = trimesh.transformations.rotation_matrix(np.pi / 2, [0, 0, 1])
        arm_l.apply_transform(rot_z)
        arm_l.apply_translation([0.42, 0.7, 0])
        arm_r = trimesh.creation.cylinder(radius=0.08, height=0.35)
        arm_r.apply_transform(rot_z)
        arm_r.apply_translation([-0.42, 0.7, 0])
        combined = trimesh.util.concatenate([torso, arm_l, arm_r])

        props, report = measure(combined.vertices, combined.faces, None)
        self.assertGreater(props.shoulder_x, 0.20)  # Should sit near torso edge (r=0.25), not pulled into chest
        self.assertAlmostEqual(props.arm_y, 0.7, delta=0.05)


if __name__ == "__main__":
    unittest.main()


