"""Developer test: feed an official figure back into Creator and compare the result with the original.

The figure is taken from the base pack, raised into a T-pose with its own skeleton and weights (as a custom model
would arrive), then imported, auto-fitted and rigged onto itself like any model. A faithful pipeline gives back the
original: same shape in the game's rest pose, same skin weights. Whatever differs is Creator's own error, measured
on a model whose right answer is known.

Usage: python tools/roundtrip.py TCW_Anakin [--blender-weights heat_out.npz] [--out build/roundtrip]
Writes report.txt and compare.png (original | Creator, front and side) in the output folder.
"""
import argparse
import sys
import tempfile
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from PIL import Image  # noqa: E402

from src.core.base_character import S1_STRIDE, BaseCharacter  # noqa: E402
from src.core.model import CharacterModel, Placement, SourceModel  # noqa: E402
from src.core.rig import build_rig, skin  # noqa: E402
from src.core.roster import find_entry  # noqa: E402
from src.core.skeleton import build_anchors, measure  # noqa: E402

ARM_CHAIN = ("Shoulder", "Elbow", "Forearm", "Hand", "Thumb", "Index", "Middle", "Ring", "Pinky")


def official_skin(base: BaseCharacter):
    """Body vertices (bind pose), triangles, UVs and the shipped weights (indices into the palette, 0-1)."""
    prim = base.body_primitive
    verts, tris, uvs = base.primitive_geometry(prim)
    n = prim.vertex_count
    s1 = np.frombuffer(base._vbuf[prim.s1_offset:prim.s1_offset + n * S1_STRIDE], np.uint8).reshape(n, S1_STRIDE)
    return verts, tris, uvs, s1[:, 0:4].astype(int), s1[:, 4:8] / 255.0


def t_pose(base: BaseCharacter, verts, idx, w):
    """The figure with both arms raised to horizontal around the shoulders (its own skinning), and the per-side
    (pivot, rotation) used."""
    world = base.bind_world
    posed = world.copy()
    names = base.bone_names
    turns = {}
    for side, sx in (("Left", 1.0), ("Right", -1.0)):
        s = world[names.index(f"Shoulder_{side}")][3, :3]
        h = world[names.index(f"Hand_{side}")][3, :3]
        a = np.arctan2(h[1] - s[1], abs(h[0] - s[0]))  # below horizontal: negative
        best = None
        for sign in (1.0, -1.0):  # pick the rotation sense that brings the hand level with the shoulder
            ang = -a * sign * sx
            r = np.array([[np.cos(ang), np.sin(ang), 0], [-np.sin(ang), np.cos(ang), 0], [0, 0, 1]])
            if best is None or abs((h - s) @ r)[1] < best[0]:
                best = (abs(((h - s) @ r)[1]), r)
        r = best[1]
        turns[side] = (s, r)
        rot = np.eye(4)
        rot[:3, :3] = r
        rot[3, :3] = s - s @ r
        for i, n in enumerate(names):
            if n.endswith(side) and n.startswith(ARM_CHAIN):
                posed[i] = world[i] @ rot
    return skin(verts, idx, (w * 255).astype(np.uint8), world, posed), turns


ARM_JOINTS = ("Shoulder", "Elbow", "Hand", "HandTip", "Middle1")


def match_rows(a: np.ndarray, b: np.ndarray, uva: np.ndarray, uvb: np.ndarray) -> np.ndarray:
    """For each row of a, the row of b at the same place with the same UV (Creator welds and reorders)."""
    from scipy.spatial import cKDTree
    scale = np.ptp(b[:, 1]) or 1.0
    _, j = cKDTree(np.c_[b / scale, uvb * 0.05]).query(np.c_[a / scale, uva * 0.05])
    return j


def render(views, faces, uvs, texture, size=(420, 560)):
    try:
        import tkinter  # noqa: F401
    except ImportError:  # headless machine: the renderer only needs the module names
        import types
        for name in ("tkinter", "customtkinter"):
            stub = types.ModuleType(name)
            stub.__getattr__ = lambda attr: type(attr, (object,), {})
            sys.modules[name] = stub
    from src.render.gl_widget import GLRenderer

    class _Host:
        def bind(self, *a, **k):
            pass
    tiles = []
    for verts, yaw in views:
        r = GLRenderer(_Host())
        r.width, r.height = size
        r._make_fbo()
        r.render = lambda: None
        r.set_model(verts, faces, uvs, texture)
        r.frame_all()
        r.yaw, r.pitch = yaw, 5
        tiles.append(r.snapshot())
    out = Image.new("RGB", (size[0] * len(tiles), size[1]))
    for i, t in enumerate(tiles):
        out.paste(t, (size[0] * i, 0))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("actor")
    ap.add_argument("--blender-weights", type=Path, help="heat_out.npz computed by Blender for this run")
    ap.add_argument("--out", type=Path, default=Path("build/roundtrip"))
    ap.add_argument("--no-render", action="store_true")
    ap.add_argument("--true-skeleton", action="store_true", help="use the figure's own joints instead of the auto-fit")
    ap.add_argument("--official-weights", action="store_true", help="skin with the shipped weights instead of Creator's")
    args = ap.parse_args()
    out = args.out / args.actor
    out.mkdir(parents=True, exist_ok=True)

    base = BaseCharacter.from_pack(find_entry(args.actor))
    bind, tris, uvs, idx, w = official_skin(base)
    tposed, turns = t_pose(base, bind, idx, w)
    texture = Image.new("RGB", (64, 64), (180, 170, 160))
    src = SourceModel(args.actor, tposed, tris, uvs, texture, base.winding_sign)
    model = CharacterModel(src, Placement(height=base.fit_height))
    placed = model.vertices()
    props, _ = measure(placed, model.faces, base)
    anchors = build_anchors(props, base)
    k = np.ptp(placed[:, 1]) / np.ptp(tposed[:, 1])
    shift = placed.mean(0) - tposed.mean(0) * k
    if args.true_skeleton:
        for name, p in base.joints.items():
            if name not in anchors:
                continue
            p = np.asarray(p, float)
            head, _, side = name.partition("_")
            if side in turns and head in ARM_JOINTS:
                pivot, r = turns[side]
                p = (p - pivot) @ r + pivot
            anchors[name] = p * k + shift
    if args.official_weights:
        import src.core.rig as rig_module
        from scipy.spatial import cKDTree

        from src.core.skeleton import heat_bones
        from src.core.weight_transfer import _canonical_columns
        names_c = [b[0] for b in heat_bones()]
        cols = _canonical_columns(base, names_c)
        canon = np.zeros((len(bind), len(names_c)))
        for c in range(4):
            np.add.at(canon, (np.arange(len(bind)), cols[idx[:, c]]), w[:, c])

        def official(wverts, *a, **kw):
            _, j = cKDTree(placed).query(wverts)
            return names_c, canon[j], "official"
        rig_module.compute_weights = official

    blender = None
    if args.blender_weights:
        import src.core.rigging as rigging
        rigging.blender_heat = lambda *a, **k: np.load(args.blender_weights)["weights"].astype(float)
        blender = Path(sys.executable)
    rig = build_rig(model, anchors, base, blender, Path(tempfile.mkdtemp()), lambda s: None)

    # Placement moved and scaled the model: the original is expressed in the same frame (k, shift above).
    original = bind * k + shift
    rows = match_rows(rig.tpose, placed, rig.uvs, uvs)
    # The rig is baked into the base's own space: compare with the shipped vertices as they are.
    original = bind
    height = np.ptp(original[:, 1])
    err = np.linalg.norm(rig.baked - original[rows], axis=1) / height
    # Rest pose offset as a whole (the bake may put the figure a little higher or lower) vs shape error.
    offset = (rig.baked - original[rows]).mean(0)
    err_shape = np.linalg.norm(rig.baked - offset - original[rows], axis=1) / height

    names = base.bone_names
    def bone_weights(ix, wt):
        m = np.zeros((len(ix), len(names)))
        for c in range(4):
            np.add.at(m, (np.arange(len(ix)), ix[:, c]), wt[:, c])
        return m / np.maximum(m.sum(1, keepdims=True), 1e-9)
    ours = bone_weights(rig.bone_idx.astype(int), rig.bone_w / 255.0)
    theirs = bone_weights(idx[rows], w[rows])
    overlap = np.minimum(ours, theirs).sum(1)  # 1 = identical weights

    # Body zones by the bone that carries each vertex in the shipped figure.
    dominant = np.array([names[i] for i in idx[rows][np.arange(len(rows)), np.argmax(w[rows], axis=1)]])
    group = {"legs/feet": ("Thigh", "Knee", "Ankle", "Ball", "Toe"), "hips/torso": ("Hips", "Spine"),
             "shoulders": ("Clavicle", "Shoulder"), "forearms/hands": ("Elbow", "Forearm", "Hand", "Thumb", "Index",
                                                                       "Middle", "Ring", "Pinky"),
             "neck/head": ("Neck", "Head", "Jaw", "Eye", "Brow", "Lip", "Upper", "Lower")}
    zones = {z: np.array([d.startswith(p) for d in dominant]) for z, p in group.items()}
    lines = [f"Round trip of {base.display_name} ({args.actor}) through Creator",
             f"weights: {rig.method}{' (Blender)' if args.blender_weights else ''}, skeleton: "
             f"{'true joints' if args.true_skeleton else 'auto-fit'}",
             f"vertices {len(rig.baked)} (original {len(bind)})",
             "",
             "Shape in the game's rest pose (distance to the original, % of the height):",
             f"  mean {100 * err.mean():.2f}%  95th pct {100 * np.percentile(err, 95):.2f}%  max {100 * err.max():.2f}%",
             f"  whole-body offset {np.round(100 * offset / height, 2)}% -> shape only: mean {100 * err_shape.mean():.2f}%"
             f"  95th {100 * np.percentile(err_shape, 95):.2f}%",
             "  by zone (mean / 95th, shape only):"]
    for zone, m in zones.items():
        if m.any():
            lines.append(f"    {zone:16s} {100 * err_shape[m].mean():.2f}% / {100 * np.percentile(err_shape[m], 95):.2f}%")
    lines += ["", "Skin weights (share identical to the shipped ones, 100% = same):",
              f"  mean {100 * overlap.mean():.1f}%  vertices under 50%: {100 * (overlap < 0.5).mean():.1f}%"]
    for zone, m in zones.items():
        if m.any():
            lines.append(f"    {zone:16s} {100 * overlap[m].mean():.1f}%")
    lines += ["", "Skeleton auto-fit vs the figure's own joints (% of the height; arms are raised, so torso and legs):"]
    for name in ("Hips", "Spine2", "Spine3", "Neck", "Head", "Thigh_Left", "Knee_Left", "Ankle_Left"):
        if name in anchors and name in base.joints:
            d = (anchors[name] - (np.asarray(base.joints[name]) * k + shift)) / height * 100
            lines.append(f"    {name:12s} dx {d[0]:+.2f}  dy {d[1]:+.2f}  dz {d[2]:+.2f}")
    report = "\n".join(lines)
    (out / "report.txt").write_text(report + "\n", encoding="utf-8")
    np.savez(out / "data.npz", original=original[rows], creator=rig.baked, err=err_shape, overlap=overlap)
    print(report)
    if not args.no_render:
        img = render([(original[rows], 0), (rig.baked, 0), (original[rows], 90), (rig.baked, 90)], rig.faces, rig.uvs,
                     texture)
        img.save(out / "compare.png")
        print(f"images: {out / 'compare.png'} (original | Creator, front then side)")


if __name__ == "__main__":
    main()
