"""Read-only access to the player's game install: model archives, loose .dnax files and textures, with a local cache.

Models and textures are read from the encrypted archives (assets/characters/<dir>/<model>.zip and
assets/textures/<xx>.zip) and extracted once into build/cache. Loose copies are only a fallback: an install can
carry hand-edited loose files that no longer match what the archives ship.
"""
import shutil
import subprocess
import tempfile
import threading
from pathlib import Path

from PIL import Image

from src.core.paths import BMS_SCRIPT, QUICKBMS, WORK_DIR

CACHE_DIR = WORK_DIR / "cache"
MODEL_SUFFIXES = (".oct", ".mtb", ".bent", ".animtreeoverrides", "_0.vbuf", "_0.ibuf")
_NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)
# The UI scans figures in the background while the player previews one: never fill a cache entry twice at once.
_CACHE_LOCK = threading.RLock()


class GameFileError(FileNotFoundError):
    pass


def _find_ci(folder: Path, name: str) -> Path | None:
    """`folder/name` matched without regard to letter case (the game's paths mix cases freely)."""
    direct = folder / name
    if direct.exists():
        return direct
    if not folder.is_dir():
        return None
    lower = name.lower()
    return next((p for p in folder.iterdir() if p.name.lower() == lower), None)


def _extract(archive: Path, pattern: str, out: Path) -> None:
    if not QUICKBMS.exists() or not BMS_SCRIPT.exists():
        raise GameFileError(f"QuickBMS is missing from {QUICKBMS.parent}")
    out.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(QUICKBMS), "-Y", "-f", pattern, str(BMS_SCRIPT), str(archive), str(out)],
                   check=True, capture_output=True, creationflags=_NO_WINDOW)


class GameInstall:
    def __init__(self, game_dir: str | Path):
        self.root = Path(game_dir)
        self.assets = self.root / "assets"

    def check(self) -> None:
        if not (self.assets / "characters").is_dir():
            raise GameFileError(f"{self.root} is not the Disney Infinity 3.0 folder (no assets/characters)")

    # ---------------------------------------------------------------- .dnax
    def dnax(self, rel_path: str) -> Path:
        """A .dnax named by the ActorList, e.g. 'characters/TS_Woody.dnax'."""
        rel = Path(rel_path)
        found = _find_ci(self.assets / rel.parent, rel.name)
        if found is None:
            raise GameFileError(f"{rel_path} not found in {self.assets}")
        return found

    # --------------------------------------------------------------- models
    def model_archive(self, model_dir: str, model: str) -> Path | None:
        folder = _find_ci(self.assets / "characters", model_dir)
        if folder is None:
            return None
        for name in (f"{model}.zip", f"{model}.zip.orig"):
            found = _find_ci(folder, name)
            if found is not None:
                return found
        return None

    def has_model(self, model_dir: str, model: str) -> bool:
        folder = _find_ci(self.assets / "characters", model_dir)
        return folder is not None and (self.model_archive(model_dir, model) is not None
                                       or _find_ci(folder, f"{model}.oct") is not None)

    def model_files(self, model_dir: str, model: str) -> Path:
        """Folder holding <model>.oct/.mtb/.bent/.animtreeoverrides/_0.vbuf/_0.ibuf, extracted on first use."""
        with _CACHE_LOCK:
            return self._model_files(model_dir, model.lower())

    def _model_files(self, model_dir: str, model: str) -> Path:
        cached = CACHE_DIR / "models" / model
        if all((cached / f"{model}{s}").exists() for s in MODEL_SUFFIXES if s != ".animtreeoverrides"):
            return cached
        archive = self.model_archive(model_dir, model)
        with tempfile.TemporaryDirectory(prefix="dic_model_") as tmp:
            if archive is not None:
                # The archive's inner root varies (model\, model\model\, model.zip\): match the names anywhere and
                # keep the shallowest hit, which skips same-named files deeper in characters\...\motions.
                _extract(archive, f"*{model}*", Path(tmp))
                hits = {}
                for p in sorted(Path(tmp).rglob("*"), key=lambda p: len(p.parts)):
                    if p.is_file():
                        hits.setdefault(p.name.lower(), p)
            else:
                folder = _find_ci(self.assets / "characters", model_dir)
                hits = {p.name.lower(): p for p in folder.iterdir() if p.is_file()} if folder else {}
            if f"{model}.oct" not in hits:
                raise GameFileError(f"no model files for characters/{model_dir}/{model}")
            cached.mkdir(parents=True, exist_ok=True)
            for suffix in MODEL_SUFFIXES:
                found = hits.get(f"{model}{suffix}")
                if found is not None:
                    shutil.copy2(found, cached / f"{model}{suffix}")
        return cached

    # ------------------------------------------------------------- textures
    def texture(self, texture_hash: str) -> Image.Image | None:
        """A texture by its 16-hex id, decoded to RGB(A); None when it is missing or in a format PIL can't read."""
        texture_hash = texture_hash.lower()
        cached = CACHE_DIR / "textures" / f"{texture_hash}.tbody"
        with _CACHE_LOCK:
            if not self._cache_texture(texture_hash, cached):
                return None
        try:
            img = Image.open(cached)
            img.load()
            return img
        except (OSError, ValueError):
            return None

    def _cache_texture(self, texture_hash: str, cached: Path) -> bool:
        if not cached.exists():
            archive = self.assets / "textures" / f"{texture_hash[:2]}.zip"
            loose = self.assets / "textures" / f"{texture_hash}.tbody"
            try:
                if archive.exists():
                    with tempfile.TemporaryDirectory(prefix="dic_tex_") as tmp:
                        _extract(archive, f"*{texture_hash}.tbody", Path(tmp))
                        hit = next(Path(tmp).rglob(f"{texture_hash}.tbody"), None)
                        if hit is not None:
                            cached.parent.mkdir(parents=True, exist_ok=True)
                            shutil.copy2(hit, cached)
                if not cached.exists() and loose.exists():
                    cached.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(loose, cached)
            except (OSError, subprocess.CalledProcessError):
                return False
        return cached.exists()
