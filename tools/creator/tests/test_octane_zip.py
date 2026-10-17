import struct
import zlib

from src.core import octane_zip as oz


def make_entry(name: str, data: bytes, h: int) -> oz.Entry:
    """An entry encrypted the way offsetting.exe stores it."""
    comp = zlib.compress(data, 9)[2:-4]
    n = name.encode()
    head = struct.pack("<4sHHHHHIIIHH", b"PK\x03\x04", 20, 0, 8, 0, 0, zlib.crc32(data), len(comp), len(data), len(n), 0)
    clear = head + n + comp
    cut = len(head) + len(n) + min(oz.ENCRYPTED_HEAD, len(comp))
    return oz.Entry(name, h, oz._ctr(clear[:cut]) + clear[cut:], struct.unpack("<4sHHHHHIIIHH", head))


def test_write_read_merge(tmp_path):
    big = bytes(range(256)) * 40  # longer than the encrypted head
    base = [make_entry("textures/a.tbody", b"aaa", 30), make_entry("textures/b.tbody", big, 10)]
    oz.write(base, tmp_path / "base.zip")
    back = oz.read(tmp_path / "base.zip")
    assert [(e.name, e.hash, e.data()) for e in back] == [("textures/a.tbody", 30, b"aaa"), ("textures/b.tbody", 10, big)]
    oz.write(back, tmp_path / "again.zip")
    assert (tmp_path / "again.zip").read_bytes() == (tmp_path / "base.zip").read_bytes()
    oz.write([make_entry("textures/c.tbody", b"new", 20), make_entry("textures/a.tbody", b"AAA", 30)],
             tmp_path / "extra.zip")
    assert oz.merge(tmp_path / "base.zip", tmp_path / "extra.zip", tmp_path / "out.zip") == 3
    raw = (tmp_path / "out.zip").read_bytes()
    index = oz._ctr(raw[:8 + 8 * 3])
    assert [struct.unpack_from("<I", index, 8 + 8 * i)[0] for i in range(3)] == [10, 20, 30]  # sorted for lookup
    assert {e.name: e.data() for e in oz.read(tmp_path / "out.zip")} == {
        "textures/b.tbody": big, "textures/c.tbody": b"new", "textures/a.tbody": b"AAA"}


def test_merge_with_digest_cache_gives_same_file(tmp_path):
    oz.write([make_entry("a.mtb", b"x" * 900, 5)], tmp_path / "base.zip")
    oz.write([make_entry("b.mtb", b"y", 7)], tmp_path / "extra.zip")
    oz.merge(tmp_path / "base.zip", tmp_path / "extra.zip", tmp_path / "plain.zip")
    for _ in range(2):  # first call fills the cache, second one uses it
        oz.merge(tmp_path / "base.zip", tmp_path / "extra.zip", tmp_path / "cached.zip", tmp_path / "md5.json")
        assert (tmp_path / "cached.zip").read_bytes() == (tmp_path / "plain.zip").read_bytes()
