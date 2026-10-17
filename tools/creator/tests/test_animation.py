import unittest
import numpy as np

from src.core.animation_player import AnimationPlayer, BUILTIN_CLIPS, evaluate_procedural_walk, evaluate_procedural_attack


class DummyBaseCharacter:
    def __init__(self):
        self.bone_names = (
            "Hips", "Spine2", "Spine3", "Neck", "Head",
            "Shoulder_L", "Elbow_L", "Forearm_L", "Hand_L", "Thumb_L", "Index_L", "Middle_L",
            "Shoulder_R", "Elbow_R", "Forearm_R", "Hand_R", "Thumb_R", "Index_R", "Middle_R",
            "Thigh_L", "Knee_L", "Ankle_L", "Ball_L", "Toes_L",
            "Thigh_R", "Knee_R", "Ankle_R", "Ball_R", "Toes_R",
        )
        self.bind_world = np.zeros((len(self.bone_names), 4, 4), dtype=np.float32)
        for i in range(len(self.bone_names)):
            self.bind_world[i] = np.eye(4, dtype=np.float32)

    def bone_index(self, name: str) -> int:
        return self.bone_names.index(name)


class TestAnimationPlayer(unittest.TestCase):
    def setUp(self):
        self.player = AnimationPlayer(fps=30.0)
        self.dummy_base = DummyBaseCharacter()

    def test_initial_state(self):
        self.assertFalse(self.player.is_playing)
        self.assertEqual(self.player.current_frame, 0)
        self.assertEqual(self.player.current_time, 0.0)
        self.assertTrue(self.player.loop)

    def test_play_pause_toggle(self):
        self.player.play()
        self.assertTrue(self.player.is_playing)
        self.player.pause()
        self.assertFalse(self.player.is_playing)
        self.player.toggle_play()
        self.assertTrue(self.player.is_playing)
        self.player.toggle_play()
        self.assertFalse(self.player.is_playing)

    def test_seek_frame_and_time(self):
        self.player.seek_frame(15)
        self.assertEqual(self.player.current_frame, 15)
        self.assertAlmostEqual(self.player.current_time, 0.5, places=2)

        self.player.seek_time(1.0)
        self.assertAlmostEqual(self.player.current_time, 1.0, places=2)
        self.assertEqual(self.player.current_frame, 30)

    def test_stop_resets_time(self):
        self.player.seek_frame(20)
        self.player.play()
        self.player.stop()
        self.assertFalse(self.player.is_playing)
        self.assertEqual(self.player.current_time, 0.0)
        self.assertEqual(self.player.current_frame, 0)

    def test_procedural_walk_evaluation(self):
        t0 = 0.0
        t1 = 0.25
        posed0 = evaluate_procedural_walk(self.dummy_base, self.dummy_base.bind_world, t0)
        posed1 = evaluate_procedural_walk(self.dummy_base, self.dummy_base.bind_world, t1)
        self.assertEqual(posed0.shape, (len(self.dummy_base.bone_names), 4, 4))
        self.assertEqual(posed1.shape, (len(self.dummy_base.bone_names), 4, 4))
        # Joints should move between t0 and t1
        self.assertFalse(np.allclose(posed0, posed1))

    def test_procedural_attack_evaluation(self):
        posed0 = evaluate_procedural_attack(self.dummy_base, self.dummy_base.bind_world, 0.0)
        posed1 = evaluate_procedural_attack(self.dummy_base, self.dummy_base.bind_world, 0.5)
        self.assertEqual(posed0.shape, (len(self.dummy_base.bone_names), 4, 4))
        self.assertFalse(np.allclose(posed0, posed1))

    def test_procedural_dive_roll_evaluation(self):
        from src.core.animation_player import evaluate_procedural_roll
        posed0 = evaluate_procedural_roll(self.dummy_base, self.dummy_base.bind_world, 0.0)
        posed_mid = evaluate_procedural_roll(self.dummy_base, self.dummy_base.bind_world, 0.6)
        self.assertEqual(posed0.shape, (len(self.dummy_base.bone_names), 4, 4))
        self.assertEqual(posed_mid.shape, (len(self.dummy_base.bone_names), 4, 4))
        # Hips and body must rotate substantially during 360 roll
        self.assertFalse(np.allclose(posed0, posed_mid))

    def test_procedural_jump_evaluation(self):
        from src.core.animation_player import evaluate_procedural_jump
        posed0 = evaluate_procedural_jump(self.dummy_base, self.dummy_base.bind_world, 0.0)
        posed_air = evaluate_procedural_jump(self.dummy_base, self.dummy_base.bind_world, 0.5)
        self.assertEqual(posed0.shape, (len(self.dummy_base.bone_names), 4, 4))
        self.assertFalse(np.allclose(posed0, posed_air))

    def test_get_character_animation_clips(self):
        from src.core.animation_player import get_character_animation_clips
        dummy = type("DummyBase", (), {
            "actor": "EMP_Luke",
            "game_model_zip": lambda self, g: Path("nonexistent.zip")
        })()
        clips = get_character_animation_clips(dummy)
        self.assertGreaterEqual(len(clips), 10)
        # Verify dive roll is included
        self.assertTrue(any("diveroll" in k.lower() or "rouler" in k.lower() for k in clips))
        # Verify combos and jump are included
        self.assertTrue(any("jump" in k.lower() or "saut" in k.lower() for k in clips))
        self.assertTrue(any("combo" in k.lower() for k in clips))

    def test_evaluate_pose_and_world(self):
        dummy_rig = type("DummyRig", (), {
            "base": self.dummy_base,
            "baked": np.zeros((10, 3), dtype=np.float32),
            "bone_idx": np.zeros((10, 4), dtype=np.uint8),
            "bone_w": np.ones((10, 4), dtype=np.uint8) * 64,
        })()
        posed_verts, posed_world = self.player.evaluate_pose_and_world(dummy_rig)
        self.assertEqual(posed_verts.shape, (10, 3))
        self.assertEqual(posed_world.shape, (len(self.dummy_base.bone_names), 4, 4))

    def test_timeline_rig_lock_state(self):
        import customtkinter as ctk
        from src.ui.timeline import TimelineWidget
        root = ctk.CTk()
        timeline = TimelineWidget(root, self.player, lambda: None)
        # By default before rig build, controls are locked
        self.assertFalse(timeline.rig_ready)
        self.assertEqual(timeline.play_btn.cget("state"), "disabled")
        # Building rig unlocks timeline
        timeline.set_rig_ready(True)
        self.assertTrue(timeline.rig_ready)
        self.assertEqual(timeline.play_btn.cget("state"), "normal")
        root.destroy()


if __name__ == "__main__":
    unittest.main()

