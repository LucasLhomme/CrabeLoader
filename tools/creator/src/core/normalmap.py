"""Relief normal map made from the model's own texture, in the game's normal-map layout.

The game's figures owe much of their "sculpted toy" look to their normal maps: seams, folds, buttons and hair
strands are carved there, while the colour textures stay simple (measured on 37 figures: 1024 px DXT5 normal map
beside each 1024 px DXT1 colour map). A ripped model has no such map, so it looked flat and plastic in game.

Without a sculpt, the texture is the best guide: dark lines and colour edges are where seams, folds and borders
are painted, so a soft height field is taken from the texture's fine detail (dark = recessed) and turned into
normals. Layout of the game's maps, read from the shipped files: R = 255, G = Y (up), A = X, B unused (~87)."""
import numpy as np
from PIL import Image

STRENGTH = 10.0
"""Slope per unit of height at 1024 px (with CONTRAST: median tilt ~4 degrees, 95th percentile ~31, game ~4 and ~35)."""
CONTRAST = 2.2
"""Exponent on the height: measured on the game's 40 large normal maps, most texels tilt ~4 degrees and the
carved seams ~35 (95th percentile); a linear height gives either flat seams or a noisy surface."""
DETAIL_SIGMA = 1 / 96
"""Fine detail = the texture minus a blur this wide (fraction of the texture side): wide shading is not relief."""


def _blur(arr: np.ndarray, radius: float) -> np.ndarray:
    from scipy.ndimage import gaussian_filter
    return gaussian_filter(arr.astype(np.float32), max(radius, 0.1), mode="nearest")


def height_field(diffuse: Image.Image, mask: np.ndarray | None = None) -> np.ndarray:
    """Height in about -1..1: recessed where the texture is darker than its surroundings or changes colour."""
    rgb = np.asarray(diffuse.convert("RGB"), np.float32) / 255
    side = max(rgb.shape[:2])
    luma = rgb @ np.array([0.299, 0.587, 0.114], np.float32)
    detail = luma - _blur(luma, DETAIL_SIGMA * side)
    # Colour borders (a red sleeve on a blue shirt with equal brightness) become a shallow groove too.
    chroma = rgb - luma[..., None]
    edge = sum(np.abs(_blur(chroma[..., c], 1.0) - _blur(chroma[..., c], 3.0)) for c in range(3))
    height = detail - 0.5 * edge
    height = _blur(height, side / 1024)  # soft, sculpted edges rather than pixel noise
    scale = np.percentile(np.abs(height), 99) if height.size else 1.0
    height = np.clip(height / max(scale, 1e-4), -1, 1)
    height = np.sign(height) * np.abs(height) ** CONTRAST  # deep seams, quiet flat areas, like the game's sculpts
    if mask is not None:
        height = height * _blur(mask.astype(np.float32), 2.0)  # unused texels and island borders stay flat
    return height


def normal_map(diffuse: Image.Image, mask: np.ndarray | None = None, strength: float = 1.0,
               max_size: int = 1024) -> Image.Image:
    """RGBA normal map in the game's layout, at the texture's size (at most `max_size`)."""
    img = diffuse
    if max(img.size) > max_size:
        k = max_size / max(img.size)
        img = img.resize((max(4, round(img.width * k)), max(4, round(img.height * k))), Image.LANCZOS)
        if mask is not None:
            mask = np.asarray(Image.fromarray(mask.astype(np.uint8) * 255).resize(img.size)) > 127
    h = height_field(img, mask)
    s = STRENGTH * strength * max(img.size) / 1024
    dx = np.gradient(h, axis=1) * s
    dy = np.gradient(h, axis=0) * s  # image rows go down, the map's Y goes up
    n = np.dstack([-dx, dy, np.ones_like(h)])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    out = np.empty(h.shape + (4,), np.uint8)
    out[..., 0] = 255
    out[..., 1] = np.clip(np.round((n[..., 1] * 0.5 + 0.5) * 255), 0, 255)
    out[..., 2] = 87
    out[..., 3] = np.clip(np.round((n[..., 0] * 0.5 + 0.5) * 255), 0, 255)
    return Image.fromarray(out, "RGBA")


def decode(normal: Image.Image) -> np.ndarray:
    """Tangent-space normals (H, W, 3) from a map in the game's layout (for previews and tests)."""
    a = np.asarray(normal.convert("RGBA"), np.float32) / 255 * 2 - 1
    x, y = a[..., 3], a[..., 1]
    z = np.sqrt(np.clip(1 - x * x - y * y, 0, 1))
    return np.dstack([x, y, z])
