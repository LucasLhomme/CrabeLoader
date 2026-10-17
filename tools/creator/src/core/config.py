import json
import shutil
from pathlib import Path

from src.core.paths import CONFIG_PATH


def find_blender() -> str:
    """Auto-detect blender.exe on the system."""
    bf = Path(r"C:\Program Files\Blender Foundation")
    if bf.is_dir():
        versions = sorted(bf.glob("*/blender.exe"), reverse=True)
        if versions:
            return str(versions[0])
    which = shutil.which("blender")
    if which:
        return which
    return ""


DEFAULTS = {
    "blender_path": find_blender(),
    "game_path": r"D:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition",
    "mod_name": "mario",
}


def normalize_blender_path(text: str) -> str:
    """Forgive typos in the blender.exe path: quotes, a folder instead of the file, a cut-off '.ex'."""
    text = (text or "").strip().strip('"').strip("'")
    if not text:
        return ""
    path = Path(text)
    candidates = [path, path / "blender.exe", Path(text + "e"), Path(text + ".exe")]
    for candidate in candidates:
        if candidate.is_file():
            return str(candidate)
    return text


def load_settings() -> dict:
    settings = dict(DEFAULTS)
    if CONFIG_PATH.exists():
        try:
            settings.update(json.loads(CONFIG_PATH.read_text(encoding="utf-8")))
        except (OSError, ValueError):
            pass
    bp = settings.get("blender_path", "")
    if not bp or not Path(bp).is_file():
        detected = find_blender()
        if detected:
            settings["blender_path"] = detected
    return settings


def save_settings(settings: dict) -> None:
    CONFIG_PATH.parent.mkdir(parents=True, exist_ok=True)
    CONFIG_PATH.write_text(json.dumps(settings, indent=4), encoding="utf-8")
