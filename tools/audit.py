"""Audit tool: build the rig of one model, play every animation on it and write a report to share.

    python tools/audit.py model.glb [--base TCW_Anakin] [--blender path\\to\\blender.exe] [--out build\\audit]

Writes in <out>\\<model name>\\: report.txt (static score + one line per animation), report.json (same, for tools)
and worst_<clip>.png for the animations that stretch the mesh (the worst frame, stretched edges in red).
Nothing is installed in the game and the Creator window is not needed."""
import argparse
import json
import re
import sys
import tempfile
from dataclasses import asdict
from pathlib import Path

from PIL import ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from src.core.animation_audit import audit_rig, pose_world, summary
from src.core.animation_player import get_character_animation_clips
from src.core.base_character import BaseCharacter
from src.core.model import CharacterModel, auto_placement, load_source
from src.core.quality import deformation_report, stretched_edges, unique_edges
from src.core.rig import build_rig, skin
from src.core.roster import find_entry
from src.core.skeleton import build_anchors, measure

FLAG_SHARE = 0.02
MAX_IMAGES = 4


class _Label:
    def bind(self, *args, **kwargs):
        pass


def slug(text: str) -> str:
    return re.sub(r"[^A-Za-z0-9]+", "_", text).strip("_")[:60]


def render_worst(rig, clip, frame, path: Path, title: str):
    """Picture of the clip's worst frame (front / side / top views), stretched edges in red."""
    from src.render.gl_widget import GLRenderer
    base = rig.base
    posed = skin(rig.baked, rig.bone_idx, rig.bone_w, base.bind_world, pose_world(clip, base, frame / clip.fps))
    edges = unique_edges(rig.faces)
    mask, _ = stretched_edges(rig.baked, posed, edges)
    renderer = GLRenderer(_Label())
    renderer.width, renderer.height = 1500, 700
    renderer._make_fbo()
    renderer.render = lambda: None
    renderer.multi_view = True
    renderer.set_model(rig.tpose, rig.faces, rig.uvs, None)
    renderer.frame_all()
    renderer.update_model_vertices(posed)
    renderer.set_heat_lines(posed[edges[mask]].reshape(-1, 3))
    image = renderer.snapshot()
    ImageDraw.Draw(image).text((10, 8), title, fill=(255, 255, 255))
    image.save(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("model", type=Path)
    parser.add_argument("--base", default="TCW_Anakin")
    parser.add_argument("--blender", type=Path, default=None)
    parser.add_argument("--out", type=Path, default=Path("build") / "audit")
    args = parser.parse_args()

    out = args.out / slug(args.model.stem)
    out.mkdir(parents=True, exist_ok=True)
    base = BaseCharacter.from_pack(find_entry(args.base))
    source = load_source(str(args.model))
    model = CharacterModel(source, auto_placement(source, base.fit_height))
    props, fit = measure(model.vertices(), model.faces, base)
    rig = build_rig(model, build_anchors(props, base), base, args.blender,
                    Path(tempfile.mkdtemp(prefix="di_audit_")), print)
    clips = get_character_animation_clips(base)
    results = audit_rig(rig, clips)
    text = "\n".join([f"Model: {args.model.name}   base: {args.base}", fit.summary(), "",
                      deformation_report(rig).summary(), "", summary(results)])
    (out / "report.txt").write_text(text, encoding="utf-8")
    (out / "report.json").write_text(json.dumps([asdict(r) for r in results], indent=2), encoding="utf-8")
    flagged = [r for r in results if r.broken_share > FLAG_SHARE or r.torn_share > 0.005][:MAX_IMAGES]
    for r in flagged:
        clip = clips[r.name]
        render_worst(rig, clip, r.worst_frame, out / f"worst_{slug(r.name)}.png",
                     f"{r.name} - frame {r.worst_frame}/{r.frames} - {100 * r.broken_share:.1f}% broken")
    print(text)
    print(f"\nReport saved in {out.resolve()}")


if __name__ == "__main__":
    main()
