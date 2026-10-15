"""Animation playback controller and pose generator for OpenGL viewport."""
from __future__ import annotations

import math
import re
import subprocess
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable

import numpy as np

from src.core.base_character import BaseCharacter
from src.core.blender_rig import AnimationTrack, ParsedRig
from src.core.paths import BMS_SCRIPT, QUICKBMS
from src.core.rig import RigResult, skin
from src.core.skeleton import SIDES

_NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)


@dataclass
class AnimationClip:
    name: str
    duration: float
    fps: float = 30.0
    evaluate_world: Callable[[BaseCharacter, np.ndarray, float], np.ndarray] | None = None
    custom_track: AnimationTrack | None = None

    @property
    def total_frames(self) -> int:
        return max(1, int(round(self.duration * self.fps)))


def _rot_x(angle_rad: float) -> np.ndarray:
    c, s = math.cos(angle_rad), math.sin(angle_rad)
    return np.array([[1.0, 0.0, 0.0], [0.0, c, s], [0.0, -s, c]], dtype=np.float32)


def _rot_y(angle_rad: float) -> np.ndarray:
    c, s = math.cos(angle_rad), math.sin(angle_rad)
    return np.array([[c, 0.0, -s], [0.0, 1.0, 0.0], [s, 0.0, c]], dtype=np.float32)


def _rot_z(angle_rad: float) -> np.ndarray:
    c, s = math.cos(angle_rad), math.sin(angle_rad)
    return np.array([[c, s, 0.0], [-s, c, 0.0], [0.0, 0.0, 1.0]], dtype=np.float32)


def _find_bone(base: BaseCharacter, prefix: str, side: str = "") -> int:
    side_short = ("_L" if side == "Left" else "_R") if side else ""
    candidates = []
    if side:
        candidates = [f"{prefix}_{side}", f"{prefix}{side_short}", f"{prefix}_{side.lower()}", f"{prefix}{side}"]
    else:
        candidates = [prefix, prefix.lower(), prefix.capitalize()]
    for candidate in candidates:
        try:
            return base.bone_index(candidate)
        except (KeyError, ValueError, AttributeError):
            pass
    for i, n in enumerate(base.bone_names):
        nl = n.lower()
        if side and any(nl == c.lower() for c in candidates):
            return i
        if not side and nl.startswith(prefix.lower()):
            return i
    return 0


# =============================================================================
# PROCEDURAL MOTION EVALUATORS
# =============================================================================

def evaluate_procedural_roll(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate a dynamic 360-degree tumbling dive roll (forward roll)."""
    cycle = 1.2
    u = (t % cycle) / cycle  # 0.0 to 1.0

    hips_idx = _find_bone(base, "Hips")
    pivot_hips = world[hips_idx][3, :3].copy()

    # Complete 360-degree forward rotation around X
    roll_angle = 2.0 * math.pi * u

    # Lower character toward floor during the roll
    drop_y = math.sin(u * math.pi) * 0.55
    # Forward leap translation
    fwd_z = math.sin(u * math.pi) * 0.40

    rx = _rot_x(roll_angle)
    rot_all = np.eye(4, dtype=np.float32)
    rot_all[:3, :3] = rx
    rot_all[3, :3] = pivot_hips - pivot_hips @ rx + np.array([0.0, -drop_y, fwd_z], dtype=np.float32)

    # Roll root and whole armature
    posed = world @ rot_all

    # Limb tucking (knees bent, thighs pulled forward, arms curled)
    tuck = math.sin(u * math.pi) ** 2
    if tuck > 0.04:
        r_thigh = _rot_x(math.radians(65.0 * tuck))
        r_knee = _rot_x(math.radians(-90.0 * tuck))
        r_arm = _rot_x(math.radians(45.0 * tuck))

        for side, _ in SIDES:
            side_sfx = (side, "_L" if side == "Left" else "_R")
            th_idx = _find_bone(base, "Thigh", side)
            p_th = posed[th_idx][3, :3]
            m_th = np.eye(4, dtype=np.float32)
            m_th[:3, :3] = r_thigh
            m_th[3, :3] = p_th - p_th @ r_thigh

            kn_idx = _find_bone(base, "Knee", side)
            p_kn = posed[kn_idx][3, :3]
            m_kn = np.eye(4, dtype=np.float32)
            m_kn[:3, :3] = r_knee
            m_kn[3, :3] = p_kn - p_kn @ r_knee

            sh_idx = _find_bone(base, "Shoulder", side)
            p_sh = posed[sh_idx][3, :3]
            m_sh = np.eye(4, dtype=np.float32)
            m_sh[:3, :3] = r_arm
            m_sh[3, :3] = p_sh - p_sh @ r_arm

            for i, n in enumerate(base.bone_names):
                if any(n.endswith(sfx) for sfxx in side_sfx for sfx in (sfxx, sfxx.lower())):
                    if n.startswith(("Knee", "Ankle", "Ball", "Toes")):
                        posed[i] = posed[i] @ m_kn
                    elif n.startswith("Thigh"):
                        posed[i] = posed[i] @ m_th
                    elif n.startswith(("Shoulder", "Elbow", "Hand")):
                        posed[i] = posed[i] @ m_sh

    return posed


def evaluate_procedural_jump(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate athletic vertical jump with crouch, launch, mid-air tuck and landing."""
    cycle = 1.4
    u = (t % cycle) / cycle

    if u < 0.2:
        p = u / 0.2
        crouch = math.sin(p * math.pi * 0.5) * 0.22
        h_offset = -crouch
        knee_angle = crouch * 2.2
    elif u < 0.8:
        p = (u - 0.2) / 0.6
        jump_h = math.sin(p * math.pi) * 0.85
        h_offset = jump_h
        knee_angle = math.sin(p * math.pi) * 0.7
    else:
        p = (u - 0.8) / 0.2
        crouch = math.sin((1.0 - p) * math.pi * 0.5) * 0.2
        h_offset = -crouch
        knee_angle = crouch * 1.8

    t_mat = np.eye(4, dtype=np.float32)
    t_mat[3, 1] = h_offset
    posed = world @ t_mat

    for side, _ in SIDES:
        r_knee = _rot_x(-knee_angle)
        kn_idx = _find_bone(base, "Knee", side)
        p_kn = posed[kn_idx][3, :3]
        m_kn = np.eye(4, dtype=np.float32)
        m_kn[:3, :3] = r_knee
        m_kn[3, :3] = p_kn - p_kn @ r_knee

        arm_out = math.radians(25.0 * (1.0 if u < 0.8 else 0.2))
        r_sh = _rot_z(arm_out if side == "Left" else -arm_out)
        sh_idx = _find_bone(base, "Shoulder", side)
        p_sh = posed[sh_idx][3, :3]
        m_sh = np.eye(4, dtype=np.float32)
        m_sh[:3, :3] = r_sh
        m_sh[3, :3] = p_sh - p_sh @ r_sh

        side_sfx = (side, "_L" if side == "Left" else "_R")
        for i, n in enumerate(base.bone_names):
            if any(n.endswith(sfx) for sfxx in side_sfx for sfx in (sfxx, sfxx.lower())):
                if n.startswith(("Knee", "Ankle", "Ball", "Toes")):
                    posed[i] = posed[i] @ m_kn
                elif n.startswith(("Shoulder", "Elbow", "Hand")):
                    posed[i] = posed[i] @ m_sh

    return posed


def evaluate_procedural_combat_stance(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate athletic combat guard stance (Saber On / ready posture)."""
    posed = world.copy()
    breathe = math.sin(2.0 * math.pi * 0.5 * t)
    spine_sway = breathe * math.radians(1.5)

    r_spine = _rot_y(math.radians(-12.0)) @ _rot_x(spine_sway)
    hips_idx = _find_bone(base, "Hips")
    p_hips = world[hips_idx][3, :3]
    m_spine = np.eye(4, dtype=np.float32)
    m_spine[:3, :3] = r_spine
    m_spine[3, :3] = p_hips - p_hips @ r_spine

    for i, n in enumerate(base.bone_names):
        if n.startswith(("Spine", "Neck", "Head")):
            posed[i] = world[i] @ m_spine

    for side, _ in SIDES:
        r_arm = _rot_x(math.radians(-35.0)) @ _rot_y(math.radians(-20.0 if side == "Right" else 20.0))
        sh_idx = _find_bone(base, "Shoulder", side)
        p_sh = world[sh_idx][3, :3]
        m_arm = np.eye(4, dtype=np.float32)
        m_arm[:3, :3] = r_arm
        m_arm[3, :3] = p_sh - p_sh @ r_arm

        side_sfx = (side, "_L" if side == "Left" else "_R")
        for i, n in enumerate(base.bone_names):
            if any(n.endswith(sfx) for sfxx in side_sfx for sfx in (sfxx, sfxx.lower())) and n.startswith(
                    ("Shoulder", "Elbow", "Hand", "Thumb", "Index", "Middle")):
                posed[i] = world[i] @ m_arm

    return posed


def evaluate_procedural_combo_1(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate rapid horizontal slash attack (Combo Y)."""
    posed = world.copy()
    u = (t % 0.8) / 0.8

    if u < 0.35:
        p = u / 0.35
        arm_z = math.radians(80.0 * p)
        arm_x = math.radians(-30.0 * p)
        spine_y = math.radians(-20.0 * p)
    elif u < 0.65:
        p = (u - 0.35) / 0.3
        arm_z = math.radians(80.0 - 130.0 * p)
        arm_x = math.radians(-30.0 + 75.0 * p)
        spine_y = math.radians(-20.0 + 45.0 * p)
    else:
        p = (u - 0.65) / 0.35
        arm_z = math.radians(-50.0 * (1.0 - p))
        arm_x = math.radians(45.0 * (1.0 - p))
        spine_y = math.radians(25.0 * (1.0 - p))

    # Spine rotation
    r_spine = _rot_y(spine_y)
    hips_idx = _find_bone(base, "Hips")
    p_hips = world[hips_idx][3, :3]
    m_sp = np.eye(4, dtype=np.float32)
    m_sp[:3, :3] = r_spine
    m_sp[3, :3] = p_hips - p_hips @ r_spine
    for i, n in enumerate(base.bone_names):
        if n.startswith(("Spine", "Neck", "Head")):
            posed[i] = world[i] @ m_sp

    # Right arm strike
    sh_idx = _find_bone(base, "Shoulder", "Right")
    p_sh = world[sh_idx][3, :3]
    r_arm = _rot_z(-arm_z) @ _rot_x(arm_x)
    m_arm = np.eye(4, dtype=np.float32)
    m_arm[:3, :3] = r_arm
    m_arm[3, :3] = p_sh - p_sh @ r_arm
    for i, n in enumerate(base.bone_names):
        if (n.endswith("_Right") or n.endswith("_R")) and n.startswith(
                ("Shoulder", "Elbow", "Forearm", "Hand", "Thumb", "Index", "Middle")):
            posed[i] = world[i] @ m_arm

    return posed


# Alias for backward compatibility
evaluate_procedural_attack = evaluate_procedural_combo_1



def evaluate_procedural_combo_2(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate reverse cross slash combo (Combo YY)."""
    posed = world.copy()
    u = (t % 0.9) / 0.9

    if u < 0.3:
        p = u / 0.3
        arm_x = math.radians(60.0 * p)
        arm_y = math.radians(-40.0 * p)
    elif u < 0.6:
        p = (u - 0.3) / 0.3
        arm_x = math.radians(60.0 - 110.0 * p)
        arm_y = math.radians(-40.0 + 80.0 * p)
    else:
        p = (u - 0.6) / 0.3
        arm_x = math.radians(-50.0 * (1.0 - p))
        arm_y = math.radians(40.0 * (1.0 - p))

    sh_idx = _find_bone(base, "Shoulder", "Right")
    p_sh = world[sh_idx][3, :3]
    r_arm = _rot_x(arm_x) @ _rot_y(arm_y)
    m_arm = np.eye(4, dtype=np.float32)
    m_arm[:3, :3] = r_arm
    m_arm[3, :3] = p_sh - p_sh @ r_arm

    for i, n in enumerate(base.bone_names):
        if (n.endswith("_Right") or n.endswith("_R")) and n.startswith(
                ("Shoulder", "Elbow", "Forearm", "Hand", "Thumb", "Index", "Middle")):
            posed[i] = world[i] @ m_arm

    return posed


def evaluate_procedural_helmbreaker(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate heavy jumping overhead smash (Helmbreaker / Ground Slam)."""
    cycle = 1.4
    u = (t % cycle) / cycle
    posed = world.copy()

    if u < 0.4:
        p = u / 0.4
        h_off = math.sin(p * math.pi * 0.5) * 0.65
        arm_lift = math.radians(-110.0 * p)
    elif u < 0.65:
        p = (u - 0.4) / 0.25
        h_off = 0.65 * (1.0 - p) - 0.25 * math.sin(p * math.pi * 0.5)
        arm_lift = math.radians(-110.0 + 160.0 * p)
    else:
        p = (u - 0.65) / 0.35
        h_off = -0.25 * (1.0 - p)
        arm_lift = math.radians(50.0 * (1.0 - p))

    t_mat = np.eye(4, dtype=np.float32)
    t_mat[3, 1] = h_off
    posed = world @ t_mat

    # Both arms raised overhead then slammed downward
    for side, _ in SIDES:
        sh_idx = _find_bone(base, "Shoulder", side)
        p_sh = posed[sh_idx][3, :3]
        r_arm = _rot_x(arm_lift)
        m_arm = np.eye(4, dtype=np.float32)
        m_arm[:3, :3] = r_arm
        m_arm[3, :3] = p_sh - p_sh @ r_arm
        side_sfx = (side, "_L" if side == "Left" else "_R")
        for i, n in enumerate(base.bone_names):
            if any(n.endswith(sfx) for sfxx in side_sfx for sfx in (sfxx, sfxx.lower())) and n.startswith(
                    ("Shoulder", "Elbow", "Hand")):
                posed[i] = posed[i] @ m_arm

    return posed


def evaluate_procedural_sprint(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate aggressive high-speed sprint cycle."""
    cycle = 0.65
    freq = 1.0 / cycle
    phase = 2.0 * math.pi * freq * t
    posed = world.copy()

    # Forward torso lean
    r_lean = _rot_x(math.radians(16.0))
    hips_idx = _find_bone(base, "Hips")
    p_hips = world[hips_idx][3, :3]
    m_lean = np.eye(4, dtype=np.float32)
    m_lean[:3, :3] = r_lean
    m_lean[3, :3] = p_hips - p_hips @ r_lean
    for i, n in enumerate(base.bone_names):
        if n.startswith(("Spine", "Neck", "Head")):
            posed[i] = world[i] @ m_lean

    leg_amp = math.radians(52.0)
    knee_amp = math.radians(70.0)
    arm_amp = math.radians(50.0)

    for side, sx in SIDES:
        leg_phase = phase if sx > 0 else (phase + math.pi)
        thigh_angle = math.sin(leg_phase) * leg_amp
        knee_angle = max(0.0, -math.sin(leg_phase)) * knee_amp

        thigh_idx = _find_bone(base, "Thigh", side)
        pivot_thigh = world[thigh_idx][3, :3]
        r_thigh = _rot_x(thigh_angle)
        rot_mat = np.eye(4, dtype=np.float32)
        rot_mat[:3, :3] = r_thigh
        rot_mat[3, :3] = pivot_thigh - pivot_thigh @ r_thigh

        side_suffix = (side, "_L" if side == "Left" else "_R")
        for i, n in enumerate(base.bone_names):
            if any(n.endswith(sfx) for sfxx in side_suffix for sfx in (sfxx, sfxx.lower())) and n.startswith(
                    ("Thigh", "Knee", "Ankle", "Ball", "Toes")):
                posed[i] = world[i] @ rot_mat

        arm_phase = leg_phase + math.pi
        shoulder_angle = math.sin(arm_phase) * arm_amp
        shoulder_idx = _find_bone(base, "Shoulder", side)
        pivot_shoulder = world[shoulder_idx][3, :3]
        r_shoulder = _rot_x(shoulder_angle)
        rot_mat_arm = np.eye(4, dtype=np.float32)
        rot_mat_arm[:3, :3] = r_shoulder
        rot_mat_arm[3, :3] = pivot_shoulder - pivot_shoulder @ r_shoulder

        for i, n in enumerate(base.bone_names):
            if any(n.endswith(sfx) for sfxx in side_suffix for sfx in (sfxx, sfxx.lower())) and n.startswith(
                    ("Shoulder", "Elbow", "Forearm", "Hand")):
                posed[i] = world[i] @ rot_mat_arm

    return posed


def evaluate_procedural_walk(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate a dynamic, continuous walk cycle."""
    posed = world.copy()
    freq = 1.0
    phase = 2.0 * math.pi * freq * t

    leg_amp = math.radians(35.0)
    knee_amp = math.radians(45.0)
    arm_amp = math.radians(30.0)

    for side, sx in SIDES:
        leg_phase = phase if sx > 0 else (phase + math.pi)
        thigh_angle = math.sin(leg_phase) * leg_amp
        knee_angle = max(0.0, -math.sin(leg_phase)) * knee_amp

        thigh_idx = _find_bone(base, "Thigh", side)
        pivot_thigh = world[thigh_idx][3, :3]
        r_thigh = _rot_x(thigh_angle)
        rot_mat = np.eye(4, dtype=np.float32)
        rot_mat[:3, :3] = r_thigh
        rot_mat[3, :3] = pivot_thigh - pivot_thigh @ r_thigh

        side_suffix = (side, "_L" if side == "Left" else "_R")
        for i, n in enumerate(base.bone_names):
            if any(n.endswith(sfx) for sfxx in side_suffix for sfx in (sfxx, sfxx.lower())) and n.startswith(
                    ("Thigh", "Knee", "Ankle", "Ball", "Toes")):
                posed[i] = world[i] @ rot_mat

        arm_phase = leg_phase + math.pi
        shoulder_angle = math.sin(arm_phase) * arm_amp
        shoulder_idx = _find_bone(base, "Shoulder", side)
        pivot_shoulder = world[shoulder_idx][3, :3]
        r_shoulder = _rot_x(shoulder_angle)
        rot_mat_arm = np.eye(4, dtype=np.float32)
        rot_mat_arm[:3, :3] = r_shoulder
        rot_mat_arm[3, :3] = pivot_shoulder - pivot_shoulder @ r_shoulder

        for i, n in enumerate(base.bone_names):
            if any(n.endswith(sfx) for sfxx in side_suffix for sfx in (sfxx, sfxx.lower())) and n.startswith(
                    ("Shoulder", "Elbow", "Forearm", "Hand", "Thumb", "Index", "Middle")):
                posed[i] = world[i] @ rot_mat_arm

    return posed


def evaluate_procedural_idle(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate natural breathing / idle posture."""
    posed = world.copy()
    breathe = math.sin(2.0 * math.pi * 0.4 * t)
    spine_angle = breathe * math.radians(2.0)
    r_spine = _rot_x(spine_angle)

    for i, n in enumerate(base.bone_names):
        if n.startswith(("Spine", "Neck", "Head", "Clavicle")):
            pivot = world[i][3, :3]
            rot = np.eye(4, dtype=np.float32)
            rot[:3, :3] = r_spine
            rot[3, :3] = pivot - pivot @ r_spine
            posed[i] = world[i] @ rot

    return posed


def evaluate_procedural_celebrate(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate victory celebration (arms raised high in triumph)."""
    posed = world.copy()
    cycle = 2.0
    u = (t % cycle) / cycle
    sway = math.sin(u * 2.0 * math.pi) * math.radians(4.0)

    # Spine celebration posture
    r_sp = _rot_x(math.radians(-6.0)) @ _rot_z(sway)
    hips_idx = _find_bone(base, "Hips")
    p_hips = world[hips_idx][3, :3]
    m_sp = np.eye(4, dtype=np.float32)
    m_sp[:3, :3] = r_sp
    m_sp[3, :3] = p_hips - p_hips @ r_sp
    for i, n in enumerate(base.bone_names):
        if n.startswith(("Spine", "Neck", "Head")):
            posed[i] = world[i] @ m_sp

    # Both arms raised high V-shape
    for side, _ in SIDES:
        r_sh = _rot_x(math.radians(-125.0)) @ _rot_z(math.radians(-30.0 if side == "Right" else 30.0))
        sh_idx = _find_bone(base, "Shoulder", side)
        p_sh = world[sh_idx][3, :3]
        m_sh = np.eye(4, dtype=np.float32)
        m_sh[:3, :3] = r_sh
        m_sh[3, :3] = p_sh - p_sh @ r_sh
        side_sfx = (side, "_L" if side == "Left" else "_R")
        for i, n in enumerate(base.bone_names):
            if any(n.endswith(sfx) for sfxx in side_sfx for sfx in (sfxx, sfxx.lower())) and n.startswith(
                    ("Shoulder", "Elbow", "Hand")):
                posed[i] = world[i] @ m_sh

    return posed


def evaluate_procedural_hit_react(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate damage recoil / hit react."""
    cycle = 0.7
    u = (t % cycle) / cycle
    posed = world.copy()

    if u < 0.25:
        p = u / 0.25
        recoil = math.radians(-22.0 * p)
    else:
        p = (u - 0.25) / 0.75
        recoil = math.radians(-22.0 * (1.0 - p))

    r_sp = _rot_x(recoil)
    hips_idx = _find_bone(base, "Hips")
    p_hips = world[hips_idx][3, :3]
    m_sp = np.eye(4, dtype=np.float32)
    m_sp[:3, :3] = r_sp
    m_sp[3, :3] = p_hips - p_hips @ r_sp
    for i, n in enumerate(base.bone_names):
        if n.startswith(("Spine", "Neck", "Head")):
            posed[i] = world[i] @ m_sp

    return posed


def evaluate_procedural_death(base: BaseCharacter, world: np.ndarray, t: float) -> np.ndarray:
    """Evaluate knockout / defeat collapse."""
    cycle = 1.6
    u = min(1.0, (t % cycle) / (cycle * 0.8))  # settle at end
    posed = world.copy()

    drop = math.sin(u * math.pi * 0.5) * 0.85
    fall_back = math.radians(-75.0 * u)

    r_fall = _rot_x(fall_back)
    hips_idx = _find_bone(base, "Hips")
    p_hips = world[hips_idx][3, :3]
    m_fall = np.eye(4, dtype=np.float32)
    m_fall[:3, :3] = r_fall
    m_fall[3, :3] = p_hips - p_hips @ r_fall + np.array([0.0, -drop, -0.3 * u], dtype=np.float32)

    posed = world @ m_fall
    return posed


# =============================================================================
# CHARACTER MOVESET SCANNING & RESOLUTION
# =============================================================================

def scan_game_motions(base: BaseCharacter, game_dir: Path | None = None) -> list[str]:
    """Scan the base character's game archive for real motion files (.oct)."""
    if not game_dir:
        return []
    archive = base.game_model_zip(game_dir)
    if not archive.exists():
        archive = archive.with_name(archive.name + ".orig")
    if not archive.exists() or not QUICKBMS.exists() or not BMS_SCRIPT.exists():
        return []
    try:
        out = subprocess.run([str(QUICKBMS), "-l", str(BMS_SCRIPT), str(archive)],
                             capture_output=True, text=True, creationflags=_NO_WINDOW).stdout
        motions = [line.split()[2].split("\\")[-1] for line in out.splitlines()
                   if len(line.split()) >= 3 and line.split()[2].lower().endswith(".oct")
                   and "motions" in line.split()[2].lower()]
        return sorted(set(motions))
    except Exception:
        return []


def get_character_animation_clips(base: BaseCharacter, game_dir: Path | None = None) -> dict[str, AnimationClip]:
    """Build the comprehensive, character-specific animation suite for the selected base figure."""
    prefix = base.actor.split("_")[-1].lower() if "_" in base.actor else base.actor.lower()

    # Core canonical moveset tailored to the character
    clips = {
        f"Roll / Dive Roll (Rouler) [{prefix}_atk_diveroll]": AnimationClip(
            name=f"Roll / Dive Roll (Rouler) [{prefix}_atk_diveroll]",
            duration=1.2, fps=30.0, evaluate_world=evaluate_procedural_roll
        ),
        f"Jump & Aerial (Saut) [{prefix}_bm_jump]": AnimationClip(
            name=f"Jump & Aerial (Saut) [{prefix}_bm_jump]",
            duration=1.4, fps=30.0, evaluate_world=evaluate_procedural_jump
        ),
        f"Combat Guard (Garde) [{prefix}_bm_idle_stance]": AnimationClip(
            name=f"Combat Guard (Garde) [{prefix}_bm_idle_stance]",
            duration=2.0, fps=30.0, evaluate_world=evaluate_procedural_combat_stance
        ),
        f"Combo Attack 1 (Attaque 1) [{prefix}_atk_combo_y]": AnimationClip(
            name=f"Combo Attack 1 (Attaque 1) [{prefix}_atk_combo_y]",
            duration=0.8, fps=30.0, evaluate_world=evaluate_procedural_combo_1
        ),
        f"Combo Attack 2 (Attaque 2) [{prefix}_atk_combo_yy]": AnimationClip(
            name=f"Combo Attack 2 (Attaque 2) [{prefix}_atk_combo_yy]",
            duration=0.9, fps=30.0, evaluate_world=evaluate_procedural_combo_2
        ),
        f"Heavy Smash / Helmbreaker [{prefix}_atk_helmbreaker]": AnimationClip(
            name=f"Heavy Smash / Helmbreaker [{prefix}_atk_helmbreaker]",
            duration=1.4, fps=30.0, evaluate_world=evaluate_procedural_helmbreaker
        ),
        f"Sprint / Fast Run (Course) [{prefix}_bm_run]": AnimationClip(
            name=f"Sprint / Fast Run (Course) [{prefix}_bm_run]",
            duration=0.65, fps=30.0, evaluate_world=evaluate_procedural_sprint
        ),
        f"Walk (Marche) [{prefix}_bm_walk]": AnimationClip(
            name=f"Walk (Marche) [{prefix}_bm_walk]",
            duration=1.0, fps=30.0, evaluate_world=evaluate_procedural_walk
        ),
        f"Idle Breathing (Attente) [{prefix}_bm_idle]": AnimationClip(
            name=f"Idle Breathing (Attente) [{prefix}_bm_idle]",
            duration=2.5, fps=30.0, evaluate_world=evaluate_procedural_idle
        ),
        f"Victory Celebrate (Victoire) [{prefix}_mnt_celebrate]": AnimationClip(
            name=f"Victory Celebrate (Victoire) [{prefix}_mnt_celebrate]",
            duration=2.0, fps=30.0, evaluate_world=evaluate_procedural_celebrate
        ),
        f"Hit React (Dégâts) [{prefix}_rct_hit]": AnimationClip(
            name=f"Hit React (Dégâts) [{prefix}_rct_hit]",
            duration=0.7, fps=30.0, evaluate_world=evaluate_procedural_hit_react
        ),
        f"Knockout / Defeat (K.O.) [{prefix}_rct_death]": AnimationClip(
            name=f"Knockout / Defeat (K.O.) [{prefix}_rct_death]",
            duration=1.6, fps=30.0, evaluate_world=evaluate_procedural_death
        ),
    }

    # Optional: scan real motion names from game archive and map key motions
    scanned = scan_game_motions(base, game_dir)
    for m in scanned[:30]:  # Register top game motion clips
        stem = m[:-4] if m.endswith(".oct") else m
        if any(stem in k for k in clips):
            continue
        evaluator = evaluate_procedural_idle
        sl = stem.lower()
        if any(k in sl for k in ("roll", "diveroll", "dodge")):
            evaluator = evaluate_procedural_roll
        elif any(k in sl for k in ("jump", "air", "fall", "land")):
            evaluator = evaluate_procedural_jump
        elif any(k in sl for k in ("run", "sprint")):
            evaluator = evaluate_procedural_sprint
        elif any(k in sl for k in ("walk", "patrol")):
            evaluator = evaluate_procedural_walk
        elif any(k in sl for k in ("combo", "atk", "slash", "swing", "strike")):
            evaluator = evaluate_procedural_combo_1
        elif any(k in sl for k in ("smash", "helmbreaker", "slam")):
            evaluator = evaluate_procedural_helmbreaker
        elif any(k in sl for k in ("stance", "guard", "block")):
            evaluator = evaluate_procedural_combat_stance
        elif any(k in sl for k in ("rct", "hit", "stmb")):
            evaluator = evaluate_procedural_hit_react
        elif any(k in sl for k in ("death", "fail", "ko")):
            evaluator = evaluate_procedural_death
        elif any(k in sl for k in ("celebrate", "taunt", "cheer")):
            evaluator = evaluate_procedural_celebrate

        name = f"Game Motion: {stem}"
        clips[name] = AnimationClip(name=name, duration=1.2, fps=30.0, evaluate_world=evaluator)

    return clips


BUILTIN_CLIPS = {
    "Roll / Dive Roll (Rouler)": AnimationClip(
        name="Roll / Dive Roll (Rouler)", duration=1.2, fps=30.0, evaluate_world=evaluate_procedural_roll
    ),
    "Walk": AnimationClip(name="Walk", duration=1.0, fps=30.0, evaluate_world=evaluate_procedural_walk),
    "Idle": AnimationClip(name="Idle", duration=2.5, fps=30.0, evaluate_world=evaluate_procedural_idle),
    "Attack Combo": AnimationClip(name="Attack Combo", duration=0.8, fps=30.0, evaluate_world=evaluate_procedural_combo_1),
    "Jump": AnimationClip(name="Jump", duration=1.4, fps=30.0, evaluate_world=evaluate_procedural_jump),
    "Sprint": AnimationClip(name="Sprint", duration=0.65, fps=30.0, evaluate_world=evaluate_procedural_sprint),
}


class AnimationPlayer:
    """Controls animation playback, scrubbing, and frame evaluation."""

    def __init__(self, fps: float = 30.0):
        self.fps = fps
        self.is_playing = False
        self.loop = True
        self.speed = 1.0
        self.current_time = 0.0
        self._last_perf_time: float | None = None
        self.active_clip: AnimationClip | None = BUILTIN_CLIPS["Roll / Dive Roll (Rouler)"]

    @property
    def duration(self) -> float:
        return self.active_clip.duration if self.active_clip else 1.0

    @property
    def total_frames(self) -> int:
        return max(1, int(round(self.duration * self.fps)))

    @property
    def current_frame(self) -> int:
        f = int(round(self.current_time * self.fps))
        return min(max(0, f), self.total_frames - 1)

    def set_clip(self, clip: AnimationClip | None):
        self.active_clip = clip
        self.current_time = 0.0
        self._last_perf_time = None

    def play(self):
        self.is_playing = True
        self._last_perf_time = time.perf_counter()

    def pause(self):
        self.is_playing = False
        self._last_perf_time = None

    def toggle_play(self):
        if self.is_playing:
            self.pause()
        else:
            self.play()

    def stop(self):
        self.is_playing = False
        self.current_time = 0.0
        self._last_perf_time = None

    def seek_frame(self, frame: int):
        target = max(0, min(frame, self.total_frames - 1))
        self.current_time = float(target) / self.fps
        if self.current_time > self.duration:
            self.current_time = self.duration

    def seek_time(self, t: float):
        if self.duration <= 0:
            self.current_time = 0.0
            return
        if self.loop:
            self.current_time = t % self.duration
        else:
            self.current_time = max(0.0, min(t, self.duration))

    def tick(self) -> bool:
        """Step playback based on real time clock. Returns True if time changed."""
        if not self.is_playing:
            return False

        now = time.perf_counter()
        if self._last_perf_time is None:
            self._last_perf_time = now
            return False

        dt = (now - self._last_perf_time) * self.speed
        self._last_perf_time = now

        new_t = self.current_time + dt
        if new_t >= self.duration:
            if self.loop:
                self.current_time = new_t % self.duration
            else:
                self.current_time = self.duration
                self.pause()
        else:
            self.current_time = new_t
        return True

    def get_current_posed_world(self, base: BaseCharacter) -> np.ndarray:
        """Return the current posed bone-to-world matrices (B, 4, 4) for the active animation."""
        world = base.bind_world
        if self.active_clip is None:
            return world

        if self.active_clip.evaluate_world is not None:
            return self.active_clip.evaluate_world(base, world, self.current_time)

        if self.active_clip.custom_track is not None:
            bone_poses = self.active_clip.custom_track.sample(self.current_time)
            posed_world = world.copy()
            for bone_name, (pos, rot) in bone_poses.items():
                try:
                    idx = base.bone_index(bone_name)
                    pivot = world[idx][3, :3]
                    m = np.eye(4, dtype=np.float32)
                    m[:3, :3] = rot
                    m[3, :3] = pivot - pivot @ rot + pos
                    posed_world[idx] = world[idx] @ m
                except (KeyError, ValueError):
                    pass
            return posed_world

        return world

    def evaluate_pose_and_world(self, rig: RigResult) -> tuple[np.ndarray, np.ndarray]:
        """Compute deformed mesh vertex positions and posed_world for the current frame."""
        posed_world = self.get_current_posed_world(rig.base)
        posed_verts = skin(rig.baked, rig.bone_idx, rig.bone_w, rig.base.bind_world, posed_world)
        return posed_verts, posed_world

    def evaluate_pose(self, rig: RigResult) -> np.ndarray:
        """Compute deformed mesh vertex positions for the current playback frame."""
        verts, _ = self.evaluate_pose_and_world(rig)
        return verts
