"""One shared mod for what every Creator character needs in the game's archives: textures and icons.

The engine reads character textures only from assets/textures/<first two hex>.zip and UI pictures only from
assets/ui/icons/icons.zip and assets/ui/virtualreadercharacters/virtualreadercharacters.zip; a mod has to ship a
rebuilt copy of the whole archive (vanilla content + its own files). Shipped per character, that is ~13 MB of
texture archive each, and the icon archive (125 MB) could only be shipped by one mod at a time.

So each character mod keeps only its own small source files (textures/<id>.tbody, ui/...), and the mod
`zz_creator_shared` holds the rebuilt archives, made once from the sources of every enabled Creator mod:
  - textures/17.zip: all new characters' textures (17 = the smallest vanilla texture archive, 5 MB);
    characters made before keep their own prefix until `migrate_prefix` moves them;
  - ui/icons/icons.zip, ui/virtualreadercharacters/virtualreadercharacters.zip: every character's icons.
Removing or disabling a character and calling `rebuild` leaves nothing of it behind. manifest.json records what
each archive was built from, so an archive is rebuilt only when its sources changed."""
import hashlib
import json
import shutil
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable

from src.core.paths import REENCRYPTER, WORK_DIR

SHARED_SLUG = "zz_creator_shared"
SHARED_PREFIX = "17"
"""Texture archive of new characters: the smallest vanilla one (assets/textures/17.zip, ~5 MB)."""
LEGACY_UI_SLUG = "zz_creator_ui"
"""First version of the shared icons (built by hand): replaced by SHARED_SLUG."""
UI_ARCHIVES = ("icons", "virtualreadercharacters")
ZIP_KEY = "681BBEEA63160188F9B7945104A51499"
GAME_EXE = "DisneyInfinity3.exe"
_NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)
Log = Callable[[str], None]


@dataclass
class CreatorMod:
    slug: str
    path: Path
    enabled: bool
    name: str = ""
    actor: str = ""
    size: int = 0
    textures: dict[str, Path] = field(default_factory=dict)  # texture id -> .tbody
    ui: dict[str, dict[str, Path]] = field(default_factory=dict)  # archive -> path inside it -> file
    own_archives: list[Path] = field(default_factory=list)  # per-character archives of the old layout

    @property
    def prefixes(self) -> set[str]:
        return {t[:2] for t in self.textures}


def _is_creator(mod_json: Path) -> dict | None:
    try:
        data = json.loads(mod_json.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None
    mid = str(data.get("id", ""))
    if not mid.startswith("creator.") or mid in ("creator.ui", "creator.shared"):
        return None
    return data


def creator_mods(game_dir: Path) -> list[CreatorMod]:
    """Every character mod made by Creator, enabled or disabled (CrabeLoader skips folders starting with '_')."""
    out = []
    mods = Path(game_dir) / "mods"
    if not mods.is_dir():
        return out
    for folder in sorted(mods.iterdir(), key=lambda p: p.name.lower().lstrip("_")):
        if not folder.is_dir() or folder.name.lstrip("_") in (SHARED_SLUG, LEGACY_UI_SLUG):
            continue
        data = _is_creator(folder / "mod.json")
        if data is None:
            continue
        char = data.get("character") or {}
        mod = CreatorMod(folder.name.lstrip("_"), folder, not folder.name.startswith("_"),
                         str(char.get("displayName") or data.get("name", folder.name)), str(char.get("name", "")))
        for f in folder.rglob("*"):
            if f.is_file():
                mod.size += f.stat().st_size
        tex = folder / "textures"
        if tex.is_dir():
            mod.textures = {f.stem.lower(): f for f in tex.glob("*.tbody")}
            mod.own_archives = sorted(tex.glob("*.zip"))
        for archive in UI_ARCHIVES:
            root = folder / "ui" / archive
            if root.is_dir():
                files = {f"{archive}/{f.relative_to(root).as_posix()}".lower(): f for f in root.rglob("*") if f.is_file()}
                if files:
                    mod.ui[archive] = files
        if not mod.actor:
            mod.actor = next((p.stem for p in (folder / "characters").glob("*.lua")), "")
        out.append(mod)
    return out


def game_running() -> bool:
    try:
        out = subprocess.run(["tasklist", "/FI", f"IMAGENAME eq {GAME_EXE}", "/NH"], capture_output=True, text=True,
                             creationflags=_NO_WINDOW).stdout
    except OSError:
        return False
    return GAME_EXE.lower() in out.lower()


def _stamp(files: dict[str, Path], vanilla: Path) -> str:
    h = hashlib.sha256()
    st = vanilla.stat()
    h.update(f"{vanilla.name}|{st.st_size}|{st.st_mtime_ns}\n".encode())
    for rel in sorted(files):
        s = files[rel].stat()
        h.update(f"{rel}|{s.st_size}|{s.st_mtime_ns}\n".encode())
    return h.hexdigest()[:16]


def _build(vanilla: Path, stem: str, files: dict[str, Path], out: Path, work: Path, log: Log) -> int:
    """out = vanilla archive + files (paths inside the archive, '<stem>/...'). Returns the number of entries.

    Only the Creator files go through offsetting.exe (a small archive, which also gives the game's name hashes);
    the game's own entries are copied byte for byte (octane_zip), so nothing is extracted or re-encrypted."""
    from src.core import octane_zip
    if not REENCRYPTER.exists():
        raise FileNotFoundError(f"missing tool: {REENCRYPTER}")
    stage = work / "stage" / stem
    shutil.rmtree(stage.parent, ignore_errors=True)
    stage.mkdir(parents=True)
    for rel, src in files.items():
        dst = stage.parent / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
    small, tmp = work / f"{stem}_creator.zip", work / f"{stem}.zip"
    small.unlink(missing_ok=True)
    subprocess.run([str(REENCRYPTER), "whyjustwhy", "-e", ZIP_KEY, str(stage), str(small)], check=True,
                   capture_output=True, creationflags=_NO_WINDOW)
    octane_zip.merge(vanilla, small, tmp, work / "digests" / f"{stem}.json")
    # Check: every game file is still there and every Creator file reads back identical.
    got = {e.name.lower(): e for e in octane_zip.read(tmp)}
    expected = {e.name.lower() for e in octane_zip.read(vanilla)} | {r.split("/", 1)[1] for r in files}
    if set(got) != expected:
        raise RuntimeError(f"{stem}.zip: {len(got)} entries, {len(expected)} expected")
    for rel, src in files.items():
        if got[rel.split("/", 1)[1]].data() != src.read_bytes():
            raise RuntimeError(f"{stem}.zip: {rel} does not read back identical")
    out.parent.mkdir(parents=True, exist_ok=True)
    shutil.move(str(tmp), str(out))
    small.unlink(missing_ok=True)
    shutil.rmtree(stage.parent, ignore_errors=True)
    shutil.rmtree(work / "vanilla", ignore_errors=True)  # extraction cache of the previous method
    return len(got)


@dataclass
class RebuildReport:
    built: list[str] = field(default_factory=list)
    kept: list[str] = field(default_factory=list)
    removed: list[str] = field(default_factory=list)
    freed: int = 0


def plan(game_dir: Path, mods: list[CreatorMod] | None = None) -> dict[str, tuple[Path, Path, dict[str, Path]]]:
    """Archive name -> (vanilla archive, output path, files to add) for the enabled Creator mods."""
    game_dir = Path(game_dir)
    mods = creator_mods(game_dir) if mods is None else mods
    shared = game_dir / "mods" / SHARED_SLUG
    targets: dict[str, tuple[Path, Path, dict[str, Path]]] = {}
    for mod in (m for m in mods if m.enabled):
        for tid, path in mod.textures.items():
            p = tid[:2]
            vanilla = game_dir / "assets" / "textures" / f"{p}.zip"
            if not vanilla.is_file():
                continue
            entry = targets.setdefault(f"textures/{p}", (vanilla, shared / "textures" / f"{p}.zip", {}))
            entry[2][f"{p}/textures/{tid}.tbody"] = path
        for archive, files in mod.ui.items():
            vanilla = game_dir / "assets" / "ui" / archive / f"{archive}.zip"
            entry = targets.setdefault(f"ui/{archive}", (vanilla, shared / "ui" / archive / f"{archive}.zip", {}))
            entry[2].update(files)
    return targets


def manifest_path(game_dir: Path) -> Path:
    return Path(game_dir) / "mods" / SHARED_SLUG / "manifest.json"


def read_manifest(game_dir: Path) -> dict:
    try:
        return json.loads(manifest_path(game_dir).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {"archives": {}}


def out_of_date(game_dir: Path) -> list[str]:
    """Archives that a rebuild would change (empty = the shared mod matches the installed characters)."""
    game_dir = Path(game_dir)
    mods = creator_mods(game_dir)
    targets = plan(game_dir, mods)
    built = read_manifest(game_dir).get("archives", {})
    stale = [n for n, (v, out, files) in targets.items() if built.get(n, {}).get("stamp") != _stamp(files, v)
             or not out.is_file()]
    stale += [n for n in built if n not in targets]
    stale += [f"{m.slug}: own archive" for m in mods if m.enabled and m.own_archives]  # rebuild removes them
    if (game_dir / "mods" / LEGACY_UI_SLUG).is_dir():
        stale.append(LEGACY_UI_SLUG)
    return stale


def rebuild(game_dir: Path, log: Log = print, force: bool = False) -> RebuildReport:
    """Bring zz_creator_shared in line with the enabled Creator mods, then remove what it replaces."""
    game_dir = Path(game_dir)
    if game_running():
        raise RuntimeError("Close the game first: it keeps its archives open.")
    report = RebuildReport()
    shared = game_dir / "mods" / SHARED_SLUG
    mods = creator_mods(game_dir)
    targets = plan(game_dir, mods)
    manifest = read_manifest(game_dir)
    built = manifest.setdefault("archives", {})
    work = WORK_DIR / "shared"
    for name, (vanilla, out, files) in sorted(targets.items()):
        stamp = _stamp(files, vanilla)
        if not force and built.get(name, {}).get("stamp") == stamp and out.is_file():
            report.kept.append(name)
            continue
        log(f"Building {name}.zip ({len(files)} file(s) of Creator characters)...")
        entries = _build(vanilla, Path(name).name, files, out, work, log)
        owners = sorted({m.slug for m in mods if m.enabled and any(p in files.values() for p in
                                                                    list(m.textures.values()) + [f for d in m.ui.values()
                                                                                                 for f in d.values()])})
        built[name] = {"stamp": stamp, "entries": entries, "added": len(files), "mods": owners}
        report.built.append(name)
    for name in [n for n in built if n not in targets]:  # nothing uses it any more: the vanilla one is enough
        entry = built.pop(name)
        path = shared / f"{name}.zip" if name.startswith("textures/") else shared / name / f"{Path(name).name}.zip"
        if path.is_file():
            report.freed += path.stat().st_size
            path.unlink()
        report.removed.append(f"{name}.zip (used by {', '.join(entry.get('mods', [])) or 'no character'})")
    # The shared archives now carry everything: the per-character copies and the hand-made icon mod go.
    for mod in mods:
        if not mod.enabled:
            continue
        for archive in mod.own_archives:
            stem = archive.stem.lower()
            if f"textures/{stem}" not in built and stem in mod.prefixes:
                continue  # its textures are not in a shared archive yet: left alone
            # Either the shared archive carries its textures, or they moved to the shared prefix (migrate_prefix).
            report.freed += archive.stat().st_size
            archive.unlink()
            report.removed.append(f"{mod.slug}/textures/{archive.name}")
    legacy = game_dir / "mods" / LEGACY_UI_SLUG
    if legacy.is_dir():
        report.freed += sum(f.stat().st_size for f in legacy.rglob("*") if f.is_file())
        shutil.rmtree(legacy)
        report.removed.append(LEGACY_UI_SLUG)
    if built:
        shared.mkdir(parents=True, exist_ok=True)
        (shared / "mod.json").write_text(json.dumps({
            "manifestVersion": 1, "id": "creator.shared", "name": "Creator shared archives", "version": "1.0.0",
            "description": "Textures and icons of the Disney Infinity Creator characters (rebuilt game archives). "
                           "Managed by Manage_Mods.bat: do not edit by hand.",
            "minLoaderVersion": "0.2.0", "entry": "shared.lua"}, indent=2) + "\n", encoding="utf-8")
        (shared / "shared.lua").write_text('print("[creator_shared] shared texture and icon archives loaded")\n',
                                           encoding="utf-8")
        manifest_path(game_dir).write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    elif shared.is_dir():
        shutil.rmtree(shared)
        report.removed.append(SHARED_SLUG)
    for line in summary(report):
        log(line)
    return report


def summary(report: RebuildReport) -> list[str]:
    lines = []
    if report.built:
        lines.append(f"Shared archives rebuilt: {', '.join(report.built)}")
    if report.kept:
        lines.append(f"Already up to date: {', '.join(report.kept)}")
    for r in report.removed:
        lines.append(f"Removed: {r}")
    if report.freed:
        lines.append(f"Space freed: {report.freed / 1e6:.1f} MB")
    return lines


def migrate_prefix(mod: CreatorMod, prefix: str = SHARED_PREFIX) -> int:
    """Move an older character's textures into the shared prefix: new ids in its .mtb and .tbody names.
    Returns the number of textures moved (its old per-character archive is removed by the next rebuild)."""
    from src.core.material_bundle import MaterialBundle
    moved = {tid: prefix + tid[2:] for tid in mod.textures if tid[:2] != prefix}
    if not moved:
        return 0
    for mtb in (mod.path / "characters").rglob("*.mtb"):
        bundle = MaterialBundle.from_bytes(mtb.read_bytes())
        ids = [t.hex() for t in bundle.textures]
        if any(i in moved for i in ids):
            bundle.textures = [bytes.fromhex(moved.get(i, i)) for i in ids]
            mtb.write_bytes(bundle.to_bytes())
    for old, new in moved.items():
        mod.textures[old].rename(mod.textures[old].with_name(f"{new}.tbody"))
    return len(moved)


def move_to_recycle_bin(path: Path) -> None:
    """Windows recycle bin (the user can restore it); plain removal elsewhere."""
    import sys
    if sys.platform != "win32":
        shutil.rmtree(path)
        return
    import ctypes
    from ctypes import wintypes

    class SHFILEOPSTRUCTW(ctypes.Structure):
        _fields_ = [("hwnd", wintypes.HWND), ("wFunc", wintypes.UINT), ("pFrom", wintypes.LPCWSTR),
                    ("pTo", wintypes.LPCWSTR), ("fFlags", ctypes.c_uint16), ("fAnyOperationsAborted", wintypes.BOOL),
                    ("hNameMappings", ctypes.c_void_p), ("lpszProgressTitle", wintypes.LPCWSTR)]
    FO_DELETE, FOF_ALLOWUNDO, FOF_NOCONFIRMATION, FOF_SILENT = 3, 0x40, 0x10, 0x4
    op = SHFILEOPSTRUCTW(None, FO_DELETE, str(Path(path).resolve()) + "\0", None,
                         FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT, False, None, None)
    if ctypes.windll.shell32.SHFileOperationW(ctypes.byref(op)) != 0 or Path(path).exists():
        raise OSError(f"could not move {path} to the recycle bin")
