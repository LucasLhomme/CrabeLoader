"""Creator's own copy of every base figure: the app never reads the player's game to preview or rig a base.

Layout of src/assets/bases/ (written by tools/build_base_pack.py):
  index.json               actor -> model folder/name, animtree path as the .dnax spells it, compatibility status
  <actor>/<model>.oct .mtb .bent _0.vbuf _0.ibuf     the model, as the game ships it
  <actor>/figure.dnax      the figure's .dnax;  <actor>/animtree.animtreeoverrides (when it has one)
  textures/<id>.jpg        colour textures the preview samples (at most PREVIEW_SIZE px, normal maps left out)
"""
import json
import shutil
from dataclasses import asdict, dataclass
from functools import lru_cache
from pathlib import Path

from PIL import Image

from src.core.paths import ASSETS_DIR
from src.core.roster import RosterEntry

PACK_DIR = ASSETS_DIR / "bases"
INDEX_PATH = PACK_DIR / "index.json"
PREVIEW_SIZE = 512
MODEL_FILES = (".oct", ".mtb", ".bent", "_0.vbuf", "_0.ibuf")


@dataclass(frozen=True)
class BaseStatus:
    state: str  # standard | adapted | unsupported | missing
    summary: str
    meshes: int = 0
    height: float = 0.0

    @property
    def usable(self) -> bool:
        return self.state in ("standard", "adapted")

    @property
    def mark(self) -> str:
        return {"standard": "●", "adapted": "◐", "unsupported": "✕"}.get(self.state, "?")


@lru_cache(maxsize=1)
def _index() -> dict:
    try:
        return json.loads(INDEX_PATH.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def has_pack() -> bool:
    return bool(_index())


def status(actor: str) -> BaseStatus:
    row = _index().get(actor)
    return BaseStatus(**row["status"]) if row else BaseStatus("missing", "not in Creator's base pack")


def load_base(cls, entry: RosterEntry):
    row = _index().get(entry.actor)
    if row is None:
        raise FileNotFoundError(f"{entry.display_name} is not in Creator's base pack ({PACK_DIR})")
    folder = PACK_DIR / entry.actor.lower()
    animtree = (row["animtree"], folder / "animtree.animtreeoverrides") if row.get("animtree") else None
    return cls(entry, row["model_dir"], row["model"], folder, folder / "figure.dnax", animtree)


def preview_texture(texture_id: str) -> Image.Image | None:
    path = PACK_DIR / "textures" / f"{texture_id.lower()}.jpg"
    try:
        img = Image.open(path)
        img.load()
        return img
    except (OSError, ValueError):
        return None


# ------------------------------------------------------------------ building (tools/build_base_pack.py)
def add_to_pack(base, status_row: BaseStatus, load_texture, is_normal_map) -> int:
    """Copy one base (built from a game install) into the pack; returns the bytes written."""
    folder = PACK_DIR / base.actor_lower
    shutil.rmtree(folder, ignore_errors=True)
    folder.mkdir(parents=True)
    written = 0
    for suffix in MODEL_FILES:
        shutil.copy2(base.file(suffix), folder / f"{base.model}{suffix}")
        written += (folder / f"{base.model}{suffix}").stat().st_size
    shutil.copy2(base.dnax_file, folder / "figure.dnax")
    if base.animtree_file:
        shutil.copy2(base.animtree_file[1], folder / "animtree.animtreeoverrides")
    textures = PACK_DIR / "textures"
    textures.mkdir(parents=True, exist_ok=True)
    for tid in base.texture_hashes:
        out = textures / f"{tid}.jpg"
        if out.exists():
            continue
        img = load_texture(tid)
        if img is None or is_normal_map(img):
            continue
        img = img.convert("RGB")
        img.thumbnail((PREVIEW_SIZE, PREVIEW_SIZE), Image.LANCZOS)
        img.save(out, quality=88)
        written += out.stat().st_size
    return written


def write_index(rows: dict[str, dict]) -> None:
    INDEX_PATH.parent.mkdir(parents=True, exist_ok=True)
    INDEX_PATH.write_text(json.dumps(rows, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    _index.cache_clear()


def index_row(base, status_row: BaseStatus) -> dict:
    return {"model_dir": base.model_dir, "model": base.model,
            "animtree": base.animtree_file[0] if base.animtree_file else None, "status": asdict(status_row)}
