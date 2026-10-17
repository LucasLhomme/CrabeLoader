import struct

import numpy as np
from PIL import Image

from src.core import icon

# assets/ui/icons/icons.zip -> icons/hud_playericons_anakin.mtb (80 bytes), as shipped with the game.
ANAKIN_HUD = bytes.fromhex(
    "424e444c01000000500000000100000054455842030000004000000001000000"
    "170000006875645f706c6179657269636f6e735f616e616b696e0000aebe6c6c"
    "80be6e37000000000000000000000000")
# virtualreadercharacters.zip -> virtualreadercharacters/tcw_anakin_sm.mtb (64 bytes).
ANAKIN_SM = bytes.fromhex(
    "424e444c01000000400000000100000054455842030000003000000001000000"
    "0e0000007463775f616e616b696e5f736d0000009d05d8044429da5500000000")


def test_head_sphere_frames_the_head():
    pts = np.array([[0, 1.0, 0], [0, 1.8, 0], [0.1, 1.6, 0.05], [1.0, 1.4, 0], [0, 0.2, 0]], float)
    c, r = icon.head_sphere(pts, 1.4, 1.8)
    assert 1.6 <= c[1] <= 1.8 and r > 0.1  # arm tip at x=1 is ignored


def test_bundles_match_the_game_files_byte_for_byte():
    assert icon.ui_bundle("HUD_PlayerIcons_Anakin", "aebe6c6c80be6e37") == ANAKIN_HUD
    assert icon.ui_bundle("tcw_anakin_sm", "9d05d8044429da55") == ANAKIN_SM


def test_write_icon_files(tmp_path):
    body = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    body.paste((200, 50, 50, 255), (200, 40, 312, 480))
    pictures = icon.build_pictures(Image.new("RGBA", (256, 256), (255, 0, 0, 255)), body)
    files = icon.write_icon(pictures, "TST_Hero", tmp_path)
    hud = tmp_path / "ui" / "icons" / "hud_playericons_tst_hero.mtb"
    assert hud in files and (tmp_path / "TST_Hero_icon.png") in files
    tex = tmp_path / "ui" / "icons" / "textures" / f"{icon.texture_hash('HUD_PlayerIcons_TST_Hero')}.tbody"
    data = tex.read_bytes()
    assert data[:4] == b"DDS " and struct.unpack("<II", data[12:20]) == (128, 128) and data[84:88] == b"DXT5"
    for name in ("tst_hero", "tst_hero_sm", "tst_hero_s"):
        assert (tmp_path / "ui" / "virtualreadercharacters" / f"{name}.mtb").stat().st_size == 64
    assert icon.hud_name("TST_Hero") == "HUD_PlayerIcons_TST_Hero"
