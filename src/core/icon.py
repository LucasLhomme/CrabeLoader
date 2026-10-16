"""Character icon: a picture of the head, taken from the model itself, and its HUD files for CrabeLoader.

The HUD files follow CrabeLoader's own pipeline (tools/character_pipeline/make_icon.py): a DXT5 .tbody named after a
hash of the actor name, plus an 80-byte .mtb. Whether the selection grid picks them up depends on the game, so the
plain PNG is always written next to them."""
import hashlib
import struct
from pathlib import Path

import numpy as np
from PIL import Image

from src.core.textures import dds_bytes


def head_sphere(vertices: np.ndarray, neck_y: float, height: float) -> tuple[np.ndarray, float]:
    """Centre and radius of a sphere around the head: the vertices above the neck, ignoring things far to the side."""
    head = height - neck_y
    pts = vertices[vertices[:, 1] > neck_y]
    pts = pts[np.abs(pts[:, 0]) < 0.9 * head]
    if len(pts) == 0:
        return np.array([0.0, neck_y + head / 2, 0.0]), head * 0.6
    lo, hi = pts.min(axis=0), pts.max(axis=0)
    center = (lo + hi) / 2
    radius = max(float(np.linalg.norm(hi - lo)) / 2, head * 0.4) * 1.05
    return center, radius


def finish(raw: Image.Image, size: int = 128, background=(38, 60, 112)) -> Image.Image:
    """Head render (transparent background) -> square RGB icon on a flat colour with a round vignette."""
    raw = raw.convert("RGBA").resize((size, size), Image.LANCZOS)
    base = Image.new("RGBA", (size, size), background + (255,))
    yy, xx = np.mgrid[:size, :size]
    d = np.hypot(xx - size / 2 + 0.5, yy - size / 2 + 0.5) / (size / 2)
    shade = np.clip(1.15 - 0.35 * d, 0.6, 1.15)[..., None]
    arr = np.asarray(base, np.float32)
    arr[..., :3] = np.clip(arr[..., :3] * shade, 0, 255)
    out = Image.alpha_composite(Image.fromarray(arr.astype(np.uint8), "RGBA"), raw)
    return out.convert("RGB")


def _hud_hash(name: str) -> str:
    return hashlib.sha256(name.encode("utf-8")).hexdigest()[:16]


def hud_material(tex_hash_hex: str, name: str) -> bytes:
    header = bytearray(80)
    header[0:4] = b"MTB\0"
    header[4:8] = struct.pack("<I", 1)
    header[8:12] = b"MATP"
    header[12:16] = hashlib.sha1(name.encode("utf-8")).digest()[:4]
    header[16:32] = hashlib.md5(name.encode("utf-8")).digest()
    header[32:40] = bytes.fromhex(tex_hash_hex)
    header[48:52] = struct.pack("<I", 0x00010000)
    return bytes(header)


def write_icon(icon: Image.Image, actor: str, mod_dir: Path) -> list[Path]:
    """<actor>_icon.png in the mod folder, and the HUD texture + material in textures/."""
    tex_dir = mod_dir / "textures"
    tex_dir.mkdir(parents=True, exist_ok=True)
    png = mod_dir / f"{actor}_icon.png"
    icon.save(png)
    tex_hash = _hud_hash(actor)
    tbody = tex_dir / f"{tex_hash}.tbody"
    mtb = tex_dir / f"{actor}.mtb"
    tbody.write_bytes(dds_bytes(icon.convert("RGBA"), "DXT5"))
    mtb.write_bytes(hud_material(tex_hash, actor))
    return [png, tbody, mtb]
