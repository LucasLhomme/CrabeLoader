"""Texture filter that pulls a downloaded model's colours towards the game's art direction.

The base characters have bright, saturated, flat colours with little baked shading. Fan models are usually darker,
greyer and noisy. The filter softens micro-detail (edge-preserving, only inside the UV islands so colours never
bleed from the empty background), lifts the shadows and boosts saturation. Strength 0 = untouched."""
import numpy as np
from PIL import Image, ImageDraw
from scipy.ndimage import uniform_filter

LUMA = np.array([0.2126, 0.7152, 0.0722])


def coverage(uvs: np.ndarray, faces: np.ndarray, size: tuple[int, int]) -> np.ndarray:
    """Boolean image of the texels the model actually uses (UVs are top-left origin, 0-1)."""
    w, h = size
    mask = Image.new("L", size, 0)
    draw = ImageDraw.Draw(mask)
    pts = np.mod(uvs, 1.0) * [w - 1, h - 1] if uvs.size and (uvs.min() < 0 or uvs.max() > 1) else uvs * [w - 1, h - 1]
    for tri in faces:
        draw.polygon([tuple(pts[i]) for i in tri], fill=255)
    grown = np.asarray(mask) > 0
    return uniform_filter(grown.astype(np.float32), size=3) > 0  # one texel of margin


def _box(x, r):
    return uniform_filter(x, size=2 * r + 1, mode="nearest")


def _guided(guide, src, weight, r, eps):
    """Guided filter that ignores texels outside the mask (normalised convolution)."""
    wsum = np.maximum(_box(weight, r), 1e-6)
    mean = lambda x: _box(x * weight, r) / wsum
    mi, mp = mean(guide), mean(src)
    var = mean(guide * guide) - mi * mi
    cov = mean(guide * src) - mi * mp
    a = cov / (var + eps)
    b = mp - a * mi
    return _box(a * weight, r) / wsum * guide + _box(b * weight, r) / wsum


def apply(image: Image.Image, strength: float, mask: np.ndarray | None = None) -> Image.Image:
    """strength in 0..1. Returns a new RGB image of the same size."""
    image = image.convert("RGB")
    if strength <= 0:
        return image
    s = float(min(strength, 1.0))
    a = np.asarray(image, np.float32) / 255
    w = np.ones(a.shape[:2], np.float32) if mask is None else mask.astype(np.float32)
    r = max(2, round(a.shape[1] / 256))
    guide = a.mean(-1)
    eps = 0.004 + 0.02 * s
    sm = a
    for _ in range(2):
        sm = np.stack([_guided(guide, sm[..., c], w, r, eps) for c in range(3)], -1)
    out = a * (1 - 0.8 * s) + sm * (0.8 * s)
    lift = 0.6 * s
    out = np.clip(0.05 * lift + out * (1 - 0.05 * lift), 0, 1) ** (1 - 0.3 * lift)
    y = (out * LUMA).sum(-1, keepdims=True)
    out = y + (out - y) * (1 + 0.8 * 0.7 * s)
    out = np.clip(out, 0, 1)
    if mask is not None:  # keep the background exactly as it was
        out = np.where(mask[..., None], out, a)
    return Image.fromarray((out * 255 + 0.5).astype(np.uint8))
