import numpy as np
from PIL import Image

from src.core import icon


def test_head_sphere_frames_the_head():
    pts = np.array([[0, 1.0, 0], [0, 1.8, 0], [0.1, 1.6, 0.05], [1.0, 1.4, 0], [0, 0.2, 0]], float)
    c, r = icon.head_sphere(pts, 1.4, 1.8)
    assert 1.5 < c[1] < 1.7 and r > 0.15  # arm tip at x=1 is ignored


def test_write_icon_files(tmp_path):
    img = icon.finish(Image.new("RGBA", (256, 256), (255, 0, 0, 255)), 128)
    files = icon.write_icon(img, "TST_Hero", tmp_path)
    names = {f.name for f in files}
    assert "TST_Hero.mtb" in names and "TST_Hero_icon.png" in names
    assert (tmp_path / "textures" / "TST_Hero.mtb").stat().st_size == 80
    assert any(f.suffix == ".tbody" and f.read_bytes()[:4] == b"DDS " for f in files)
