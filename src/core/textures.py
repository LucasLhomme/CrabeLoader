"""Textures: DDS encoding, per-character texture ids and the encrypted texture archive that carries them.

The engine reads textures/<hash>.tbody only from assets/textures/<first two hex>.zip and never falls back to a
loose file while that archive exists, so a mod ships a rebuilt copy of the whole prefix archive (the VFS serves it
in place of the vanilla one). Two mods shipping the same prefix archive would hide each other, so the prefix is
chosen among those no other installed mod uses.
"""
import hashlib
import io
import re
import shutil
import struct
import subprocess
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

from src.core.paths import BMS_SCRIPT, QUICKBMS, REENCRYPTER

ZIP_KEY = "681BBEEA63160188F9B7945104A51499"
FLAT_NORMAL_RGBA = (255, 127, 140, 127)
_NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)


@dataclass(frozen=True)
class TextureIds:
    prefix: str
    diffuse: str
    normal: str
    material_hash: bytes


def dds_bytes(img: Image.Image, fmt: str) -> bytes:
    levels, size = [], img.size
    cur = img
    while True:
        buf = io.BytesIO()
        cur.save(buf, "DDS", pixel_format=fmt)
        levels.append(buf.getvalue()[128:])
        if min(cur.size) <= 4:
            break
        cur = cur.resize((cur.size[0] // 2, cur.size[1] // 2), Image.LANCZOS)
    header = struct.pack("<4s7I44s2I4s5I5I", b"DDS ", 124, 0x000A1007, size[1], size[0],
                         len(levels[0]), 0, len(levels), b"\0" * 44,
                         32, 0x4, fmt.encode(), 0, 0, 0, 0, 0,
                         0x401008, 0, 0, 0, 0)
    return header + b"".join(levels)


def prefixes_used_by_other_mods(game_dir: Path, mod_name: str) -> set[str]:
    used = set()
    for archive in (Path(game_dir) / "mods").glob("*/textures/*.zip"):
        if archive.parent.parent.name.lower() != mod_name.lower():
            used.add(archive.stem.lower())
    return used


def choose_texture_ids(actor: str, game_dir: Path, mod_name: str) -> TextureIds:
    digest = hashlib.sha256(f"DisneyInfinityCreator/{actor.lower()}".encode()).hexdigest()
    taken = prefixes_used_by_other_mods(game_dir, mod_name)
    vanilla = Path(game_dir) / "assets" / "textures"
    prefix = None
    for i in range(0, len(digest) - 1, 2):
        candidate = digest[i:i + 2]
        if candidate not in taken and (vanilla / f"{candidate}.zip").exists():
            prefix = candidate
            break
    if prefix is None:
        raise RuntimeError("no free texture archive found in assets/textures")
    # Both halves of each id differ between the two textures and from anything else (engine cache keys).
    diffuse = prefix + digest[10:24]
    normal = prefix + digest[24:38]
    material_hash = bytes.fromhex(digest[38:46])
    return TextureIds(prefix, diffuse, normal, material_hash)


def _listing(archive: Path) -> set[str]:
    out = subprocess.run([str(QUICKBMS), "-l", str(BMS_SCRIPT), str(archive)],
                         capture_output=True, text=True, creationflags=_NO_WINDOW).stdout
    return {line.split()[2].split("\\", 1)[1].replace("\\", "/").lower() for line in out.splitlines()
            if re.match(r"\s+[0-9a-f]{8}\s+\d+\s+\S", line)}


def normal_or_flat(normal: Image.Image | None) -> Image.Image:
    """The relief normal map, or a flat one (the model then looks smooth, as before)."""
    return normal.convert("RGBA") if normal is not None else Image.new("RGBA", (256, 256), FLAT_NORMAL_RGBA)


def build_texture_archive(ids: TextureIds, diffuse: Image.Image, game_dir: Path, work: Path, out_zip: Path,
                          normal: Image.Image | None = None) -> int:
    """Rebuild <prefix>.zip = vanilla content + our two textures. Returns the number of entries."""
    for tool in (QUICKBMS, BMS_SCRIPT, REENCRYPTER):
        if not tool.exists():
            raise FileNotFoundError(f"missing tool: {tool}")
    vanilla = Path(game_dir) / "assets" / "textures" / f"{ids.prefix}.zip"
    shutil.rmtree(work, ignore_errors=True)
    extract = work / "extract"
    extract.mkdir(parents=True)
    subprocess.run([str(QUICKBMS), "-Y", str(BMS_SCRIPT), str(vanilla), str(extract)],
                   check=True, capture_output=True, creationflags=_NO_WINDOW)
    root = extract / ids.prefix
    (root / "textures").mkdir(parents=True, exist_ok=True)
    (root / "textures" / f"{ids.diffuse}.tbody").write_bytes(dds_bytes(diffuse.convert("RGB"), "DXT1"))
    (root / "textures" / f"{ids.normal}.tbody").write_bytes(dds_bytes(normal_or_flat(normal), "DXT5"))
    archive = work / f"{ids.prefix}.zip"
    subprocess.run([str(REENCRYPTER), "whyjustwhy", "-e", ZIP_KEY, str(root), str(archive)],
                   check=True, capture_output=True, creationflags=_NO_WINDOW)
    expected = _listing(vanilla) | {f"textures/{ids.diffuse}.tbody", f"textures/{ids.normal}.tbody"}
    got = _listing(archive)
    if got != expected:
        raise RuntimeError(f"invalid texture archive ({len(got)} entries, {len(expected)} expected)")
    out_zip.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(archive, out_zip)
    shutil.rmtree(work, ignore_errors=True)
    return len(got)


def export_loose_textures(ids: TextureIds, diffuse: Image.Image, mod_dir: Path,
                          normal: Image.Image | None = None) -> list[Path]:
    """Export diffuse and normal maps as loose .tbody DDS containers for CrabeLoader V2 VFS.

    CrabeLoader V2 hooks CreateFile via VfsOverrideManager and serves mods/<slug>/textures/<hash>.tbody
    directly in memory without repacking 50MB vanilla archives with offsetting.exe.
    """
    tex_dir = mod_dir / "textures"
    tex_dir.mkdir(parents=True, exist_ok=True)
    diffuse_path = tex_dir / f"{ids.diffuse}.tbody"
    normal_path = tex_dir / f"{ids.normal}.tbody"

    diffuse_path.write_bytes(dds_bytes(diffuse.convert("RGB"), "DXT1"))
    normal_path.write_bytes(dds_bytes(normal_or_flat(normal), "DXT5"))
    return [diffuse_path, normal_path]

