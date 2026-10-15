"""Blender rig and animation track parser.

Parses bone hierarchies, rest/bind transforms, and animation keyframe tracks from:
- glTF 2.0 / GLB files (standard export from Blender with armature and action tracks)
- JSON rig & animation manifests (custom or Blender script export)

Validates supported rigs (humanoid mapping, standard Disney Infinity joints, Rigify, Mixamo)
and provides clear, user-friendly error diagnostics.
"""
from __future__ import annotations

import json
import math
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import numpy as np

# Canonical joints expected by Disney Infinity Creator
CANONICAL_JOINTS = {
    "Hips", "Spine2", "Spine3", "Neck", "Head",
    "Clavicle_L", "Shoulder_L", "Elbow_L", "Hand_L",
    "Clavicle_R", "Shoulder_R", "Elbow_R", "Hand_R",
    "Thigh_L", "Knee_L", "Ankle_L",
    "Thigh_R", "Knee_R", "Ankle_R",
}

# Common bone name aliases across Blender Rigify, Mixamo, and generic rigs
BONE_ALIASES: dict[str, list[str]] = {
    "Hips": ["hips", "pelvis", "root", "c_pos", "hip"],
    "Spine2": ["spine", "spine1", "spine_01", "spine01", "abdomen", "torso"],
    "Spine3": ["spine2", "spine_02", "chest", "chest1", "upper_chest"],
    "Neck": ["neck", "neck1", "neck_01"],
    "Head": ["head", "head_01", "c_head"],
    "Clavicle_L": ["clavicle_l", "clavicle.l", "shoulder_l", "shoulder.l", "left_clavicle", "clavicle_left", "collar_l"],
    "Shoulder_L": ["upperarm_l", "upperarm.l", "upper_arm_l", "shoulder_l", "arm_l", "arm.l", "left_shoulder", "left_arm"],
    "Elbow_L": ["lowerarm_l", "lowerarm.l", "forearm_l", "forearm.l", "elbow_l", "elbow.l", "left_forearm"],
    "Hand_L": ["hand_l", "hand.l", "wrist_l", "wrist.l", "left_hand"],
    "Clavicle_R": ["clavicle_r", "clavicle.r", "shoulder_r", "shoulder.r", "right_clavicle", "clavicle_right", "collar_r"],
    "Shoulder_R": ["upperarm_r", "upperarm.r", "upper_arm_r", "shoulder_r", "arm_r", "arm.r", "right_shoulder", "right_arm"],
    "Elbow_R": ["lowerarm_r", "lowerarm.r", "forearm_r", "forearm.r", "elbow_r", "elbow.r", "right_forearm"],
    "Hand_R": ["hand_r", "hand.r", "wrist_r", "wrist.r", "right_hand"],
    "Thigh_L": ["thigh_l", "thigh.l", "upperleg_l", "upperleg.l", "upper_leg_l", "left_thigh", "leg_l"],
    "Knee_L": ["shin_l", "shin.l", "calf_l", "calf.l", "lowerleg_l", "knee_l", "knee.l", "left_shin"],
    "Ankle_L": ["foot_l", "foot.l", "ankle_l", "ankle.l", "left_foot", "foot_root_l"],
    "Thigh_R": ["thigh_r", "thigh.r", "upperleg_r", "upperleg.r", "upper_leg_r", "right_thigh", "leg_r"],
    "Knee_R": ["shin_r", "shin.r", "calf_r", "calf.r", "lowerleg_r", "knee_r", "knee.r", "right_shin"],
    "Ankle_R": ["foot_r", "foot.r", "ankle_r", "ankle.r", "right_foot", "foot_root_r"],
}


class RigParsingError(Exception):
    """Raised when a rig or animation file cannot be parsed or is malformed."""


class UnsupportedRigError(RigParsingError):
    """Raised when the rig structure does not meet humanoid requirements."""


@dataclass
class BoneNode:
    name: str
    index: int
    parent_index: int | None = None
    parent_name: str | None = None
    children: list[int] = field(default_factory=list)
    local_translation: np.ndarray = field(default_factory=lambda: np.zeros(3, dtype=np.float32))
    local_rotation_quat: np.ndarray = field(default_factory=lambda: np.array([0.0, 0.0, 0.0, 1.0], dtype=np.float32))  # (x, y, z, w)
    local_scale: np.ndarray = field(default_factory=lambda: np.ones(3, dtype=np.float32))
    local_matrix: np.ndarray = field(default_factory=lambda: np.eye(4, dtype=np.float32))
    world_matrix: np.ndarray = field(default_factory=lambda: np.eye(4, dtype=np.float32))
    inverse_bind_matrix: np.ndarray = field(default_factory=lambda: np.eye(4, dtype=np.float32))


@dataclass
class TransformKeyframe:
    time: float
    value: np.ndarray  # (3,) for pos/scale, (4,) for quaternion (x,y,z,w)
    interpolation: str = "LINEAR"


@dataclass
class BoneTrack:
    bone_name: str
    bone_index: int
    translations: list[TransformKeyframe] = field(default_factory=list)
    rotations: list[TransformKeyframe] = field(default_factory=list)  # quat (x,y,z,w)
    scales: list[TransformKeyframe] = field(default_factory=list)

    def evaluate(self, t: float, rest_pos: np.ndarray, rest_quat: np.ndarray, rest_scale: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
        pos = _sample_vec3(self.translations, t, rest_pos)
        rot = _sample_quat(self.rotations, t, rest_quat)
        scale = _sample_vec3(self.scales, t, rest_scale)
        return pos, rot, scale


@dataclass
class AnimationTrack:
    name: str
    duration: float
    fps: float
    tracks: dict[str, BoneTrack] = field(default_factory=dict)  # bone_name -> BoneTrack

    @property
    def frame_count(self) -> int:
        return max(1, int(round(self.duration * self.fps)))


@dataclass
class ParsedRig:
    name: str
    bones: list[BoneNode]
    bone_by_name: dict[str, BoneNode]
    mapped_canonical: dict[str, str]  # canonical_joint -> actual_bone_name
    animations: list[AnimationTrack] = field(default_factory=list)

    @property
    def bone_names(self) -> list[str]:
        return [b.name for b in self.bones]

    def summary(self) -> str:
        coverage = len(self.mapped_canonical) / len(CANONICAL_JOINTS) * 100
        anims = f", {len(self.animations)} animation(s)" if self.animations else ""
        return f"Rig '{self.name}': {len(self.bones)} bones, {coverage:.0f}% humanoid joint coverage{anims}"


# -----------------------------------------------------------------------------
# Math helpers for animation keyframe evaluation
# -----------------------------------------------------------------------------

def _sample_vec3(keys: list[TransformKeyframe], t: float, fallback: np.ndarray) -> np.ndarray:
    if not keys:
        return fallback.copy()
    if t <= keys[0].time:
        return keys[0].value.copy()
    if t >= keys[-1].time:
        return keys[-1].value.copy()

    # Binary search
    for i in range(len(keys) - 1):
        k0, k1 = keys[i], keys[i + 1]
        if k0.time <= t <= k1.time:
            dt = k1.time - k0.time
            factor = (t - k0.time) / dt if dt > 1e-6 else 0.0
            return (k0.value * (1.0 - factor) + k1.value * factor).astype(np.float32)
    return keys[-1].value.copy()


def _slerp(q0: np.ndarray, q1: np.ndarray, t: float) -> np.ndarray:
    """Spherical linear interpolation between two quaternions (x, y, z, w)."""
    dot = float(np.dot(q0, q1))
    q1 = q1.copy()
    if dot < 0.0:
        q1 = -q1
        dot = -dot
    if dot > 0.9995:
        res = q0 + t * (q1 - q0)
        norm = np.linalg.norm(res)
        return (res / (norm if norm > 1e-9 else 1.0)).astype(np.float32)

    theta_0 = math.acos(np.clip(dot, -1.0, 1.0))
    sin_theta_0 = math.sin(theta_0)
    theta = theta_0 * t
    sin_theta = math.sin(theta)

    s0 = math.cos(theta) - dot * sin_theta / sin_theta_0
    s1 = sin_theta / sin_theta_0
    res = (s0 * q0) + (s1 * q1)
    norm = np.linalg.norm(res)
    return (res / (norm if norm > 1e-9 else 1.0)).astype(np.float32)


def _sample_quat(keys: list[TransformKeyframe], t: float, fallback: np.ndarray) -> np.ndarray:
    if not keys:
        return fallback.copy()
    if t <= keys[0].time:
        return keys[0].value.copy()
    if t >= keys[-1].time:
        return keys[-1].value.copy()

    for i in range(len(keys) - 1):
        k0, k1 = keys[i], keys[i + 1]
        if k0.time <= t <= k1.time:
            dt = k1.time - k0.time
            factor = (t - k0.time) / dt if dt > 1e-6 else 0.0
            return _slerp(k0.value, k1.value, factor)
    return keys[-1].value.copy()


def quat_to_matrix(q: np.ndarray) -> np.ndarray:
    """Convert quaternion (x, y, z, w) to 4x4 matrix (column-vector or row-vector)."""
    x, y, z, w = q
    n = x * x + y * y + z * z + w * w
    s = 2.0 / n if n > 0.0 else 0.0
    xs, ys, zs = x * s, y * s, z * s
    wx, wy, wz = w * xs, w * ys, w * zs
    xx, xy, xz = x * xs, x * ys, x * zs
    yy, yz, zz = y * ys, y * zs, z * zs

    m = np.eye(4, dtype=np.float32)
    m[0, 0] = 1.0 - (yy + zz)
    m[0, 1] = xy - wz
    m[0, 2] = xz + wy
    m[1, 0] = xy + wz
    m[1, 1] = 1.0 - (xx + zz)
    m[1, 2] = yz - wx
    m[2, 0] = xz - wy
    m[2, 1] = yz + wx
    m[2, 2] = 1.0 - (xx + yy)
    return m


def compose_transform(trans: np.ndarray, quat: np.ndarray, scale: np.ndarray) -> np.ndarray:
    """Compose translation, quaternion rotation, and scale into a 4x4 matrix."""
    m = quat_to_matrix(quat)
    m[0, :3] *= scale[0]
    m[1, :3] *= scale[1]
    m[2, :3] *= scale[2]
    m[0, 3] = trans[0]
    m[1, 3] = trans[1]
    m[2, 3] = trans[2]
    return m


# -----------------------------------------------------------------------------
# Rig Validation & Bone Mapping
# -----------------------------------------------------------------------------

def map_and_validate_rig(bones: list[BoneNode]) -> dict[str, str]:
    """Map canonical joints to rig bones and validate minimum requirements."""
    if not bones:
        raise UnsupportedRigError("No bones found in the armature.")

    name_map = {b.name.lower(): b.name for b in bones}
    # Also index without prefixes like 'def-', 'org-', 'mch-', 'bip01 '
    clean_map = {}
    for b in bones:
        clean = b.name.lower()
        for prefix in ("def-", "org-", "mch-", "bip01 ", "bip01_"):
            if clean.startswith(prefix):
                clean = clean[len(prefix):]
        clean_map[clean] = b.name

    mapped: dict[str, str] = {}
    for canonical, aliases in BONE_ALIASES.items():
        # First check canonical directly
        if canonical.lower() in name_map:
            mapped[canonical] = name_map[canonical.lower()]
            continue
        # Then check aliases
        found = False
        for alias in aliases:
            if alias in name_map:
                mapped[canonical] = name_map[alias]
                found = True
                break
            if alias in clean_map:
                mapped[canonical] = clean_map[alias]
                found = True
                break
        if found:
            continue

    # Essential bones required to form a valid humanoid rig
    essential = {"Hips", "Head", "Shoulder_L", "Shoulder_R", "Thigh_L", "Thigh_R"}
    missing_essential = [joint for joint in essential if joint not in mapped]
    if len(missing_essential) > 2:
        missing_names = ", ".join(missing_essential)
        raise UnsupportedRigError(
            f"The provided rig does not match a supported humanoid skeleton: missing essential bones ({missing_names}). "
            f"Ensure the armature uses standard bone naming (Blender Rigify, Mixamo, or Disney Infinity convention)."
        )

    return mapped


# -----------------------------------------------------------------------------
# glTF / GLB Parser
# -----------------------------------------------------------------------------

def parse_gltf_rig(file_path: str | Path) -> ParsedRig:
    """Parse bone hierarchy and animation tracks from a glTF/GLB file."""
    path = Path(file_path)
    if not path.is_file():
        raise RigParsingError(f"File not found: {path}")

    # Read glTF JSON header
    gltf_data: dict[str, Any]
    bin_buffers: list[bytes] = []

    suffix = path.suffix.lower()
    if suffix == ".glb":
        raw = path.read_bytes()
        if len(raw) < 12 or raw[:4] != b"glTF":
            raise RigParsingError("Invalid GLB binary header.")
        version, length = int.from_bytes(raw[4:8], "little"), int.from_bytes(raw[8:12], "little")
        if version != 2:
            raise RigParsingError(f"Unsupported glTF version: {version} (expected version 2).")

        offset = 12
        while offset < len(raw):
            chunk_len = int.from_bytes(raw[offset:offset + 4], "little")
            chunk_type = raw[offset + 4:offset + 8]
            chunk_data = raw[offset + 8:offset + 8 + chunk_len]
            offset += 8 + chunk_len
            if chunk_type == b"JSON":
                try:
                    gltf_data = json.loads(chunk_data.decode("utf-8"))
                except Exception as exc:
                    raise RigParsingError(f"Failed to parse GLB JSON chunk: {exc}") from exc
            elif chunk_type == b"BIN\x00":
                bin_buffers.append(chunk_data)
    elif suffix in (".gltf", ".json"):
        try:
            gltf_data = json.loads(path.read_text(encoding="utf-8"))
        except Exception as exc:
            raise RigParsingError(f"Failed to parse glTF JSON: {exc}") from exc
        # Load external buffer if referenced
        for buf in gltf_data.get("buffers", []):
            uri = buf.get("uri")
            if uri and not uri.startswith("data:"):
                bin_path = path.parent / uri
                if bin_path.exists():
                    bin_buffers.append(bin_path.read_bytes())
                else:
                    bin_buffers.append(b"")
    else:
        raise RigParsingError(f"Unsupported rig file format: '{suffix}'. Expected .glb, .gltf, or .json")

    # Helper to read buffer views / accessors
    def read_accessor(acc_idx: int) -> np.ndarray:
        accessors = gltf_data.get("accessors", [])
        if acc_idx >= len(accessors):
            return np.array([])
        acc = accessors[acc_idx]
        bv_idx = acc.get("bufferView", 0)
        buffer_views = gltf_data.get("bufferViews", [])
        if bv_idx >= len(buffer_views):
            return np.array([])
        bv = buffer_views[bv_idx]
        buf_idx = bv.get("buffer", 0)
        byte_offset = bv.get("byteOffset", 0) + acc.get("byteOffset", 0)
        byte_length = bv.get("byteLength", 0)

        if buf_idx >= len(bin_buffers):
            return np.array([])
        raw_bytes = bin_buffers[buf_idx]
        count = acc.get("count", 0)
        comp_type = acc.get("componentType", 5126)  # 5126 = FLOAT
        type_str = acc.get("type", "SCALAR")

        type_counts = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}
        num_comps = type_counts.get(type_str, 1)

        dtype_map = {
            5120: np.int8, 5121: np.uint8,
            5122: np.int16, 5123: np.uint16,
            5125: np.uint32, 5126: np.float32,
        }
        dtype = dtype_map.get(comp_type, np.float32)
        total_items = count * num_comps
        item_size = np.dtype(dtype).itemsize

        data = np.frombuffer(raw_bytes, dtype=dtype, count=total_items, offset=byte_offset)
        if num_comps > 1:
            data = data.reshape(count, num_comps)
        return data

    # Check for skins / armatures
    skins = gltf_data.get("skins", [])
    nodes = gltf_data.get("nodes", [])

    if not nodes:
        raise UnsupportedRigError("glTF file contains no nodes.")

    joint_indices: list[int] = []
    inv_bind_matrices: np.ndarray | None = None
    skin_name = "Armature"

    if skins:
        skin = skins[0]
        skin_name = skin.get("name", "Armature")
        joint_indices = skin.get("joints", [])
        if "inverseBindMatrices" in skin and bin_buffers:
            inv_bind_matrices = read_accessor(skin["inverseBindMatrices"])
    else:
        # Check nodes marked as armature or child bones
        for i, node in enumerate(nodes):
            if "children" in node and any("name" in nodes[c] for c in node["children"]):
                joint_indices.append(i)
                joint_indices.extend(node["children"])
        joint_indices = list(dict.fromkeys(joint_indices))

    if not joint_indices:
        raise UnsupportedRigError("No skin or bone joints found in the glTF file. Check that the armature is exported.")

    # Build BoneNode list
    bone_nodes: list[BoneNode] = []
    node_to_bone_idx: dict[int, int] = {}

    for b_idx, node_idx in enumerate(joint_indices):
        if node_idx >= len(nodes):
            continue
        node = nodes[node_idx]
        b_name = node.get("name", f"bone_{node_idx}")
        node_to_bone_idx[node_idx] = b_idx

        # Translation, rotation, scale
        trans = np.array(node.get("translation", [0.0, 0.0, 0.0]), dtype=np.float32)
        rot = np.array(node.get("rotation", [0.0, 0.0, 0.0, 1.0]), dtype=np.float32)
        scale = np.array(node.get("scale", [1.0, 1.0, 1.0]), dtype=np.float32)

        if "matrix" in node:
            local_m = np.array(node["matrix"], dtype=np.float32).reshape(4, 4)
        else:
            local_m = compose_transform(trans, rot, scale)

        inv_bind = np.eye(4, dtype=np.float32)
        if inv_bind_matrices is not None and b_idx < len(inv_bind_matrices):
            inv_bind = inv_bind_matrices[b_idx].reshape(4, 4)

        bone = BoneNode(
            name=b_name,
            index=b_idx,
            local_translation=trans,
            local_rotation_quat=rot,
            local_scale=scale,
            local_matrix=local_m,
            inverse_bind_matrix=inv_bind,
        )
        bone_nodes.append(bone)

    # Establish parent-child relationships
    for node_idx, b_idx in node_to_bone_idx.items():
        node = nodes[node_idx]
        for child_node_idx in node.get("children", []):
            if child_node_idx in node_to_bone_idx:
                child_b_idx = node_to_bone_idx[child_node_idx]
                bone_nodes[b_idx].children.append(child_b_idx)
                bone_nodes[child_b_idx].parent_index = b_idx
                bone_nodes[child_b_idx].parent_name = bone_nodes[b_idx].name

    # Compute world matrices (hierarchical)
    def compute_world(idx: int, parent_m: np.ndarray):
        bone = bone_nodes[idx]
        bone.world_matrix = parent_m @ bone.local_matrix
        for c in bone.children:
            compute_world(c, bone.world_matrix)

    for b in bone_nodes:
        if b.parent_index is None:
            compute_world(b.index, np.eye(4, dtype=np.float32))

    # Parse animations
    animation_tracks: list[AnimationTrack] = []
    for anim in gltf_data.get("animations", []):
        anim_name = anim.get("name", "Animation")
        channels = anim.get("channels", [])
        samplers = anim.get("samplers", [])
        max_duration = 0.0

        track_by_bone: dict[str, BoneTrack] = {}

        for ch in channels:
            target = ch.get("target", {})
            target_node = target.get("node")
            target_path = target.get("path")
            sampler_idx = ch.get("sampler", 0)

            if target_node not in node_to_bone_idx or sampler_idx >= len(samplers):
                continue

            bone_idx = node_to_bone_idx[target_node]
            bone_name = bone_nodes[bone_idx].name
            sampler = samplers[sampler_idx]

            time_acc = sampler.get("input")
            value_acc = sampler.get("output")
            interp = sampler.get("interpolation", "LINEAR")

            if time_acc is None or value_acc is None or not bin_buffers:
                continue

            timestamps = read_accessor(time_acc)
            values = read_accessor(value_acc)

            if len(timestamps) == 0:
                continue

            if timestamps[-1] > max_duration:
                max_duration = float(timestamps[-1])

            if bone_name not in track_by_bone:
                track_by_bone[bone_name] = BoneTrack(bone_name, bone_idx)

            b_track = track_by_bone[bone_name]
            keys = [
                TransformKeyframe(time=float(t), value=np.array(v, dtype=np.float32), interpolation=interp)
                for t, v in zip(timestamps, values)
            ]

            if target_path == "translation":
                b_track.translations = keys
            elif target_path == "rotation":
                b_track.rotations = keys
            elif target_path == "scale":
                b_track.scales = keys

        if track_by_bone:
            fps = 30.0  # standard evaluation rate
            animation_tracks.append(AnimationTrack(
                name=anim_name,
                duration=max_duration,
                fps=fps,
                tracks=track_by_bone,
            ))

    bone_by_name = {b.name: b for b in bone_nodes}
    mapped_joints = map_and_validate_rig(bone_nodes)

    return ParsedRig(
        name=skin_name,
        bones=bone_nodes,
        bone_by_name=bone_by_name,
        mapped_canonical=mapped_joints,
        animations=animation_tracks,
    )


# -----------------------------------------------------------------------------
# JSON Rig / Animation Manifest Parser
# -----------------------------------------------------------------------------

def parse_json_rig(file_path: str | Path) -> ParsedRig:
    """Parse rig and animations from a JSON manifest file."""
    path = Path(file_path)
    if not path.is_file():
        raise RigParsingError(f"JSON rig file not found: {path}")

    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        raise RigParsingError(f"Malformed JSON rig file: {exc}") from exc

    bones_data = data.get("bones", [])
    if not isinstance(bones_data, list) or not bones_data:
        raise UnsupportedRigError("JSON rig does not contain a 'bones' list.")

    bone_nodes: list[BoneNode] = []
    name_to_idx: dict[str, int] = {}

    for i, b in enumerate(bones_data):
        name = b.get("name", f"bone_{i}")
        name_to_idx[name] = i
        trans = np.array(b.get("position", [0.0, 0.0, 0.0]), dtype=np.float32)
        rot = np.array(b.get("rotation", [0.0, 0.0, 0.0, 1.0]), dtype=np.float32)
        scale = np.array(b.get("scale", [1.0, 1.0, 1.0]), dtype=np.float32)
        local_m = compose_transform(trans, rot, scale)
        bone_nodes.append(BoneNode(
            name=name,
            index=i,
            local_translation=trans,
            local_rotation_quat=rot,
            local_scale=scale,
            local_matrix=local_m,
        ))

    for i, b in enumerate(bones_data):
        parent_name = b.get("parent")
        if parent_name and parent_name in name_to_idx:
            p_idx = name_to_idx[parent_name]
            bone_nodes[i].parent_index = p_idx
            bone_nodes[i].parent_name = parent_name
            bone_nodes[p_idx].children.append(i)

    # Hierarchical world matrices
    def compute_world(idx: int, parent_m: np.ndarray):
        bone = bone_nodes[idx]
        bone.world_matrix = parent_m @ bone.local_matrix
        for c in bone.children:
            compute_world(c, bone.world_matrix)

    for b in bone_nodes:
        if b.parent_index is None:
            compute_world(b.index, np.eye(4, dtype=np.float32))

    # Parse animations if present
    animation_tracks: list[AnimationTrack] = []
    for anim_data in data.get("animations", []):
        anim_name = anim_data.get("name", "Animation")
        duration = float(anim_data.get("duration", 1.0))
        fps = float(anim_data.get("fps", 30.0))
        tracks = {}
        for b_name, track_dict in anim_data.get("tracks", {}).items():
            if b_name not in name_to_idx:
                continue
            b_idx = name_to_idx[b_name]
            b_track = BoneTrack(b_name, b_idx)
            for kf in track_dict.get("positions", []):
                b_track.translations.append(TransformKeyframe(float(kf["time"]), np.array(kf["value"], dtype=np.float32)))
            for kf in track_dict.get("rotations", []):
                b_track.rotations.append(TransformKeyframe(float(kf["time"]), np.array(kf["value"], dtype=np.float32)))
            for kf in track_dict.get("scales", []):
                b_track.scales.append(TransformKeyframe(float(kf["time"]), np.array(kf["value"], dtype=np.float32)))
            tracks[b_name] = b_track

        animation_tracks.append(AnimationTrack(
            name=anim_name,
            duration=duration,
            fps=fps,
            tracks=tracks,
        ))

    bone_by_name = {b.name: b for b in bone_nodes}
    mapped_joints = map_and_validate_rig(bone_nodes)

    return ParsedRig(
        name=data.get("name", path.stem),
        bones=bone_nodes,
        bone_by_name=bone_by_name,
        mapped_canonical=mapped_joints,
        animations=animation_tracks,
    )


def _convert_and_load_fbx_bvh(path: Path) -> ParsedRig:
    import subprocess
    import tempfile
    from src.core.config import find_blender
    blender = find_blender()
    if not blender or not Path(blender).is_file():
        raise RigParsingError(f"Blender is required to import {path.suffix.upper()} files. Please install Blender or import as .glb / .gltf.")
    with tempfile.TemporaryDirectory(prefix="dic_anim_") as tmp_dir:
        out_glb = Path(tmp_dir) / "converted.glb"
        script = f"""
import bpy
bpy.ops.wm.read_factory_settings(use_empty=True)
p = {repr(str(path))}
if p.lower().endswith('.fbx'):
    bpy.ops.import_scene.fbx(filepath=p)
elif p.lower().endswith('.bvh'):
    bpy.ops.import_anim.bvh(filepath=p)
bpy.ops.export_scene.gltf(filepath={repr(str(out_glb))}, export_format='GLB', export_animations=True)
"""
        py_script = Path(tmp_dir) / "convert.py"
        py_script.write_text(script, encoding="utf-8")
        res = subprocess.run([str(blender), "-b", "-P", str(py_script)], capture_output=True, text=True,
                             creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        if not out_glb.exists():
            raise RigParsingError(f"Failed to convert {path.name} with Blender:\n{res.stderr or res.stdout}")
        return parse_gltf_rig(out_glb)


def load_rig_file(file_path: str | Path) -> ParsedRig:
    """Universal loader for rig and animation files (.glb, .gltf, .json, .fbx, .bvh)."""
    path = Path(file_path)
    suffix = path.suffix.lower()
    if suffix in (".glb", ".gltf"):
        return parse_gltf_rig(path)
    if suffix in (".fbx", ".bvh"):
        return _convert_and_load_fbx_bvh(path)
    if suffix == ".json":
        # Check if it's gltf json or custom json
        try:
            head = json.loads(path.read_text(encoding="utf-8"))
            if "asset" in head and "version" in head["asset"]:
                return parse_gltf_rig(path)
        except Exception:
            pass
        return parse_json_rig(path)
    raise RigParsingError(f"Unsupported file format '{suffix}'. Supported formats: .glb, .gltf, .json, .fbx, .bvh")


