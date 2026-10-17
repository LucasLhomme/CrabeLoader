"""PC material bundles (.mtb): read, write, and the one Creator ships with every character.

Layout (Octane RenderAssetBuilder, PC D3D11 flavour), all little endian, sub-bundles padded to 16 bytes:
  BNDL  version 1, total size, sub-bundle count
  TEXB  version 3, size, texture count | name table size (0) | per texture: 8-byte id + name offset (-1)
  MATP  version 2, size, material count | shader count + NUL-terminated shader ids (4-aligned) |
        uuid count + NUL-terminated uuids | shader index per material (u16, 2-aligned) |
        material hash per material (u32, 4-aligned) | material data (16-aligned): per material an instance block
        then a material block, each starting with its sampler texture indices (8 bytes each, into TEXB).
Materials are matched to the .oct MaterialPool by index.

The block sizes depend on each shader, so the data is kept as one blob; src/assets/shader_layouts.json (learnt
from the game's own bundles) gives, per shader, the record size and where its texture indices sit. The character
bundle Creator writes reuses the body material of Luke Skywalker (standard skinned shader, diffuse + normal
map) for every material slot: only the body primitive draws anything once the others are emptied.
"""
import json
import struct
from dataclasses import dataclass
from functools import lru_cache

from src.core.paths import ASSETS_DIR

TEMPLATE_PATH = ASSETS_DIR / "templates" / "skinned_body.mtb"
LAYOUTS_PATH = ASSETS_DIR / "shader_layouts.json"


@dataclass
class MaterialBundle:
    textures: list[bytes]  # 8-byte ids
    shaders: list[str]
    uuids: list[str]
    shader_ids: list[int]
    hashes: list[bytes]  # 4 bytes each
    data: bytes

    @classmethod
    def from_bytes(cls, d: bytes) -> "MaterialBundle":
        if d[:4] != b"BNDL":
            raise ValueError("not a material bundle")
        p, textures, matp = 16, [], None
        for _ in range(struct.unpack_from("<I", d, 12)[0]):
            tag, _version, size, count = struct.unpack_from("<4s3I", d, p)
            body = d[p + 16:p + size]
            if tag == b"TEXB":
                names = struct.unpack_from("<I", body, 0)[0]
                start = 4 + names
                textures = [body[start + 12 * i:start + 12 * i + 8] for i in range(count)]
            elif tag == b"MATP":
                matp = (body, count)
            p += size
        if matp is None:
            raise ValueError("no MATP sub-bundle")
        body, count = matp
        q, strings = 0, []

        def read_strings(q):
            n = struct.unpack_from("<I", body, q)[0]
            q += 4
            out = []
            for _ in range(n):
                end = body.index(b"\0", q)
                out.append(body[q:end].decode("ascii"))
                q = end + 1
            return out, q

        shaders, q = read_strings(q)
        q = _align(q, 4)
        uuids, q = read_strings(q)
        q = _align(q, 2)
        shader_ids = list(struct.unpack_from(f"<{count}H", body, q))
        q = _align(q + 2 * count, 4)
        hashes = [body[q + 4 * i:q + 4 * i + 4] for i in range(count)]
        q = _align(q + 4 * count, 16)
        return cls(textures, shaders, uuids, shader_ids, hashes, body[q:])

    def material_textures(self) -> list[list[str]] | None:
        """Texture ids each material samples, in slot order (diffuse first for the skinned shaders), or None when a
        shader's layout is unknown (see tools/build_shader_table.py)."""
        layouts = _shader_layouts()
        out, p = [], 0
        for sid in self.shader_ids:
            layout = layouts.get(self.shaders[sid])
            if not layout or "texture_offset" not in layout:
                return None
            record = self.data[p:p + layout["record_size"]]
            ids = struct.unpack_from(f"<{layout['texture_slots']}q", record, layout["texture_offset"])
            out.append([self.textures[i].hex() for i in ids if 0 <= i < len(self.textures)])
            p += layout["record_size"]
        return out if p == len(self.data) else None

    def to_bytes(self) -> bytes:
        texb = bytearray(struct.pack("<I", 0))
        for t in self.textures:
            texb += t + b"\xff\xff\xff\xff"
        matp = bytearray(struct.pack("<I", len(self.shaders)))
        for s in self.shaders:
            matp += s.encode("ascii") + b"\0"
        matp += bytes(_align(len(matp), 4) - len(matp)) + struct.pack("<I", len(self.uuids))
        for u in self.uuids:
            matp += u.encode("ascii") + b"\0"
        matp += bytes(_align(len(matp), 2) - len(matp)) + struct.pack(f"<{len(self.shader_ids)}H", *self.shader_ids)
        matp += bytes(_align(len(matp), 4) - len(matp)) + b"".join(self.hashes)
        matp += bytes(_align(len(matp), 16) - len(matp)) + self.data
        subs = [_sub(b"TEXB", 3, len(self.textures), texb)] if self.textures else []
        subs.append(_sub(b"MATP", 2, len(self.hashes), matp))
        body = b"".join(subs)
        return struct.pack("<4s3I", b"BNDL", 1, 16 + len(body), len(subs)) + body


@lru_cache(maxsize=1)
def _shader_layouts() -> dict[str, dict]:
    return json.loads(LAYOUTS_PATH.read_text(encoding="utf-8")) if LAYOUTS_PATH.exists() else {}


def _align(n: int, a: int) -> int:
    return (n + a - 1) // a * a


def _sub(tag: bytes, version: int, count: int, body: bytes) -> bytes:
    size = _align(16 + len(body), 16)
    return struct.pack("<4s3I", tag, version, size, count) + body + bytes(size - 16 - len(body))


def character_bundle(material_count: int, diffuse: str, normal: str, material_hash: bytes,
                     uuids: list[str]) -> bytes:
    """A bundle whose every material is the template's body material, sampling [diffuse, normal]."""
    template = MaterialBundle.from_bytes(TEMPLATE_PATH.read_bytes())
    if len(template.hashes) != 1:
        raise ValueError("the material template must hold exactly one material")
    return MaterialBundle(
        textures=[bytes.fromhex(diffuse), bytes.fromhex(normal)],
        shaders=template.shaders[:1], uuids=list(uuids), shader_ids=[0] * material_count,
        hashes=[material_hash] * material_count, data=template.data * material_count).to_bytes()
