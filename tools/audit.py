"""Build the rig of one model and audit it against every animation clip (no window, nothing installed).

    python tools/audit.py model.glb [--base TCW_Anakin] [--blender path\\to\\blender.exe]"""
import argparse
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from src.core.animation_audit import audit_rig, summary
from src.core.animation_player import get_character_animation_clips
from src.core.base_character import BaseCharacter
from src.core.model import CharacterModel, auto_placement, load_source
from src.core.rig import build_rig
from src.core.roster import find_entry
from src.core.skeleton import build_anchors, measure


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("model", type=Path)
    parser.add_argument("--base", default="GOG_Gamora")
    parser.add_argument("--blender", type=Path, default=None)
    args = parser.parse_args()
    base = BaseCharacter.from_pack(find_entry(args.base))
    source = load_source(str(args.model))
    model = CharacterModel(source, auto_placement(source, base.fit_height))
    props, _ = measure(model.vertices(), model.faces, base)
    rig = build_rig(model, build_anchors(props, base), base, args.blender, Path(tempfile.mkdtemp(prefix="di_audit_")), print)
    print(summary(audit_rig(rig, get_character_animation_clips(base))))


if __name__ == "__main__":
    main()
