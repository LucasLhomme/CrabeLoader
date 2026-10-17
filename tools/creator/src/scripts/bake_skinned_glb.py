#!/usr/bin/env python3
"""Fige un modèle .glb rigué (skinné) en un modèle statique, SANS Blender.

Certains modèles (Sketchfab, etc.) sont déformés par leur squelette : lus par Blender,
ils sortent étirés. Ce script applique le calcul de skinning de la norme glTF, comme le
ferait un lecteur de .glb, puis écrit un .glb statique : un seul maillage, matériaux et
texture conservés, pieds au sol, centré, en mètres si le modèle semble être en cm.

Usage : python bake_skinned_glb.py modele.glb [sortie.glb] [--scale 0.01]
Python 3.8+ uniquement, aucune bibliothèque externe.
"""
import json
import math
import struct
import sys

CT = {5120: ("b", 1), 5121: ("B", 1), 5122: ("h", 2), 5123: ("H", 2), 5125: ("I", 4), 5126: ("f", 4)}
NC = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}
INT_MAX = {5120: 127, 5121: 255, 5122: 32767, 5123: 65535, 5125: 4294967295}


def read_glb(path):
    data = open(path, "rb").read()
    if data[:4] != b"glTF":
        raise SystemExit("Ce n'est pas un fichier .glb.")
    pos, js, binary = 12, None, b""
    while pos < len(data):
        length, kind = struct.unpack_from("<I4s", data, pos)
        body = data[pos + 8:pos + 8 + length]
        if kind == b"JSON":
            js = json.loads(body)
        elif kind.startswith(b"BIN"):
            binary = body
        pos += 8 + length
    return js, binary


def accessor(js, binary, index):
    a = js["accessors"][index]
    view = js["bufferViews"][a["bufferView"]]
    fmt, size = CT[a["componentType"]]
    n = NC[a["type"]]
    offset = view.get("byteOffset", 0) + a.get("byteOffset", 0)
    stride = view.get("byteStride", size * n)
    scale = 1.0 / INT_MAX[a["componentType"]] if a.get("normalized") else None
    rows = []
    for k in range(a["count"]):
        row = struct.unpack_from("<" + fmt * n, binary, offset + k * stride)
        rows.append(tuple(v * scale for v in row) if scale else row)
    return rows


def mat_mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def local_matrix(node):
    if "matrix" in node:
        m = node["matrix"]
        return [[m[c * 4 + r] for c in range(4)] for r in range(4)]
    tx, ty, tz = node.get("translation", (0, 0, 0))
    sx, sy, sz = node.get("scale", (1, 1, 1))
    x, y, z, w = node.get("rotation", (0, 0, 0, 1))
    r = [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
         [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
         [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]]
    return [[r[0][0] * sx, r[0][1] * sy, r[0][2] * sz, tx],
            [r[1][0] * sx, r[1][1] * sy, r[1][2] * sz, ty],
            [r[2][0] * sx, r[2][1] * sy, r[2][2] * sz, tz],
            [0, 0, 0, 1]]


def apply(m, v, w=1.0):
    return (m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3] * w,
            m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3] * w,
            m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3] * w)


def texture_key(js, mat):
    """Matériaux qui partagent la même texture de couleur -> même groupe."""
    if mat is None:
        return ("none",)
    tex = js["materials"][mat].get("pbrMetallicRoughness", {}).get("baseColorTexture")
    return ("tex", tex["index"]) if tex else ("mat", mat)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not args:
        raise SystemExit(__doc__)
    src = args[0]
    dst = args[1] if len(args) > 1 else src.rsplit(".", 1)[0] + "_baked.glb"
    forced = float(sys.argv[sys.argv.index("--scale") + 1]) if "--scale" in sys.argv else None
    if forced is not None and dst == str(forced):
        dst = src.rsplit(".", 1)[0] + "_baked.glb"

    js, binary = read_glb(src)
    nodes = js["nodes"]
    parent = {c: i for i, n in enumerate(nodes) for c in n.get("children", [])}
    cache = {}

    def world(i):
        if i not in cache:
            m = local_matrix(nodes[i])
            cache[i] = mat_mul(world(parent[i]), m) if i in parent else m
        return cache[i]

    skin_mats = {}
    for si, skin in enumerate(js.get("skins", [])):
        ibm = accessor(js, binary, skin["inverseBindMatrices"])
        mats = []
        for j, flat in zip(skin["joints"], ibm):
            inv = [[flat[c * 4 + r] for c in range(4)] for r in range(4)]
            mats.append(mat_mul(world(j), inv))
        skin_mats[si] = mats

    prims = []  # (positions, normals, uvs, indices, material)
    for ni, node in enumerate(nodes):
        if "mesh" not in node:
            continue
        for p in js["meshes"][node["mesh"]]["primitives"]:
            at = p["attributes"]
            pos = accessor(js, binary, at["POSITION"])
            nor = accessor(js, binary, at["NORMAL"]) if "NORMAL" in at else None
            uv = accessor(js, binary, at["TEXCOORD_0"]) if "TEXCOORD_0" in at else None
            idx = ([i[0] for i in accessor(js, binary, p["indices"])] if "indices" in p
                   else list(range(len(pos))))
            if "skin" in node and "JOINTS_0" in at:
                mats = skin_mats[node["skin"]]
                joints = accessor(js, binary, at["JOINTS_0"])
                weights = accessor(js, binary, at["WEIGHTS_0"])
                out_p, out_n = [], []
                for v, vp in enumerate(pos):
                    px = py = pz = nx = ny = nz = 0.0
                    for k in range(4):
                        wk = weights[v][k]
                        if wk <= 0:
                            continue
                        m = mats[int(joints[v][k])]
                        a = apply(m, vp)
                        px += wk * a[0]; py += wk * a[1]; pz += wk * a[2]
                        if nor:
                            b = apply(m, nor[v], 0.0)
                            nx += wk * b[0]; ny += wk * b[1]; nz += wk * b[2]
                    out_p.append((px, py, pz))
                    ln = math.sqrt(nx * nx + ny * ny + nz * nz) or 1.0
                    out_n.append((nx / ln, ny / ln, nz / ln))
            else:  # maillage non skinné : transformation du nœud
                m = world(ni)
                out_p = [apply(m, v) for v in pos]
                out_n = []
                for v in (nor or []):
                    b = apply(m, v, 0.0)
                    ln = math.sqrt(sum(c * c for c in b)) or 1.0
                    out_n.append(tuple(c / ln for c in b))
            prims.append((out_p, out_n if nor else None, uv, idx, p.get("material")))
    if not prims:
        raise SystemExit("Aucun maillage trouvé.")

    # Fusionne les parties qui partagent la même texture : un seul matériau, texture pleine résolution
    groups = {}
    for pr in prims:
        groups.setdefault(texture_key(js, pr[4]), []).append(pr)
    merged = []
    for group in groups.values():
        pos, nor, uv, idx = [], [], [], []
        has_n, has_uv = all(g[1] for g in group), all(g[2] for g in group)
        for p_, n_, u_, i_, _ in group:
            offset = len(pos)
            pos += p_
            idx += [i + offset for i in i_]
            if has_n:
                nor += n_
            if has_uv:
                uv += u_
        merged.append((pos, nor if has_n else None, uv if has_uv else None, idx, group[0][4]))
    prims = merged
    used = sorted({pr[4] for pr in prims if pr[4] is not None})
    remap = {m: i for i, m in enumerate(used)}

    allp = [v for pr in prims for v in pr[0]]
    lo = [min(v[i] for v in allp) for i in range(3)]
    hi = [max(v[i] for v in allp) for i in range(3)]
    size = [hi[i] - lo[i] for i in range(3)]
    print(f"Dimensions après skinning (X, Y, Z) : {size[0]:.2f} x {size[1]:.2f} x {size[2]:.2f}")
    scale = forced if forced is not None else (0.01 if size[1] > 20 else 1.0)
    cx, cz = (lo[0] + hi[0]) / 2, (lo[2] + hi[2]) / 2

    buf = bytearray()
    views, accs = [], []

    def add_view(data, target=None):
        while len(buf) % 4:
            buf.append(0)
        view = {"buffer": 0, "byteOffset": len(buf), "byteLength": len(data)}
        if target:
            view["target"] = target
        buf.extend(data)
        views.append(view)
        return len(views) - 1

    def add_acc(view, comp, count, kind, mn=None, mx=None):
        a = {"bufferView": view, "componentType": comp, "count": count, "type": kind}
        if mn is not None:
            a["min"], a["max"] = mn, mx
        accs.append(a)
        return len(accs) - 1

    out_prims = []
    for pos, nor, uv, idx, mat in prims:
        pp = [((x - cx) * scale, (y - lo[1]) * scale, (z - cz) * scale) for x, y, z in pos]
        attrs = {"POSITION": add_acc(
            add_view(b"".join(struct.pack("<3f", *v) for v in pp), 34962), 5126, len(pp), "VEC3",
            [min(v[i] for v in pp) for i in range(3)], [max(v[i] for v in pp) for i in range(3)])}
        if nor:
            attrs["NORMAL"] = add_acc(add_view(b"".join(struct.pack("<3f", *v) for v in nor), 34962),
                                      5126, len(nor), "VEC3")
        if uv:
            attrs["TEXCOORD_0"] = add_acc(add_view(b"".join(struct.pack("<2f", *v) for v in uv), 34962),
                                          5126, len(uv), "VEC2")
        prim = {"attributes": attrs, "mode": 4,
                "indices": add_acc(add_view(struct.pack(f"<{len(idx)}I", *idx), 34963), 5125, len(idx), "SCALAR")}
        if mat is not None:
            prim["material"] = remap[mat]
        out_prims.append(prim)

    images = []
    for img in js.get("images", []):
        v = js["bufferViews"][img["bufferView"]]
        start = v.get("byteOffset", 0)
        images.append({"bufferView": add_view(binary[start:start + v["byteLength"]]),
                       "mimeType": img.get("mimeType", "image/png")})

    out = {"asset": {"version": "2.0", "generator": "bake_skinned_glb.py"}, "scene": 0,
           "scenes": [{"nodes": [0]}], "nodes": [{"name": "Model", "mesh": 0}],
           "meshes": [{"name": "Model", "primitives": out_prims}],
           "materials": [js["materials"][m] for m in used], "textures": js.get("textures", []),
           "samplers": js.get("samplers", []), "images": images,
           "buffers": [{"byteLength": len(buf)}], "bufferViews": views, "accessors": accs}
    out = {k: v for k, v in out.items() if v != []}
    text = json.dumps(out, separators=(",", ":")).encode()
    text += b" " * (-len(text) % 4)
    buf.extend(b"\0" * (-len(buf) % 4))
    with open(dst, "wb") as f:
        f.write(struct.pack("<4sII", b"glTF", 2, 28 + len(text) + len(buf)))
        f.write(struct.pack("<I4s", len(text), b"JSON") + text)
        f.write(struct.pack("<I4s", len(buf), b"BIN\0") + bytes(buf))
    n_tri = sum(len(pr[3]) for pr in prims) // 3
    print(f"Échelle appliquée : {scale}  |  {len(allp)} sommets, {n_tri} triangles, "
          f"{len(out_prims)} matériau(x) après fusion")
    print(f"Hauteur finale : {size[1] * scale:.2f}  |  largeur : {size[0] * scale:.2f}  |  profondeur : {size[2] * scale:.2f}")
    print(f"Fichier écrit : {dst}")


if __name__ == "__main__":
    main()
