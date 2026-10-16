import numpy as np
from PIL import Image

from src.core import gamelook


def test_strength_zero_is_identity():
    img = Image.fromarray((np.random.rand(64, 64, 3) * 255).astype(np.uint8))
    assert np.array_equal(np.asarray(gamelook.apply(img, 0)), np.asarray(img))


def test_dark_grey_gets_brighter_and_background_untouched():
    arr = np.full((64, 64, 3), (60, 50, 45), np.uint8)
    img = Image.fromarray(arr)
    uvs = np.array([[0.1, 0.1], [0.5, 0.1], [0.1, 0.5]])
    mask = gamelook.coverage(uvs, np.array([[0, 1, 2]]), img.size)
    out = np.asarray(gamelook.apply(img, 1.0, mask))
    assert out[10, 10].sum() > arr[10, 10].sum()
    assert np.array_equal(out[60, 60], arr[60, 60])


def test_no_bleeding_from_background():
    arr = np.zeros((64, 64, 3), np.uint8)
    arr[:, :32] = (200, 40, 40)
    mask = np.zeros((64, 64), bool)
    mask[:, :32] = True
    out = np.asarray(gamelook.apply(Image.fromarray(arr), 0.8, mask)).astype(int)
    assert abs(out[30, 5, 0] - out[30, 30, 0]) < 8  # edge of the island keeps the island's colour


def test_match_moves_brightness_towards_target():
    arr = np.full((64, 64, 3), (60, 60, 60), np.uint8)
    img = Image.fromarray(arr)
    out = gamelook.apply(img, 0.0, None, 1.0, 0.5)
    assert abs(gamelook.mean_luma(out) - 0.5) < 0.03
    half = gamelook.apply(img, 0.0, None, 0.5, 0.5)
    assert gamelook.mean_luma(img) < gamelook.mean_luma(half) < gamelook.mean_luma(out)
