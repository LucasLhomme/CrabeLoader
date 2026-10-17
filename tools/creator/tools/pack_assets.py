"""Pack src/assets (game extracts, not versioned) into one archive for a cloud session.

A cloud session starts from the git repo only: without the base pack it cannot rig, audit or compare against the
game's figures. One archive moves the ~900 files in a single transfer.
Usage: python tools/pack_assets.py   ->   build/claude_sync/assets.zip
"""
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    src = ROOT / "src" / "assets"
    if not src.is_dir():
        sys.exit(f"{src} not found: build the assets first (see README, Ressources du jeu).")
    out = ROOT / "build" / "claude_sync" / "assets.zip"
    out.parent.mkdir(parents=True, exist_ok=True)
    files = [p for p in src.rglob("*") if p.is_file()]
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for p in files:
            z.write(p, p.relative_to(src).as_posix())
    print(f"{len(files)} files -> {out} ({out.stat().st_size / 1e6:.1f} MB)")


if __name__ == "__main__":
    main()
