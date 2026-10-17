"""Loading a downloaded 3D model and placing it the way the engine expects (Y up, facing +Z, feet on the ground)."""
import tempfile
import zipfile
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
import trimesh
from PIL import Image

MODEL_EXTENSIONS = (".glb", ".gltf", ".obj", ".dae", ".ply", ".stl", ".off")
MAX_TEXTURE = 2048
_extracted_dirs: list[tempfile.TemporaryDirectory] = []


@dataclass
class SourceModel:
    name: str
    vertices: np.ndarray
    faces: np.ndarray
    uvs: np.ndarray
    """Texture coordinates with a top-left origin (the engine's and PIL's convention)."""
    texture: Image.Image
    winding_sign: float
    """+1 when the triangle winding agrees with the file's normals, -1 when it is reversed."""
    report: list = field(default_factory=list)
    """Import warnings: materials without a texture (they come out flat grey), tiled UVs, etc."""


@dataclass
class Placement:
    upright_fix: bool = False
    """True for Z-up files: rotate them so +Y is up."""
    quarter_turns: int = 0
    """Yaw in 90 degree steps, so that the character faces +Z with its left hand on +X."""
    height: float = 2.13
    offset_x: float = 0.0
    offset_y: float = 0.0
    offset_z: float = 0.0
    """Manual nudges in metres (after auto-centring): +X = character's left, +Y = up, +Z = forward."""
    yaw_deg: float = 0.0
    """Fine yaw in degrees, added to the 90 degree steps."""


@dataclass
class CharacterModel:
    source: SourceModel
    placement: Placement = field(default_factory=Placement)
    ignored: np.ndarray | None = None
    """Per-vertex flag: accessories (cape, weapon...) left out of the measurements and skinned to follow the torso."""
    removed: np.ndarray | None = None
    """Per-vertex flag: parts left out of the model altogether."""
    arm_scale: float = 1.0
    """Arm thickness around the bone axis (1 = as modelled), applied when the rig is built."""
    leg_scale: float = 1.0
    torso_scale: float = 1.0
    forearm_scale: float = 1.0
    head_scale: float = 1.0
    """Head size, scaled around the neck (the neck follows halfway)."""
    """Chest and belly thickness around the spine (the pelvis follows halfway)."""
    look: float = 0.0
    """Game-look filter strength (0 = original texture)."""
    match: float = 0.0
    """How far (0-1) the average brightness is pulled towards the base figure's."""
    match_target: float | None = None
    """Average brightness of the base figure's texture."""
    _look_cache: tuple | None = field(default=None, repr=False, compare=False)
    _coverage: object = field(default=None, repr=False, compare=False)

    @property
    def faces(self) -> np.ndarray:
        faces = self.source.faces
        if self.removed is not None and self.removed.any():
            faces = faces[~self.removed[faces].any(axis=1)]
        return faces

    def measure_mask(self):
        """Vertices the measurements must not see (accessories and removed parts), or None."""
        parts = [m for m in (self.ignored, self.removed) if m is not None and m.any()]
        return np.logical_or.reduce(parts) if parts else None

    @property
    def uvs(self) -> np.ndarray:
        return self.source.uvs

    @property
    def texture(self) -> Image.Image:
        """The texture that is shown and exported: the original, or filtered towards the game's look."""
        use_match = self.match > 0 and self.match_target is not None
        if self.look <= 0 and not use_match:
            return self.source.texture
        key = (round(self.look, 3), round(self.match, 3), self.match_target if use_match else None)
        if self._look_cache is None or self._look_cache[0] != key:
            from src.core import gamelook
            self._look_cache = (key, gamelook.apply(self.source.texture, self.look, self.coverage(), self.match,
                                                    self.match_target))
        return self._look_cache[1]

    def coverage(self) -> np.ndarray:
        """Texels of the atlas the model actually uses (cached)."""
        if self._coverage is None:
            from src.core import gamelook
            self._coverage = gamelook.coverage(self.source.uvs, self.source.faces, self.source.texture.size)
        return self._coverage

    def texture_preview(self, size: int = 512) -> Image.Image:
        """Quick, lower-resolution version of `texture` for live slider feedback (about 0.1 s)."""
        from src.core import gamelook
        tex = self.source.texture
        scale = min(1.0, size / max(tex.size))
        small_size = (max(1, round(tex.width * scale)), max(1, round(tex.height * scale)))
        small = tex.convert("RGB").resize(small_size, Image.BILINEAR)
        mask = np.asarray(Image.fromarray(self.coverage()).resize(small_size, Image.NEAREST))
        return gamelook.apply(small, self.look, mask, self.match, self.match_target)

    def vertices(self) -> np.ndarray:
        return place(self.source.vertices, self.placement)


def pick_model_file(folder: Path) -> Path | None:
    candidates = [p for p in folder.rglob("*") if p.suffix.lower() in MODEL_EXTENSIONS]
    candidates.sort(key=lambda p: (MODEL_EXTENSIONS.index(p.suffix.lower()), -p.stat().st_size))
    return candidates[0] if candidates else None


def load_source(path: str | Path) -> SourceModel:
    path = Path(path)
    model_file = path
    if path.suffix.lower() == ".zip":
        tmp = tempfile.TemporaryDirectory(prefix="dic_model_")
        _extracted_dirs.append(tmp)
        with zipfile.ZipFile(path) as archive:
            archive.extractall(tmp.name)
        model_file = pick_model_file(Path(tmp.name))
        if model_file is None:
            raise ValueError(f"no 3D model ({', '.join(MODEL_EXTENSIONS)}) in {path.name}")
    elif path.suffix.lower() not in MODEL_EXTENSIONS:
        raise ValueError(f"unsupported format: {path.suffix} (accepted: .zip {' '.join(MODEL_EXTENSIONS)})")

    if model_file.suffix.lower() == ".dae":
        from src.core.dae import load as load_dae
        parts = load_dae(model_file)
    else:
        loaded = trimesh.load(model_file, process=False)
        parts = loaded.dump(concatenate=False) if isinstance(loaded, trimesh.Scene) else [loaded]
    parts = [p for p in parts if isinstance(p, trimesh.Trimesh) and len(p.faces)]
    if not parts:
        raise ValueError(f"{model_file.name} contains no triangles")

    report: list = []
    fallback = _folder_image(model_file.parent, model_file, path.suffix.lower() == ".zip")
    pieces = [_piece(p, i, report, fallback) for i, p in enumerate(parts)]
    atlas, rects = _pack_atlas([piece[3] for piece in pieces])
    vertices, faces, uvs, windings = [], [], [], []
    base = 0
    for (v, f, uv, _, winding), (rx, ry, rw, rh, pad) in zip(pieces, rects):
        u = (rx + pad + np.clip(uv[:, 0], 0, 1) * (rw - 2 * pad)) / atlas.width
        w = (ry + pad + np.clip(uv[:, 1], 0, 1) * (rh - 2 * pad)) / atlas.height
        vertices.append(v)
        faces.append(f + base)
        uvs.append(np.c_[u, w])
        windings.append((winding, len(f)))
        base += len(v)
    winding = float(np.sign(sum(s * n for s, n in windings))) or 1.0
    return SourceModel(model_file.stem, np.vstack(vertices), np.vstack(faces), np.vstack(uvs), atlas, winding, report)


IMAGE_EXTENSIONS = (".png", ".jpg", ".jpeg", ".tga", ".bmp", ".webp")


def _is_placeholder(image) -> bool:
    """trimesh invents a 2x2 grey image when an .obj points at a missing .mtl/texture."""
    return image is None or image.width * image.height <= 16


def _folder_image(folder: Path, model_file: Path, from_zip: bool):
    """A texture lying next to the model, for files whose material does not link to it.

    Inside a .zip every image counts; next to a loose file only an image with a similar name (or the only image)."""
    images = sorted((p for p in folder.rglob("*") if p.suffix.lower() in IMAGE_EXTENSIONS and p.is_file()),
                    key=lambda p: -p.stat().st_size)
    if not from_zip:
        images = [p for p in images if p.parent == folder]
        stem = model_file.stem.lower()
        similar = [p for p in images if stem[:6] in p.stem.lower() or p.stem.lower()[:6] in stem]
        images = similar or (images if len(images) == 1 else [])
    for path in images:
        try:
            return Image.open(path).convert("RGB"), path.name
        except OSError:
            continue
    return None


def _piece(mesh: trimesh.Trimesh, index: int = 0, report: list | None = None, fallback=None):
    """One material's worth of geometry: vertices, faces, top-left UVs, its texture tile and winding sign."""
    report = report if report is not None else []
    vertices = np.asarray(mesh.vertices, float)
    faces = np.asarray(mesh.faces, int)
    face_n = np.cross(vertices[faces[:, 1]] - vertices[faces[:, 0]], vertices[faces[:, 2]] - vertices[faces[:, 0]])
    normals = np.asarray(mesh.vertex_normals, float)
    winding = float(np.sign(np.median((face_n * normals[faces].mean(1)).sum(1)))) or 1.0
    visual = mesh.visual
    material = getattr(visual, "material", None)
    name = str(getattr(material, "name", None) or getattr(mesh, "metadata", {}).get("name") or f"part {index}")
    if visual.kind == "texture" and visual.uv is not None and len(visual.uv) == len(vertices):
        image = _material_image(material)
        if _is_placeholder(image) and fallback is not None:
            image = fallback[0]
            report.append(f"'{name}': material without texture, '{fallback[1]}' found next to the model and used")
        if image is not None and not _is_placeholder(image):
            uv = np.asarray(visual.uv, float)
            uv = np.c_[uv[:, 0], 1.0 - uv[:, 1]]
            image = _tint(image, material)
            image, uv = _tile_if_repeating(image, uv, name, report)
            return vertices, faces, uv, image, winding
        report.append(f"'{name}' ({len(faces)} triangles): no texture image found, flat colour used. "
                      "Put the .png/.tga files in the same .zip, or import with Blender cleaning enabled")
        color = _material_color(material)
    elif visual.kind in ("vertex", "face"):
        vertices, faces, uvs, image = _palette_texture(mesh, vertices, faces)
        return vertices, faces, uvs, image, winding
    else:
        report.append(f"'{name}' ({len(faces)} triangles): the file has no material/texture (check the original files "
                      f"for .png/.tga next to the model); flat grey used")
        color = _material_color(material)
    return vertices, faces, np.full((len(vertices), 2), 0.5), Image.new("RGB", (8, 8), color), winding


def _tint(image: Image.Image, material) -> Image.Image:
    """glTF multiplies the texture by baseColorFactor; apply it when it is not (almost) white."""
    factor = getattr(material, "baseColorFactor", None)
    if factor is None:
        return image
    c = np.asarray(factor, float).ravel()[:3]
    c = c / 255.0 if c.max() > 1.0 else c
    if c.min() > 0.97:
        return image
    return Image.fromarray(np.clip(np.asarray(image, np.float32) * c, 0, 255).astype(np.uint8))


def _tile_if_repeating(image: Image.Image, uv: np.ndarray, name: str, report: list):
    """UVs outside 0-1 mean the texture repeats. The atlas cannot repeat, so the tile is duplicated instead
    (clamping would smear the border pixels over the whole surface)."""
    lo, hi = np.floor(uv.min(0) + 0.01), np.ceil(uv.max(0) - 0.01)
    if np.all(lo >= 0) and np.all(hi <= 1):
        return image, uv
    nx, ny = (int(max(1, v)) for v in (hi[0] - lo[0], hi[1] - lo[1]))
    nx, ny = min(nx, 8), min(ny, 8)
    if (nx, ny) == (1, 1) and np.all(lo == 0):
        return image, np.clip(uv, 0, 1)
    big = Image.new("RGB", (image.width * nx, image.height * ny))
    for i in range(nx):
        for j in range(ny):
            big.paste(image, (i * image.width, j * image.height))
    limit = 2048
    if max(big.size) > limit:
        big = big.resize((min(big.width, limit), min(big.height, limit)), Image.LANCZOS)
    uv = (uv - lo) / np.array([nx, ny])
    if nx * ny > 1:
        report.append(f"'{name}': repeating texture ({nx}x{ny}), tiled")
    return big, np.clip(uv, 0, 1)


def _pack_atlas(images: list[Image.Image]) -> tuple[Image.Image, list[tuple[int, int, int, int, int]]]:
    """Shelf-pack every tile (edge-padded against bleeding) into one power-of-two RGB texture."""
    if len(images) == 1:
        return _power_of_two(images[0]), [(0, 0, *_power_of_two(images[0]).size, 0)]
    area = sum(im.width * im.height for im in images)
    side = int(min(MAX_TEXTURE, 2 ** int(np.ceil(np.log2(max(64, np.sqrt(area) * 1.2))))))
    scale = min(1.0, np.sqrt(side * side / (area * 1.4)))
    while True:
        tiles = [im.resize((max(4, int(im.width * scale)), max(4, int(im.height * scale))), Image.LANCZOS)
                 for im in images]
        pad = max(2, int(8 * scale))
        order = sorted(range(len(tiles)), key=lambda i: -tiles[i].height)
        rects, x, y, shelf = [None] * len(tiles), 0, 0, 0
        fits = True
        for i in order:
            w, h = tiles[i].width + 2 * pad, tiles[i].height + 2 * pad
            if x + w > side:
                x, y, shelf = 0, y + shelf, 0
            if w > side or y + h > side:
                fits = False
                break
            rects[i] = (x, y, w, h, pad)
            x, shelf = x + w, max(shelf, h)
        if fits:
            break
        scale *= 0.85
    atlas = np.zeros((side, side, 3), np.uint8)
    for tile, (x, y, w, h, p) in zip(tiles, rects):
        atlas[y:y + h, x:x + w] = np.pad(np.asarray(tile), ((p, p), (p, p), (0, 0)), mode="edge")
    used = max(r[1] + r[3] for r in rects)
    height = int(2 ** int(np.ceil(np.log2(max(used, 4)))))
    return Image.fromarray(atlas[:height]), rects


def _material_image(material):
    for attr in ("image", "baseColorTexture"):
        image = getattr(material, attr, None)
        if image is not None:
            return image.convert("RGB")
    return None


def _material_color(material):
    for attr in ("main_color", "baseColorFactor", "diffuse"):
        color = getattr(material, attr, None)
        if color is not None:
            c = np.asarray(color, float).ravel()[:3]
            return tuple(int(v if c.max() > 1.0 else v * 255) for v in c)
    return (200, 200, 200)


def _palette_texture(mesh, vertices, faces):
    """Per-face colours become cells of a small tile, one UV per triangle corner."""
    colors = np.asarray(mesh.visual.face_colors, np.uint8)[:, :3]
    unique, face_cell = np.unique(colors, axis=0, return_inverse=True)
    face_cell = face_cell.reshape(-1)
    side = int(np.ceil(np.sqrt(len(unique))))
    cell = 8
    image = np.zeros((side * cell, side * cell, 3), np.uint8)
    for i, c in enumerate(unique):
        r, q = divmod(i, side)
        image[r * cell:(r + 1) * cell, q * cell:(q + 1) * cell] = c
    corner_uv = np.stack([(face_cell % side + 0.5) / side, (face_cell // side + 0.5) / side], 1)
    new_vertices = vertices[faces].reshape(-1, 3)
    new_faces = np.arange(len(new_vertices)).reshape(-1, 3)
    uvs = np.repeat(corner_uv, 3, axis=0)
    return new_vertices, new_faces, uvs, Image.fromarray(image)


def _power_of_two(image: Image.Image) -> Image.Image:
    def p2(n):
        return int(min(MAX_TEXTURE, max(4, 2 ** int(np.ceil(np.log2(max(n, 1)))))))
    size = (p2(image.width), p2(image.height))
    return image if image.size == size else image.resize(size, Image.LANCZOS)


def _rotation(placement: Placement) -> np.ndarray:
    m = np.eye(3)
    if placement.upright_fix:
        m = m @ np.array([[1, 0, 0], [0, 0, -1], [0, 1, 0]], float)
    a = np.radians(90 * (placement.quarter_turns % 4) + placement.yaw_deg)
    yaw = np.array([[np.cos(a), 0, -np.sin(a)], [0, 1, 0], [np.sin(a), 0, np.cos(a)]])
    return m @ yaw


def place(vertices: np.ndarray, placement: Placement) -> np.ndarray:
    v = vertices @ _rotation(placement)
    span = np.ptp(v[:, 1])
    if span > 0:
        v = v * (placement.height / span)
    v[:, 1] -= v[:, 1].min()
    v[:, 0] -= (v[:, 0].min() + v[:, 0].max()) / 2
    h = v[:, 1].max()
    torso = (v[:, 1] > 0.3 * h) & (v[:, 1] < 0.7 * h) & (np.abs(v[:, 0]) < 0.15 * h)
    zs = v[torso, 2] if torso.sum() > 10 else v[:, 2]
    v[:, 2] -= (zs.min() + zs.max()) / 2
    v[:, 0] += placement.offset_x
    v[:, 1] += placement.offset_y
    v[:, 2] += placement.offset_z
    return v


def auto_placement(source: SourceModel, height: float) -> Placement:
    ext = np.ptp(source.vertices, axis=0)
    placement = Placement(upright_fix=bool(ext[1] < min(ext[0], ext[2]) * 0.8), height=height)
    v = place(source.vertices, placement)
    ext = np.ptp(v, axis=0)
    if ext[0] < ext[2]:
        placement.quarter_turns = 1
        v = place(source.vertices, placement)
    if not faces_forward(v):
        placement.quarter_turns += 2
    return placement


def faces_forward(v: np.ndarray) -> bool:
    """Feet stick out towards the front: compare how far the soles reach past the shins in +Z and -Z."""
    h = v[:, 1].max()
    soles = v[:, 1] < 0.06 * h
    shins = (v[:, 1] > 0.15 * h) & (v[:, 1] < 0.3 * h)
    if soles.sum() < 5 or shins.sum() < 5:
        return True
    forward = v[soles, 2].max() - v[shins, 2].max()
    backward = v[shins, 2].min() - v[soles, 2].min()
    return forward >= backward
