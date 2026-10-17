"""Check many models at once, without opening the app.

    python tools/batch_check.py models_folder [--base GOG_Gamora | --suggest] [--blender path\\to\\blender.exe]

For every .glb/.gltf/.obj/.zip in the folder: places it, builds the rig on the base figure and prints the deformation
score. With --suggest each model is tried on every figure and the best one is reported. Nothing is installed."""
import argparse
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from src.core.base_character import BaseCharacter
from src.core.model import MODEL_EXTENSIONS, CharacterModel, auto_placement, load_source
from src.core.quality import deformation_report
from src.core.ranking import rank_bases
from src.core.rig import build_rig
from src.core.roster import find_entry, load_roster
from src.core.skeleton import build_anchors, measure


def check_one(path: Path, base: BaseCharacter, blender, work: Path):
    source = load_source(str(path))
    model = CharacterModel(source, auto_placement(source, base.fit_height))
    props, _ = measure(model.vertices(), model.faces, base)
    rig = build_rig(model, build_anchors(props, base), base, blender, work, lambda text: None)
    return deformation_report(rig)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("folder", type=Path)
    parser.add_argument("--base", default="GOG_Gamora", help="base figure actor name (default: %(default)s)")
    parser.add_argument("--suggest", action="store_true", help="try every figure and report the best ones")
    parser.add_argument("--blender", type=Path, default=None, help="blender.exe for high-quality weights")
    args = parser.parse_args()

    files = sorted(p for p in args.folder.iterdir() if p.suffix.lower() in MODEL_EXTENSIONS or p.suffix.lower() == ".zip")
    if not files:
        sys.exit(f"No model found in {args.folder}")
    work = Path(tempfile.mkdtemp(prefix="di_batch_"))
    base = None if args.suggest else BaseCharacter.from_pack(find_entry(args.base))
    for path in files:
        try:
            if args.suggest:
                ranked = rank_bases(load_source(str(path)), load_roster(), work, blender=args.blender)[:3]
                best = ", ".join(f"{c.display_name} {c.score}/100" for c in ranked)
                print(f"{path.name}: best bases -> {best}")
            else:
                report = check_one(path, base, args.blender, work)
                print(f"{path.name}: {report.score}/100 ({report.level}) - "
                      + ", ".join(f"{p}: {c} stretched, worst x{w:.1f}" for p, (c, w) in report.per_pose.items()))
        except Exception as exc:  # one bad file must not stop the batch
            print(f"{path.name}: FAILED - {exc}")


if __name__ == "__main__":
    main()
