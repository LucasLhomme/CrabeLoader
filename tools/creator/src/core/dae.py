"""Built-in COLLADA (.dae) reader: no Blender, no pycollada.

Reads the geometry (triangles, polylist, polygons), the first UV set, the diffuse texture or colour of each material,
the unit scale and the up axis. A skinned mesh is read in its bind pose (bind shape matrix, the joints are ignored),
a static mesh through the transform of its scene node. Returns one trimesh per material, like the .glb loader does."""
import re
import xml.etree.ElementTree as ET
from pathlib import Path

import numpy as np
import trimesh
from PIL import Image
from trimesh.visual import ColorVisuals, TextureVisuals
from trimesh.visual.material import SimpleMaterial


def _local(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def _children(node, name):
    return [c for c in node if _local(c.tag) == name]


def _child(node, name):
    return next(iter(_children(node, name)), None)


def _find_all(node, name):
    return [e for e in node.iter() if _local(e.tag) == name]


def _floats(text) -> np.ndarray:
    return np.array(text.split(), dtype=float) if text and text.strip() else np.zeros(0)


def _ref(value: str) -> str:
    return value[1:] if value and value.startswith("#") else (value or "")


def _source(mesh, source_id):
    """(array (n, stride)) of a <source>."""
    for src in _children(mesh, "source"):
        if src.get("id") != source_id:
            continue
        data = _floats(_child(src, "float_array").text)
        accessor = _find_all(src, "accessor")
        stride = int(accessor[0].get("stride", 1)) if accessor else 1
        return data.reshape(-1, stride)
    raise ValueError(f"source {source_id} not found")


def _node_matrix(node) -> np.ndarray:
    m = np.eye(4)
    for t in node:
        tag = _local(t.tag)
        v = _floats(t.text)
        if tag == "matrix" and len(v) == 16:
            m = m @ v.reshape(4, 4)
        elif tag == "translate" and len(v) == 3:
            k = np.eye(4)
            k[:3, 3] = v
            m = m @ k
        elif tag == "scale" and len(v) == 3:
            m = m @ np.diag([*v, 1.0])
        elif tag == "rotate" and len(v) == 4:
            ax, a = v[:3] / max(np.linalg.norm(v[:3]), 1e-12), np.radians(v[3])
            c, s, x, y, z = np.cos(a), np.sin(a), *ax
            k = np.eye(4)
            k[:3, :3] = [[c + x * x * (1 - c), x * y * (1 - c) - z * s, x * z * (1 - c) + y * s],
                         [x * y * (1 - c) + z * s, c + y * y * (1 - c), y * z * (1 - c) - x * s],
                         [x * z * (1 - c) - y * s, y * z * (1 - c) + x * s, c + z * z * (1 - c)]]
            m = m @ k
    return m


def _primitives(mesh, geometry_id):
    """Yield (material symbol, positions (n,3), uvs (n,2) or None, triangle corners (T,3) indices into them)."""
    vertices_id = {v.get("id"): _ref(_child(v, "input").get("source")) for v in _children(mesh, "vertices")}
    for prim in mesh:
        kind = _local(prim.tag)
        if kind not in ("triangles", "polylist", "polygons"):
            continue
        inputs = _children(prim, "input")
        stride = max(int(i.get("offset", 0)) for i in inputs) + 1
        pos_in = next(i for i in inputs if i.get("semantic") in ("VERTEX", "POSITION"))
        ref = _ref(pos_in.get("source"))
        pos_src = vertices_id.get(ref, ref)
        positions = _source(mesh, pos_src)[:, :3]
        uv_in = next((i for i in inputs if i.get("semantic") == "TEXCOORD" and int(i.get("set", 0)) == 0), None) or \
            next((i for i in inputs if i.get("semantic") == "TEXCOORD"), None)
        uvs = _source(mesh, _ref(uv_in.get("source")))[:, :2] if uv_in is not None else None
        if kind == "triangles":
            p = np.array(_child(prim, "p").text.split(), dtype=np.int64).reshape(-1, stride)
            corners = p.reshape(-1, 3, stride)
        else:
            counts = [int(c) for c in _child(prim, "vcount").text.split()] if kind == "polylist" else None
            if kind == "polylist":
                flat = np.array(_child(prim, "p").text.split(), dtype=np.int64).reshape(-1, stride)
                polys, at = [], 0
                for n in counts:
                    polys.append(flat[at:at + n])
                    at += n
            else:
                polys = [np.array(p.text.split(), dtype=np.int64).reshape(-1, stride) for p in _children(prim, "p")]
            tris = [np.stack([poly[0], poly[k], poly[k + 1]]) for poly in polys for k in range(1, len(poly) - 1)]
            corners = np.array(tris).reshape(-1, 3, stride)
        pos_idx = corners[:, :, int(pos_in.get("offset", 0))]
        uv_idx = corners[:, :, int(uv_in.get("offset", 0))] if uv_in is not None else None
        yield prim.get("material"), positions, uvs, pos_idx, uv_idx


def _texture_files(root):
    """image id -> file path string."""
    out = {}
    for image in _find_all(root, "image"):
        init = _child(image, "init_from")
        if init is not None and init.text:
            out[image.get("id")] = init.text.strip()
    return out


def _materials(root, folder: Path):
    """material id -> (image, rgba colour or None)."""
    files = _texture_files(root)
    effects = {e.get("id"): e for e in _find_all(root, "effect")}
    by_name = {p.name.lower(): p for p in folder.rglob("*") if p.is_file()}

    def load_image(ref):
        name = files.get(ref, ref)
        name = re.sub(r"^file:(//)?/?", "", name).replace("\\", "/")
        for candidate in (folder / name, by_name.get(Path(name).name.lower())):
            if candidate and Path(candidate).is_file():
                try:
                    return Image.open(candidate).convert("RGB")
                except OSError:
                    return None
        return None

    out = {}
    for material in _find_all(root, "material"):
        inst = _child(material, "instance_effect")
        effect = effects.get(_ref(inst.get("url"))) if inst is not None else None
        image, colour = None, None
        if effect is not None:
            diffuse = next(iter(_find_all(effect, "diffuse")), None)
            if diffuse is not None:
                tex = _child(diffuse, "texture")
                if tex is not None:
                    sampler = tex.get("texture")
                    # sampler -> surface -> image id (profile_COMMON newparams), or the image id directly
                    surface_id = sampler
                    for p in _find_all(effect, "newparam"):
                        if p.get("sid") == sampler and _child(p, "sampler2D") is not None:
                            surface_id = _child(_child(p, "sampler2D"), "source").text.strip()
                    for p in _find_all(effect, "newparam"):
                        if p.get("sid") == surface_id and _child(p, "surface") is not None:
                            surface_id = _child(_child(p, "surface"), "init_from").text.strip()
                    image = load_image(surface_id)
                color = _child(diffuse, "color")
                if color is not None:
                    v = _floats(color.text)
                    colour = (v if len(v) == 4 else np.r_[v, 1.0]) if len(v) else None
        out[material.get("id")] = (image, colour)
    return out


def load(path) -> list[trimesh.Trimesh]:
    path = Path(path)
    root = ET.parse(path).getroot()
    unit = _child(_child(root, "asset"), "unit") if _child(root, "asset") is not None else None
    scale = float(unit.get("meter", 1.0)) if unit is not None else 1.0
    up = _child(root, "asset")
    up_axis = (_child(up, "up_axis").text.strip() if up is not None and _child(up, "up_axis") is not None
               else "Y_UP")
    fix = np.eye(4)
    if up_axis == "Z_UP":
        fix[:3, :3] = [[1, 0, 0], [0, 0, 1], [0, -1, 0]]
    elif up_axis == "X_UP":
        fix[:3, :3] = [[0, 1, 0], [-1, 0, 0], [0, 0, 1]]
    fix[:3, :3] *= scale

    geometries = {g.get("id"): _child(g, "mesh") for g in _find_all(root, "geometry") if _child(g, "mesh") is not None}
    skins = {}
    for ctrl in _find_all(root, "controller"):
        skin = _child(ctrl, "skin")
        if skin is not None:
            bind = _child(skin, "bind_shape_matrix")
            skins[ctrl.get("id")] = (_ref(skin.get("source")),
                                     _floats(bind.text).reshape(4, 4) if bind is not None else np.eye(4))
    materials = _materials(root, path.parent)

    instances = []  # (geometry id, matrix applied to the row-vector positions, material symbol -> material id)

    def walk(node, parent):
        m = parent @ _node_matrix(node)
        for tag in ("instance_geometry", "instance_controller"):
            for inst in _children(node, tag):
                target = _ref(inst.get("url"))
                matrix = m
                if tag == "instance_controller":
                    if target not in skins:
                        continue
                    target, bind = skins[target]
                    matrix = bind  # skinned: the bind pose, the node transform does not apply
                symbols = {mi.get("symbol"): _ref(mi.get("target")) for mi in _find_all(inst, "instance_material")}
                instances.append((target, matrix, symbols))
        for child in _children(node, "node"):
            walk(child, m)

    scenes = _find_all(root, "visual_scene")
    for scene in scenes[:1]:
        for node in _children(scene, "node"):
            walk(node, np.eye(4))
    if not instances:  # no scene graph: every geometry as it is
        instances = [(gid, np.eye(4), {}) for gid in geometries]

    parts = []
    for gid, matrix, symbols in instances:
        mesh = geometries.get(gid)
        if mesh is None:
            continue
        for symbol, positions, uvs, pos_idx, uv_idx in _primitives(mesh, gid):
            world = (np.c_[positions, np.ones(len(positions))] @ matrix.T) @ fix.T
            image, colour = materials.get(symbols.get(symbol, symbol), (None, None))
            # One vertex per distinct (position, uv) value: exporters often give every corner its own index.
            if uvs is not None and uv_idx is not None:
                corner = np.c_[world[pos_idx.ravel(), :3], uvs[uv_idx.ravel()]]
            else:
                corner = world[pos_idx.ravel(), :3]
            unique, first, inverse = np.unique(np.round(corner, 6), axis=0, return_index=True, return_inverse=True)
            verts = world[pos_idx.ravel()[first], :3]
            uv = uvs[uv_idx.ravel()[first]] if uvs is not None and uv_idx is not None else None
            faces = inverse.reshape(-1, 3)
            tm = trimesh.Trimesh(verts, faces, process=False)
            tm.metadata["name"] = symbol or gid
            if image is not None and uv is not None:
                tm.visual = TextureVisuals(uv=uv, material=SimpleMaterial(image=image))
            else:
                rgba = np.clip((colour if colour is not None else np.array([0.8, 0.8, 0.8, 1.0])) * 255, 0, 255)
                tm.visual = ColorVisuals(tm, face_colors=np.tile(rgba.astype(np.uint8), (len(faces), 1)))
            parts.append(tm)
    if not parts:
        raise ValueError(f"{path.name}: no mesh found in this COLLADA file")
    return parts
