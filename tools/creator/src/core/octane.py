"""Build the clone's model files from the base figure's: vertex/index buffers, .oct, .bent and .dnax.

The .oct is edited as a scene (octane_scene), so sizes, offsets and names may change freely:
  * the vertex buffer is rebuilt mesh by mesh (stream 0 block, then stream 1 block, as the game lays it out);
    the custom mesh fills the body primitive, every other primitive shrinks to one degenerate triangle;
  * each primitive's Idata (index byte offset, first vertex, index count) and Vdata (vertex count, stream
    offsets) are rewritten; indices are relative to their mesh's first vertex;
  * the body mesh's bounds are recomputed; buffers, material bundle and materials are renamed after the clone,
    so the clone never shares a compiled material with the base.
The .bent and .dnax only have their model path repointed (the .dnax is compiled Lua, rewritten by luac).
"""
import re

import numpy as np

from src.core import luac
from src.core.base_character import S0_STRIDE, S1_STRIDE, BaseCharacter, Primitive
from src.core.octane_scene import Scene
from src.core.rig import RigResult

DEGENERATE_VERTS = 3


BODY_RENDER_CAPS = 512


class PatchError(RuntimeError):
    pass


def _streams(rig: RigResult) -> tuple[np.ndarray, np.ndarray]:
    """Stream 0 (position + half UV) and stream 1 (4 bones, 4 weights, half normal, half tangent) of the body."""
    n = len(rig.baked)
    s0 = np.zeros((n, S0_STRIDE), np.uint8)
    s0[:, :12] = rig.baked.astype("<f4").view(np.uint8).reshape(-1, 12)
    s0[:, 12:] = rig.uvs.astype("<f2").view(np.uint8).reshape(-1, 4)
    s1 = np.zeros((n, S1_STRIDE), np.uint8)
    s1[:, 0:4] = rig.bone_idx
    s1[:, 4:8] = rig.bone_w
    s1[:, 8:16] = np.c_[rig.normals, np.zeros(n)].astype("<f2").view(np.uint8).reshape(-1, 8)
    s1[:, 16:24] = np.c_[rig.tangents, rig.tangent_sign].astype("<f2").view(np.uint8).reshape(-1, 8)
    return s0, s1


def _degenerate(prim: Primitive, position: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Three identical vertices: whatever the skinning does to them, their triangle has no area."""
    s0 = np.zeros((DEGENERATE_VERTS, prim.s0_stride), np.uint8)
    s0[:, :12] = np.repeat(position[None].astype("<f4"), DEGENERATE_VERTS, 0).view(np.uint8).reshape(-1, 12)
    s1 = np.zeros((DEGENERATE_VERTS, prim.s1_stride), np.uint8)
    if prim.standard:
        s1[:, 4] = 255
    return s0, s1


def build_buffers(base: BaseCharacter, rig: RigResult):
    """Returns (vbuf, ibuf, layout) where layout maps each primitive to its new ranges."""
    body_s0, body_s1 = _streams(rig)
    park = (rig.baked.min(0) + rig.baked.max(0)) / 2
    vbuf, indices, layout = bytearray(), [], {}
    index_count = 0
    for mesh in base.meshes:
        blocks, first = [], 0
        for prim in mesh.primitives:
            if prim == base.body_primitive:
                s0, s1, faces = body_s0, body_s1, rig.faces
            else:
                s0, s1 = _degenerate(prim, park)
                faces = np.array([[0, 1, 2]])
            blocks.append((prim, first, s0, s1, faces))
            first += len(s0)
        if first > 0xFFFF:
            raise PatchError(f"mesh {mesh.name}: {first} vertices do not fit 16-bit indices")
        s0_start = len(vbuf)
        for _, _, s0, _, _ in blocks:
            vbuf += s0.tobytes()
        s1_start = len(vbuf)
        for _, _, _, s1, _ in blocks:
            vbuf += s1.tobytes()
        s0_pos, s1_pos = s0_start, s1_start
        for prim, first_vertex, s0, s1, faces in blocks:
            idx = (np.asarray(faces).reshape(-1) + first_vertex).astype("<u2")
            layout[prim] = dict(first_vertex=first_vertex, vertex_count=len(s0), index_offset=index_count * 2,
                                index_count=len(idx), s0_offset=s0_pos, s1_offset=s1_pos)
            indices.append(idx)
            index_count += len(idx)
            s0_pos += len(s0) * prim.s0_stride
            s1_pos += len(s1) * prim.s1_stride
    return bytes(vbuf), np.concatenate(indices).tobytes(), layout


def patch_oct(base: BaseCharacter, rig: RigResult, clone: str, vbuf: bytes, ibuf: bytes, layout) -> bytes:
    """clone: the clone's actor name (e.g. MAR_Mari); its files are named after clone.lower()."""
    lower = clone.lower()
    scene = base.load_scene()
    geometry = [n for n in scene.top.find("SceneTreeNodePool").children if n.get("Type") == "Geometry"]
    for mesh, node in zip(base.meshes, geometry):
        for prim, pnode in zip(mesh.primitives, node.find("Primitives").children):
            new = layout[prim]
            idata, vdata = pnode.find("Idata"), pnode.find("Vdata")
            idata.set_ints([idata.value[0], new["index_offset"], new["first_vertex"], new["index_count"]])
            v = vdata.value
            vdata.set_ints([v[0], new["vertex_count"], v[2], new["s0_offset"], v[4], v[5], new["s1_offset"], v[7]])
            if prim == base.body_primitive:
                # The body always uses the template's opaque body material: keep the standard render caps.
                # A base whose body primitive carries extra bits (Iron Fist: 1536) makes the engine pick another
                # render path (blended / extra pass), which is very expensive on the GPU.
                caps = pnode.find("RenderCaps")
                if caps is not None and caps.value != BODY_RENDER_CAPS:
                    caps.set_ints(BODY_RENDER_CAPS)

    body = geometry[base.body_mesh_index]
    pts = rig.baked
    lo, hi = pts.min(0), pts.max(0)
    center = (lo + hi) / 2
    body.find("BoundingBox").set_floats([*lo, *hi])
    body.find("BoundingSphereCenter").set_floats(center)
    body.find("BoundingSphereRadius").set_floats(float(np.linalg.norm(pts - center, axis=1).max()))
    body.find("BoundingOBBCenter").set_floats(center)
    body.find("BoundingOBBExtents").set_floats((hi - lo) / 2)

    for pool, size in (("VertexBufferPool", len(vbuf)), ("IndexBufferPool", len(ibuf))):
        named = [b for b in scene.top.find(pool).children if b.get("FileName")]
        if len(named) != 1:
            raise PatchError(f".oct: expected one file-backed buffer in {pool}, found {len(named)}")
        named[0].find("Size").set_ints(size)
        named[0].find("FileName").value = f"{lower}_0.{'vbuf' if pool.startswith('Vertex') else 'ibuf'}"
    for bundle in scene.top.find("MaterialBundlePool").children:
        bundle.find("FileName").value = f"{lower}.mtb"

    names = []
    for k, material in enumerate(scene.top.find("MaterialPool").children):
        stem = f"characters__{lower}__materials__{lower}_m{k}__skinning_isEnabled_true"
        material.find("Name").value = f"{clone}_M{k}"
        material.find("FileName").value = stem + ".mtl"
        names.append(stem)
    for node in geometry:
        for pnode in node.find("Primitives").children:
            pnode.find("MaterialName").value = names[pnode.get("MaterialReference")]
    return scene.to_bytes()


def material_count(base: BaseCharacter) -> int:
    return len(base.load_scene().top.find("MaterialPool").children)


def clone_bent(base: BaseCharacter, clone: str) -> bytes:
    lower = clone.lower()
    scene = Scene.load(base.file(".bent"))
    old = f"characters/{base.model_dir}/{base.model}.oct"
    changed = 0
    for node in scene.root.iter():
        if node.type in (0x05, 0x0B) and isinstance(node.value, str) and node.value.lower() == old:
            node.value = f"characters/{lower}/{lower}.oct"
            changed += 1
    if not changed:
        raise PatchError(f".bent: model path {old} not found")
    return scene.to_bytes()


def clone_dnax(base: BaseCharacter, clone: str) -> bytes:
    """Repoint the default model and the animtree overrides (any letter case) at the clone's folder."""
    model = f"characters/{base.model_dir}/{base.model}"
    animtree = base.animtree_file[0].lower() if base.animtree_file else None
    counts = {"model": 0}

    def rewrite(s: bytes) -> bytes | None:
        low = s.decode("latin-1").lower()
        if low == model:
            counts["model"] += 1
            return f"characters/{clone}/{clone}".encode()
        if animtree and low == animtree:
            return f"characters/{clone}/{clone}.animtreeoverrides".encode()
        return None

    data = base.dnax_file.read_bytes()
    if data.startswith(luac.HEADER[:4]):
        main = luac.load(data)
        luac.rewrite_strings(main, rewrite)
        out = luac.dump(main)
    else:  # Lua source (hand-edited installs)
        def sub(m):
            new = rewrite(m[1].encode("latin-1"))
            return f'"{new.decode()}"' if new is not None else m[0]
        out = re.sub(r'"([^"\r\n]*)"', sub, data.decode("latin-1")).encode("latin-1")
    if not counts["model"]:
        raise PatchError(f".dnax: model path {model} not found in {base.dnax_file.name}")
    return out
