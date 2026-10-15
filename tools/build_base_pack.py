"""Developer script: build Creator's base pack (src/assets/bases/) from a Disney Infinity 3.0 install.

Every figure of src/assets/roster.json gets its model files, .dnax, animtree overrides and preview textures copied
into the pack, with its compatibility status, so the app previews and rigs bases without reading any game files.
Usage: python tools/build_base_pack.py "<game folder>"
"""
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from src.core.base_character import BaseCharacter, looks_like_normal_map  # noqa: E402
from src.core.base_pack import BaseStatus, add_to_pack, index_row, write_index  # noqa: E402
from src.core.game_files import GameInstall  # noqa: E402
from src.core.roster import load_roster  # noqa: E402


def evaluate(base: BaseCharacter) -> BaseStatus:
    skeleton = base.skeleton
    if not base.supported:
        state = "unsupported"
    else:
        state = "adapted" if skeleton.fallbacks else "standard"
    return BaseStatus(state, skeleton.summary(), len(base.meshes), round(base.fit_height, 3))


def main(game_dir: str) -> None:
    game = GameInstall(game_dir)
    game.check()
    rows, total, t0 = {}, 0, time.time()
    for entry in load_roster():
        try:
            base = BaseCharacter.from_game(entry, game)
            status = evaluate(base)
            total += add_to_pack(base, status, game.texture, looks_like_normal_map)
            rows[entry.actor] = index_row(base, status)
            print(f"{status.mark} {entry.display_name:28} {status.summary[:80]}")
        except Exception as exc:  # one broken figure must not stop the others
            print(f"? {entry.display_name:28} skipped: {exc}")
    write_index(rows)
    print(f"{len(rows)} figures, {total / 1e6:.1f} MB, {time.time() - t0:.0f} s")


if __name__ == "__main__":
    main(sys.argv[1])
