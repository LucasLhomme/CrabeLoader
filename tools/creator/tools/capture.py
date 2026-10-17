"""Build the rig of one model without opening the app and save pictures of the result.

    python tools/capture.py model.glb [--base GOG_Gamora] [--blender path\\to\\blender.exe] [--out build\\captures]

Writes, for rest / arms up / walk, one PNG in the 3-views layout (front, side, top) with the stretched edges in red,
plus report.txt with the deformation score. Nothing is installed in the game."""
import argparse
import sys
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from src.core.base_character import BaseCharacter
from src.core.model import CharacterModel, auto_placement, load_source
from src.core.quality import deformation_report, stretched_segments
from src.core.rig import build_rig
from src.core.roster import find_entry
from src.core.skeleton import build_anchors, measure
from src.render.gl_widget import GLRenderer


class _Label:
    """Stands in for the Tk label: the renderer only needs somewhere to bind mouse events."""
    def bind(self, *args, **kwargs):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("model", type=Path)
    parser.add_argument("--base", default="GOG_Gamora")
    parser.add_argument("--blender", type=Path, default=None)
    parser.add_argument("--out", type=Path, default=Path("build") / "captures")
    parser.add_argument("--amount", type=float, default=1.0, help="pose amplitude, 1.0 = 100%%")
    args = parser.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)
    base = BaseCharacter.from_pack(find_entry(args.base))
    source = load_source(str(args.model))
    model = CharacterModel(source, auto_placement(source, base.fit_height))
    props, fit = measure(model.vertices(), model.faces, base)
    work = Path(tempfile.mkdtemp(prefix="di_capture_"))
    rig = build_rig(model, build_anchors(props, base), base, args.blender, work, print)
    report = deformation_report(rig, args.amount)

    renderer = GLRenderer(_Label())
    renderer.width, renderer.height = 1500, 700
    renderer._make_fbo()
    renderer.render = lambda: None  # no window to draw into: pictures come from snapshot()
    renderer.multi_view = True
    renderer.set_model(rig.tpose, rig.faces, rig.uvs, model.texture)
    renderer.frame_all()
    for pose, title in (("tpose", "Rest"), ("arms", "Arms up"), ("walk", "Walk")):
        renderer.update_model_vertices(rig.posed(pose, args.amount))
        renderer.set_heat_lines(stretched_segments(rig, pose, args.amount))
        image = renderer.snapshot()
        ImageDraw.Draw(image).text((10, 8), f"{title} - {args.amount:.0%} - {report.score}/100 ({report.level})",
                                   fill=(255, 255, 255))
        image.save(args.out / f"{pose}.png")
    text = fit.summary() + "\n" + report.summary()
    (args.out / "report.txt").write_text(text, encoding="utf-8")
    print(text)
    print(f"Pictures saved in {args.out.resolve()}")


if __name__ == "__main__":
    main()
