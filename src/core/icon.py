"""Character icons: pictures taken from the model itself, written as the game's own UI files.

The game keeps two kinds of character pictures, both as a tiny material bundle (.mtb) pointing to a DDS texture:
  - ui/icons/hud_playericons_<name>.mtb: the 128 px head portrait (HUD, selection grid). A character row names it
    in its Icon field ("HUD_PlayerIcons_<name>"), which CrabeLoader fills from mod.json ("character" > "icon").
  - ui/virtualreadercharacters/<actor>.mtb, <actor>_sm.mtb, <actor>_s.mtb: the Virtual Reader figure (512 px),
    its small version (256 px) and its glowing silhouette (512 px).
The bundles follow the layout of the shipped files (BNDL > TEXB > name + 8-byte texture hash, read from
assets/ui/icons/icons.zip and virtualreadercharacters.zip); the textures are single-level DXT5 like the originals.
CrabeLoader serves files of a mod folder at the same path under assets/, and any .tbody by its file name."""
import hashlib
import struct
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageFilter

HUD_SIZE = 128
FIGURE_SIZE = 512
SMALL_SIZE = 256
GLOW = (120, 205, 255)


def hud_name(actor: str) -> str:
    """Icon field of the character row (and, lowercased, the name of the HUD bundle)."""
    return f"HUD_PlayerIcons_{actor}"


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


def body_sphere(vertices: np.ndarray) -> tuple[np.ndarray, float]:
    """Centre and radius framing the whole figure."""
    lo, hi = vertices.min(axis=0), vertices.max(axis=0)
    return (lo + hi) / 2, float(max(hi[1] - lo[1], hi[0] - lo[0]) / 2) * 1.04


def texture_hash(name: str) -> str:
    """16 hex digits, stable per bundle name, kept away from the game's own hashes by a prefix."""
    return hashlib.sha256(f"DisneyInfinityCreator/ui/{name.lower()}".encode()).hexdigest()[:16]


def ui_bundle(name: str, tex_hash: str) -> bytes:
    """A UI material bundle naming one texture, laid out like the game's (e.g. hud_playericons_anakin.mtb)."""
    raw = name.lower().encode("ascii") + b"\0"
    body = struct.pack("<I", len(raw)) + raw + b"\0" * (-len(raw) % 4) + bytes.fromhex(tex_hash)
    total = -(-(32 + len(body) + 4) // 16) * 16
    head = b"BNDL" + struct.pack("<III", 1, total, 1) + b"TEXB" + struct.pack("<III", 3, total - 16, 1)
    out = head + body
    return out + b"\0" * (total - len(out))


def dds_single(img: Image.Image) -> bytes:
    """One-level DXT5 DDS with the header fields of the game's UI textures."""
    import io
    buf = io.BytesIO()
    img.convert("RGBA").save(buf, "DDS", pixel_format="DXT5")
    payload = buf.getvalue()[128:]
    w, h = img.size
    header = struct.pack("<4s7I44s2I4s5I5I", b"DDS ", 124, 0x81007, h, w, len(payload), 0, 1, b"\0" * 44,
                         32, 0x4, b"DXT5", 0, 0, 0, 0, 0, 0x1000, 0, 0, 73, 0)
    return header + payload


def _fit(raw: Image.Image, size: int, margin: float) -> Image.Image:
    """Crop to the visible pixels and centre them in a transparent square, `margin` of the side left around."""
    raw = raw.convert("RGBA")
    box = raw.getchannel("A").getbbox()
    if box:
        raw = raw.crop(box)
    inner = max(1, int(size * (1 - 2 * margin)))
    scale = inner / max(raw.size)
    raw = raw.resize((max(1, round(raw.width * scale)), max(1, round(raw.height * scale))), Image.LANCZOS)
    out = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    out.alpha_composite(raw, ((size - raw.width) // 2, size - raw.height - int(size * margin)))
    return out


def _glow(alpha: Image.Image, radius: float, colour=GLOW, strength: float = 1.0) -> Image.Image:
    halo = alpha.filter(ImageFilter.MaxFilter(3)).filter(ImageFilter.GaussianBlur(radius))
    halo = halo.point(lambda v: min(255, int(v * 2.2 * strength)))
    layer = Image.new("RGBA", alpha.size, colour + (0,))
    layer.putalpha(halo)
    return layer


def portrait_sphere(vertices: np.ndarray, neck_y: float, height: float) -> tuple[np.ndarray, float]:
    """Framing of the HUD portrait: the head filling most of the square, the top of the shoulders at the bottom."""
    center, radius = head_sphere(vertices, neck_y, height)
    return center - np.array([0.0, 0.18 * radius, 0.0]), radius * 1.08


def hud_icon(head: Image.Image) -> Image.Image:
    """128 px head portrait on transparency with the soft blue rim of the game's HUD icons."""
    face = head.convert("RGBA").resize((HUD_SIZE, HUD_SIZE), Image.LANCZOS)
    out = _glow(face.getchannel("A"), 2.5)
    out.alpha_composite(face)
    return out


def figure_icon(body: Image.Image, size: int = FIGURE_SIZE) -> Image.Image:
    return _fit(body, size, 0.04)


def silhouette_icon(figure: Image.Image) -> Image.Image:
    """Glowing outline with a faint fill, like the Virtual Reader's locked figures."""
    alpha = figure.getchannel("A").point(lambda v: 255 if v > 40 else 0)
    edge = ImageChops.subtract(alpha.filter(ImageFilter.MaxFilter(5)), alpha.filter(ImageFilter.MinFilter(5)))
    h = figure.height
    ramp = np.linspace(0.15, 0.55, h)[:, None] * np.ones((1, figure.width))
    fill_a = (np.asarray(alpha, np.float32) / 255 * ramp * 255).astype(np.uint8)
    out = Image.new("RGBA", figure.size, (40, 110, 150, 0))
    out.putalpha(Image.fromarray(fill_a))
    out.alpha_composite(_glow(edge, 6, strength=0.8))
    line = Image.new("RGBA", figure.size, (235, 250, 255, 0))
    line.putalpha(edge)
    out.alpha_composite(line)
    return out


def build_pictures(head: Image.Image, body: Image.Image | None) -> dict[str, Image.Image]:
    """Every picture the game shows for a character, from a head render and a full-body render (transparent)."""
    pictures = {"hud": hud_icon(head)}
    if body is not None:
        figure = figure_icon(body)
        pictures.update(figure=figure, small=figure.resize((SMALL_SIZE, SMALL_SIZE), Image.LANCZOS),
                        silhouette=silhouette_icon(figure))
    return pictures


def write_icon(pictures: dict[str, Image.Image], actor: str, mod_dir: Path) -> list[Path]:
    """The UI bundles and textures under ui/, and <actor>_icon.png (the HUD picture) at the mod root."""
    a = actor.lower()
    files = {"hud": ("icons", hud_name(actor))}
    files.update(figure=("virtualreadercharacters", a), small=("virtualreadercharacters", f"{a}_sm"),
                 silhouette=("virtualreadercharacters", f"{a}_s"))
    written = []
    for key, picture in pictures.items():
        folder, name = files[key]
        tex = texture_hash(name)
        target = mod_dir / "ui" / folder
        (target / "textures").mkdir(parents=True, exist_ok=True)
        (target / f"{name.lower()}.mtb").write_bytes(ui_bundle(name, tex))
        (target / "textures" / f"{tex}.tbody").write_bytes(dds_single(picture))
        written += [target / f"{name.lower()}.mtb", target / "textures" / f"{tex}.tbody"]
    png = mod_dir / f"{actor}_icon.png"
    pictures["hud"].save(png)
    return [png] + written
