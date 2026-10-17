"""Read and write the game's encrypted archives (Disney Infinity 3.0 "new octane" zips) without unpacking them.

Layout, worked out from quickbms' script and the files offsetting.exe writes:
  - index: b"PK\\xff\\xff", entry count, then (name hash, entry offset) pairs sorted by hash (binary search);
  - entries, end to end: a zip local header + name + extra + data (deflate); AES-128-CTR (zero IV) restarts at
    each entry and covers the header, the name, the extra and the first 0x200 bytes of data, the rest is plain;
  - a zip central directory (one CTR stream), each record carrying an extra field "KF" = b"MD5" + md5(data);
  - the end-of-central-directory record (its own CTR stream).
Each entry is encrypted independently of where it sits, so an archive can be extended by copying the game's
entries byte for byte and adding new ones: rebuilding icons.zip (22 700 files, 125 MB) takes a few seconds
instead of extracting, copying and re-encrypting everything. The name hash is computed by offsetting.exe (the
new entries come from a small archive it builds), as the game expects it.
"""
import hashlib
import struct
import zlib
from dataclasses import dataclass
from pathlib import Path

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

KEY = bytes.fromhex("681BBEEA63160188F9B7945104A51499")
ENCRYPTED_HEAD = 0x200
_LOCAL = struct.Struct("<4sHHHHHIIIHH")
_CENTRAL = struct.Struct("<4sHHHHHHIIIHHHHHII")
_EOCD = struct.Struct("<4sHHHHIIH")


def _ctr(data: bytes) -> bytes:
    """AES-128-CTR from a zero counter (same operation both ways)."""
    c = Cipher(algorithms.AES(KEY), modes.CTR(b"\0" * 16)).encryptor()
    return c.update(data) + c.finalize()


@dataclass
class Entry:
    name: str
    hash: int
    raw: bytes  # the encrypted entry exactly as stored (header + name + extra + data)
    header: tuple  # decrypted local header fields

    @property
    def method(self) -> int:
        return self.header[3]

    def data(self) -> bytes:
        """The file's content (decrypted, inflated)."""
        nl, el, cs = self.header[9], self.header[10], self.header[7]
        start = 0x1e + nl + el
        head = _ctr(self.raw[:start + min(ENCRYPTED_HEAD, cs)])
        comp = head[start:] + self.raw[start + min(ENCRYPTED_HEAD, cs):start + cs]
        return zlib.decompress(comp, -15) if self.method == 8 else comp


def read(path: Path) -> list[Entry]:
    """Entries in file order."""
    raw = Path(path).read_bytes()
    count = struct.unpack_from("<I", _ctr(raw[:8]), 4)[0]
    index = _ctr(raw[:8 + 8 * count])
    if index[:4] != b"PK\xff\xff":
        raise ValueError(f"{path}: not an encrypted game archive")
    pairs = sorted((struct.unpack_from("<II", index, 8 + 8 * i) for i in range(count)), key=lambda p: p[1])
    entries = []
    for h, off in pairs:
        head = _LOCAL.unpack_from(_ctr(raw[off:off + 0x1e]))
        if head[0] != b"PK\x03\x04":
            raise ValueError(f"{path}: bad entry at {off}")
        nl, el, cs = head[9], head[10], head[7]
        name = _ctr(raw[off:off + 0x1e + nl])[0x1e:].decode("utf-8")
        entries.append(Entry(name, h, raw[off:off + 0x1e + nl + el + cs], head))
    return entries


def write(entries: list[Entry], out: Path, md5: dict[str, bytes] | None = None) -> None:
    """An archive holding `entries` in this order, laid out like offsetting.exe's. `md5`: known digests by name
    (the game's entries never change: computing them means inflating the whole archive)."""
    md5 = md5 or {}
    index_size = 8 + 8 * len(entries)
    offsets, pos = [], index_size
    for e in entries:
        offsets.append(pos)
        pos += len(e.raw)
    index = b"PK\xff\xff" + struct.pack("<I", len(entries)) + b"".join(
        struct.pack("<II", h, o) for h, o in sorted(((e.hash, o) for e, o in zip(entries, offsets))))
    central = bytearray()
    for e, off in zip(entries, offsets):
        _, ver, flag, method, mtime, mdate, crc, cs, us, nl, _el = e.header
        name = e.name.encode("utf-8")
        extra = b"KF" + struct.pack("<H", 19) + b"MD5" + (md5.get(e.name) or hashlib.md5(e.data()).digest())
        central += _CENTRAL.pack(b"PK\x01\x02", 20, ver, flag, method, mtime, mdate, crc, cs, us, len(name),
                                 len(extra), 0, 0, 0, 0, off) + name + extra
    eocd = _EOCD.pack(b"PK\x05\x06", 0, 0, len(entries), len(entries), len(central), pos, 0)
    out = Path(out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "wb") as f:
        f.write(_ctr(index))
        for e in entries:
            f.write(e.raw)
        f.write(_ctr(bytes(central)))
        f.write(_ctr(eocd))


def base_digests(base: Path, cache: Path | None) -> dict[str, bytes]:
    """md5 of every entry of a game archive, kept in `cache` (json) while the archive is unchanged."""
    import json
    st = Path(base).stat()
    key = f"{st.st_size}_{st.st_mtime_ns}"
    if cache is not None and cache.is_file():
        try:
            data = json.loads(cache.read_text(encoding="utf-8"))
            if data.get("source") == key:
                return {k: bytes.fromhex(v) for k, v in data["md5"].items()}
        except (OSError, ValueError, KeyError):
            pass
    digests = {e.name: hashlib.md5(e.data()).digest() for e in read(base)}
    if cache is not None:
        cache.parent.mkdir(parents=True, exist_ok=True)
        cache.write_text(json.dumps({"source": key, "md5": {k: v.hex() for k, v in digests.items()}}), encoding="utf-8")
    return digests


def merge(base: Path, extra: Path, out: Path, cache: Path | None = None) -> int:
    """out = base archive + the entries of `extra` (same names replaced). Returns the number of entries.
    `cache`: json file keeping the base archive's digests between rebuilds."""
    added = read(extra)
    names = {e.name.lower() for e in added}
    entries = [e for e in read(base) if e.name.lower() not in names] + added
    write(entries, out, base_digests(base, cache) if cache is not None else None)
    return len(entries)
