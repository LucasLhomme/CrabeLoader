"""The shipped figure a custom character is grafted onto: its files, meshes, skinning palette and joints.

Everything is read from the figure's own .oct (an Octane scene): each Geometry node of the scene tree is a mesh
with its primitives (vertex/index ranges in the shared _0.vbuf/_0.ibuf), its skinning palette (bone names and
inverse bind matrices) and its bounds. Every avatar uses the same vertex format: stream 0 is position + half UV
(16 B), stream 1 is 4 bone indices, 4 weights, half normal, half tangent (24 B).

The body mesh is the one whose palette best covers Creator's canonical joints; the custom model replaces its
largest primitive and every other primitive of the figure is emptied at export.
"""
import struct
from dataclasses import dataclass
from functools import cached_property
from pathlib import Path

import numpy as np

from src.core.game_files import GameInstall
from src.core.material_bundle import MaterialBundle
from src.core.octane_scene import Scene
from src.core.roster import RosterEntry, dnax_strings, resolve_model
from src.core.skeleton_map import SkeletonMap, map_skeleton

S0_STRIDE, S1_STRIDE = 16, 24
HEAD_TO_HEIGHT = 0.874  # Luke: head joint 1.858 / height 2.126


@dataclass(frozen=True)
class Primitive:
    mesh: int
    index: int
    material: int
    first_vertex: int
    vertex_count: int
    index_offset: int
    index_count: int
    s0_offset: int
    s1_offset: int
    s0_stride: int
    s1_stride: int

    @property
    def standard(self) -> bool:
        return (self.s0_stride, self.s1_stride) == (S0_STRIDE, S1_STRIDE)


@dataclass(frozen=True)
class Mesh:
    name: str
    primitives: tuple[Primitive, ...]
    bone_names: tuple[str, ...]
    bind_world: np.ndarray  # (B, 4, 4) bone-to-world, row-vector convention (translation in row 3)
    bbox: tuple[float, ...]

    @property
    def vertex_count(self) -> int:
        return sum(p.vertex_count for p in self.primitives)


def _geometry_nodes(scene: Scene):
    return [n for n in scene.top.find("SceneTreeNodePool").children if n.get("Type") == "Geometry"]


def _read_meshes(scene: Scene) -> list[Mesh]:
    meshes = []
    for mi, node in enumerate(_geometry_nodes(scene)):
        prims = []
        for pi, p in enumerate(node.find("Primitives").children):
            idata, vdata = p.get("Idata"), p.get("Vdata")
            if len(vdata) != 8 or vdata[0] != 2:
                raise ValueError(f"unsupported vertex streams {vdata} in mesh {node.get('MeshName')}")
            prims.append(Primitive(mi, pi, p.get("MaterialReference"), idata[2], vdata[1], idata[1], idata[3],
                                   vdata[3], vdata[6], vdata[4], vdata[7]))
        influences = node.find("Influences")
        names, world = [], []
        for inf in (influences.children if influences else []):
            inverse = np.array(inf.get("BindPoseSkinLocalToWorldMatrixInverseData"), float).reshape(4, 4)
            names.append(inf.get("Name"))
            world.append(np.linalg.inv(inverse))
        meshes.append(Mesh(node.get("MeshName") or node.get("NodeName") or f"mesh{mi}", tuple(prims), tuple(names),
                           np.array(world).reshape(-1, 4, 4), tuple(node.get("BoundingBox"))))
    return meshes


def _find_animtree(dnax_file: Path, files_dir: Path, game: GameInstall) -> tuple[str, Path] | None:
    """The animtree overrides the .dnax loads; it can belong to another model (Black Suit Spider-Man)."""
    for s in dnax_strings(dnax_file.read_bytes()):
        text = s.decode("latin-1")
        parts = text.split("/")
        if len(parts) == 3 and parts[0].lower() == "characters" and text.lower().endswith(".animtreeoverrides"):
            folder, stem = parts[1].lower(), parts[2][:-len(".animtreeoverrides")].lower()
            local = files_dir / f"{stem}.animtreeoverrides"
            if not local.exists():
                try:
                    local = game.model_files(folder, stem) / f"{stem}.animtreeoverrides"
                except FileNotFoundError:
                    continue
            if local.exists():
                return text, local
    return None


def read_texture_hashes(mtb: bytes) -> list[str]:
    """TEXB entries of a PC material bundle: 8-byte texture id + name offset (-1), in first-use order."""
    if mtb[:4] != b"BNDL" or mtb[16:20] != b"TEXB":
        return []
    count, name_table = struct.unpack_from("<2I", mtb, 28)
    start = 36 + name_table
    return [mtb[start + 12 * i:start + 12 * i + 8].hex() for i in range(count)]


class BaseCharacter:
    """A base figure. The app builds it from Creator's own pack (from_pack); tools/build_base_pack.py builds the
    pack from a game install (from_game)."""

    def __init__(self, entry: RosterEntry, model_dir: str, model: str, files_dir: Path, dnax_file: Path,
                 animtree_file: tuple[str, Path] | None = None):
        self.entry = entry
        self.model_dir, self.model = model_dir, model
        self.files_dir = files_dir
        self.dnax_file = dnax_file
        self.animtree_file = animtree_file
        """('characters/<dir>/<name>.animtreeoverrides' as the .dnax spells it, local copy), or None."""

    @classmethod
    def from_pack(cls, entry: RosterEntry) -> "BaseCharacter":
        from src.core.base_pack import load_base
        return load_base(cls, entry)

    @classmethod
    def from_game(cls, entry: RosterEntry, game: GameInstall) -> "BaseCharacter":
        model_dir, model = resolve_model(entry, game)
        files_dir = game.model_files(model_dir, model)
        dnax_file = game.dnax(entry.dnax)
        return cls(entry, model_dir, model, files_dir, dnax_file, _find_animtree(dnax_file, files_dir, game))

    # ---------------------------------------------------------------- identity
    @property
    def actor(self) -> str:
        return self.entry.actor

    @property
    def actor_lower(self) -> str:
        return self.entry.actor.lower()

    @property
    def display_name(self) -> str:
        return self.entry.display_name

    def file(self, suffix: str) -> Path:
        return self.files_dir / f"{self.model}{suffix}"

    def load_scene(self) -> Scene:
        """A fresh, editable copy of the figure's .oct."""
        return Scene.load(self.file(".oct"))

    def game_model_zip(self, game_dir) -> Path:
        return Path(game_dir) / "assets" / "characters" / self.model_dir / f"{self.model}.zip"

    # ------------------------------------------------------------------ meshes
    @cached_property
    def meshes(self) -> list[Mesh]:
        return _read_meshes(self.load_scene())

    @cached_property
    def _vbuf(self) -> bytes:
        return self.file("_0.vbuf").read_bytes()

    @cached_property
    def _ibuf(self) -> np.ndarray:
        return np.frombuffer(self.file("_0.ibuf").read_bytes(), "<u2")

    def primitive_geometry(self, prim: Primitive):
        """Vertices, triangles (local indices) and UVs of one primitive."""
        n, stride = prim.vertex_count, prim.s0_stride
        s0 = np.frombuffer(self._vbuf[prim.s0_offset:prim.s0_offset + n * stride], np.uint8).reshape(n, stride)
        verts = s0[:, :12].copy().view("<f4").reshape(n, 3).astype(float)
        uvs = s0[:, 12:16].copy().view("<f2").reshape(n, 2).astype(float)
        tris = self._ibuf[prim.index_offset // 2:prim.index_offset // 2 + prim.index_count].astype(int)
        tris = (tris - prim.first_vertex).reshape(-1, 3)
        return verts, tris, uvs

    def primitive_normals(self, prim: Primitive) -> np.ndarray:
        n = prim.vertex_count
        s1 = np.frombuffer(self._vbuf[prim.s1_offset:prim.s1_offset + n * S1_STRIDE], np.uint8).reshape(n, S1_STRIDE)
        return s1[:, 8:14].copy().view("<f2").astype(float)

    @cached_property
    def body_mesh_index(self) -> int:
        def score(i):
            mesh = self.meshes[i]
            m = map_skeleton(self._palette_positions(mesh))
            standard = max(mesh.primitives, key=lambda p: p.vertex_count).standard
            return (standard, m.supported, sum(s == "exact" for s in m.status.values()), mesh.vertex_count)
        return max(range(len(self.meshes)), key=score)

    @property
    def body(self) -> Mesh:
        return self.meshes[self.body_mesh_index]

    @cached_property
    def body_primitive(self) -> Primitive:
        return max(self.body.primitives, key=lambda p: p.vertex_count)

    @property
    def body_mesh(self):
        return self.primitive_geometry(self.body_primitive)

    @cached_property
    def winding_sign(self) -> float:
        verts, tris, _ = self.body_mesh
        normals = self.primitive_normals(self.body_primitive)
        face_n = np.cross(verts[tris[:, 1]] - verts[tris[:, 0]], verts[tris[:, 2]] - verts[tris[:, 0]])
        return float(np.sign(np.median((face_n * normals[tris].mean(1)).sum(1))))

    @property
    def height(self) -> float:
        return float(self.body.bbox[4])

    @property
    def fit_height(self) -> float:
        """Height of a humanly proportioned model whose head joint lands on this figure's (Luke: his own height).

        Big-headed figures (Mickey, Phineas) are much taller than their skeleton: their bounds would oversize the
        imported model.
        """
        head = self.joints.get("Head")
        return float(min(self.height, head[1] / HEAD_TO_HEIGHT)) if head is not None else self.height

    # ---------------------------------------------------------------- skeleton
    @staticmethod
    def _palette_positions(mesh: Mesh) -> dict[str, np.ndarray]:
        return {name: mesh.bind_world[i][3, :3].copy() for i, name in enumerate(mesh.bone_names)}

    @cached_property
    def skeleton(self) -> SkeletonMap:
        return map_skeleton(self._palette_positions(self.body))

    @property
    def supported(self) -> bool:
        return self.skeleton.supported and self.body_primitive.standard

    @property
    def bone_names(self) -> tuple[str, ...]:
        return self.body.bone_names

    @property
    def bind_world(self) -> np.ndarray:
        return self.body.bind_world

    @property
    def joints(self) -> dict[str, np.ndarray]:
        """Canonical joint -> bind-pose position (real, or estimated when the palette lacks that bone)."""
        return self.skeleton.joints

    def bone_index(self, name: str) -> int:
        """Palette index for a canonical joint (through its skin bone) or for a palette bone name."""
        return self.bone_names.index(self.skeleton.skin_bone.get(name, name))

    # ---------------------------------------------------------------- textures
    @cached_property
    def texture_hashes(self) -> list[str]:
        return read_texture_hashes(self.file(".mtb").read_bytes())

    @cached_property
    def material_textures(self) -> list[list[str]] | None:
        try:
            return MaterialBundle.from_bytes(self.file(".mtb").read_bytes()).material_textures()
        except ValueError:
            return None

    def preview_parts(self, load_texture) -> list[tuple]:
        """(verts, faces, uvs, texture) per visible primitive, for the viewport.

        Each primitive shows the first colour texture its material samples. For a shader whose layout is unknown,
        the largest colour texture of the bundle stands in.
        """
        cache = {}

        def image(h):
            if h not in cache:
                cache[h] = load_texture(h)
            return cache[h]

        def colour(ids):
            return next((img for h in ids if (img := image(h)) is not None and not looks_like_normal_map(img)), None)

        fallback = None
        sized = [(h, image(h)) for h in self.texture_hashes]
        sized = [img for _, img in sized if img is not None and not looks_like_normal_map(img)]
        if sized:
            fallback = max(sized, key=lambda img: img.width * img.height)
        per_material = self.material_textures
        parts = []
        for mesh in self.meshes:
            for prim in mesh.primitives:
                verts, tris, uvs = self.primitive_geometry(prim)
                if len(tris) == 0:
                    continue
                tex = None
                if per_material and 0 <= prim.material < len(per_material):
                    tex = colour(per_material[prim.material])
                parts.append((verts, tris, uvs, tex or fallback))
        return parts

    def deform_preview_parts(self, posed_world: np.ndarray) -> list[np.ndarray]:
        """Return posed vertex coordinates for each primitive in preview_parts."""
        from src.core.rig import skin

        if not hasattr(self, "_cached_prim_skin_data"):
            cache = []
            for mesh in self.meshes:
                bone_map = [self.bone_names.index(bn) if bn in self.bone_names else -1 for bn in mesh.bone_names]
                for prim in mesh.primitives:
                    verts, tris, _ = self.primitive_geometry(prim)
                    if len(tris) == 0:
                        continue
                    n = prim.vertex_count
                    if prim.standard and n > 0:
                        s1 = np.frombuffer(self._vbuf[prim.s1_offset:prim.s1_offset + n * S1_STRIDE], np.uint8).reshape(n, S1_STRIDE)
                        cache.append((verts, s1[:, 0:4], s1[:, 4:8], mesh.bind_world, bone_map))
                    else:
                        cache.append((verts, None, None, None, None))
            self._cached_prim_skin_data = cache

        posed_parts = []
        for verts, idx, wq, bind_world, bone_map in self._cached_prim_skin_data:
            if idx is None or bind_world is None:
                posed_parts.append(verts)
                continue
            mesh_posed_world = bind_world.copy()
            for i, bi in enumerate(bone_map):
                if 0 <= bi < len(posed_world):
                    mesh_posed_world[i] = posed_world[bi]
            posed_v = skin(verts, idx, wq, bind_world, mesh_posed_world)
            posed_parts.append(posed_v)
        return posed_parts


def looks_like_normal_map(img) -> bool:
    """Tangent-space normal maps: DXT5nm (red saturated, alpha = X) or plain (bluish, blue channel high)."""
    small = np.asarray(img.convert("RGBA").resize((16, 16)), float) / 255.0
    r, g, b, a = small[..., 0].mean(), small[..., 1].mean(), small[..., 2].mean(), small[..., 3].mean()
    swizzled = r > 0.9 and 0.35 < a < 0.65 and 0.35 < g < 0.65
    plain = b > 0.75 and 0.35 < r < 0.65 and 0.35 < g < 0.65
    return swizzled or plain
