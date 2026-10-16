"""Texture filter that pulls a downloaded model's colours towards the game's art direction.

The base characters have bright, saturated, flat colours with little baked shading. Fan models are usually darker,
greyer and noisy. The filter softens micro-detail (edge-preserving, only inside the UV islands so colours never
bleed from the empty background), lifts the shadows and boosts saturation. Strength 0 = untouched."""
import numpy as np
from PIL import Image, ImageDraw
from scipy.ndimage import uniform_filter

LUMA = np.array([0.2126, 0.7152, 0.0722], np.float32)


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


def _box3(x, r):
    """Box blur of an (H, W, C) stack, channel by channel, in one call."""
    return uniform_filter(x, size=(2 * r + 1, 2 * r + 1, 1), mode="nearest")


def _down(x, k):
    """Average over k x k blocks (the image is cropped to a multiple of k)."""
    h, w = (x.shape[0] // k) * k, (x.shape[1] // k) * k
    x = x[:h, :w]
    return x.reshape(h // k, k, w // k, k, *x.shape[2:]).mean(axis=(1, 3))


def _up(x, shape):
    """Bilinear resize back to the full (H, W), channel by channel (PIL is much faster than scipy here)."""
    size = (shape[1], shape[0])
    if x.ndim == 2:
        return np.asarray(Image.fromarray(x.astype(np.float32), "F").resize(size, Image.BILINEAR))
    return np.stack([_up(x[..., c], shape) for c in range(x.shape[2])], -1)


def _fast_guided(guide, src, weight, r, eps, k):
    """Fast guided filter (He & Sun 2015): coefficients computed at 1/k resolution, applied at full resolution.
    Same look as the full filter for large radii, about k^2 times cheaper."""
    if k <= 1:
        return _guided(guide, src, weight, r, eps)
    g, p, w = _down(guide, k), _down(src, k), _down(weight, k)
    rk = max(1, round(r / k))
    wsum = np.maximum(_box(w, rk), 1e-6)
    mi = _box(g * w, rk) / wsum
    var = _box(g * g * w, rk) / wsum - mi * mi
    w3, g3, ws3, mi3 = w[..., None], g[..., None], wsum[..., None], mi[..., None]
    mp = _box3(p * w3, rk) / ws3
    cov = _box3(g3 * p * w3, rk) / ws3 - mi3 * mp
    a = cov / (var[..., None] + eps)
    b = mp - a * mi3
    a, b = _box3(a * w3, rk) / ws3, _box3(b * w3, rk) / ws3
    shape = guide.shape
    return _up(a, shape) * guide[..., None] + _up(b, shape)


def _guided(guide, src, weight, r, eps):
    """Guided filter of the (H, W, 3) image `src`, ignoring texels outside the mask (normalised convolution).
    The guide's statistics are shared by the three channels."""
    wsum = np.maximum(_box(weight, r), 1e-6)
    mi = _box(guide * weight, r) / wsum
    var = _box(guide * guide * weight, r) / wsum - mi * mi
    w3, g3, ws3, mi3 = weight[..., None], guide[..., None], wsum[..., None], mi[..., None]
    mp = _box3(src * w3, r) / ws3
    cov = _box3(g3 * src * w3, r) / ws3 - mi3 * mp
    a = cov / (var[..., None] + eps)
    b = mp - a * mi3
    return _box3(a * w3, r) / ws3 * g3 + _box3(b * w3, r) / ws3


def mean_luma(image: Image.Image, mask: np.ndarray | None = None) -> float:
    """Average brightness (0-1) of the used texels, ignoring the near-black padding of an atlas."""
    a = np.asarray(image.convert("RGB"), np.float32) / 255
    y = (a * LUMA).sum(-1)
    keep = y > 0.05
    if mask is not None:
        keep &= mask
    return float(y[keep].mean()) if keep.any() else float(y.mean())


def base_luma(images) -> float | None:
    """Average brightness of a base figure's colour textures."""
    values = [mean_luma(im) for im in images if im is not None]
    return float(np.mean(values)) if values else None


def _match(out: np.ndarray, mask, target: float, amount: float) -> np.ndarray:
    """Gamma curve that moves the average brightness a fraction `amount` of the way to `target`."""
    y = (out * LUMA).sum(-1)
    keep = (y > 0.05) if mask is None else ((y > 0.05) & mask)
    if not keep.any():
        return out
    sample = y[keep]
    if len(sample) > 200_000:  # the curve only needs the brightness distribution: a sample is enough
        sample = sample[np.random.default_rng(0).choice(len(sample), 200_000, replace=False)]
    cur = float(sample.mean())
    goal = cur + amount * (target - cur)
    lo, hi = 0.3, 3.0
    for _ in range(24):
        g = (lo + hi) / 2
        if float((sample ** g).mean()) > goal:
            lo = g
        else:
            hi = g
    return np.clip(out, 0, 1) ** ((lo + hi) / 2)


def apply(image: Image.Image, strength: float, mask: np.ndarray | None = None,
          match: float = 0.0, target: float | None = None) -> Image.Image:
    """strength in 0..1 (softening, lift, saturation). match in 0..1 moves the average brightness towards `target`
    (the base figure's). Returns a new RGB image of the same size."""
    image = image.convert("RGB")
    if strength <= 0 and not (match > 0 and target is not None):
        return image
    s = float(min(strength, 1.0))
    if s <= 0:
        a0 = np.asarray(image, np.float32) / 255
        out = _match(a0, mask, target, float(min(match, 1.0)))
        if mask is not None:
            out = np.where(mask[..., None], out, a0)
        return Image.fromarray((out * 255 + 0.5).astype(np.uint8))
    a = np.asarray(image, np.float32) / 255
    w = np.ones(a.shape[:2], np.float32) if mask is None else mask.astype(np.float32)
    r = max(2, round(a.shape[1] / 256))
    guide = a.mean(-1)
    eps = 0.004 + 0.02 * s
    sm = a
    k = max(1, r // 2)  # coefficient resolution: 1/k of the texture (k = 1 below 1024 px)
    for _ in range(2):
        sm = _fast_guided(guide, sm, w, r, eps, k)
    out = a * (1 - 0.8 * s) + sm * (0.8 * s)
    lift = 0.6 * s
    out = np.clip(0.05 * lift + out * (1 - 0.05 * lift), 0, 1) ** (1 - 0.3 * lift)
    y = (out * LUMA).sum(-1, keepdims=True)
    out = y + (out - y) * (1 + 0.8 * 0.7 * s)
    out = np.clip(out, 0, 1)
    if match > 0 and target is not None:
        out = _match(out, mask, target, float(min(match, 1.0)))
    if mask is not None:  # keep the background exactly as it was
        out = np.where(mask[..., None], out, a)
    return Image.fromarray((out * 255 + 0.5).astype(np.uint8))
