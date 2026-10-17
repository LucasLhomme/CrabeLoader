"""Lossless reader/writer for Octane scene files (.oct, .bent, .banm): header, string table, tagged node tree.

Layout: magic (12 B), string table size, tree size, node count (3 x u32 LE), 36 zero bytes, the NUL-separated
string table, then the tree. Each node is a u16 flag (level << 10 | value type) and a u16 string index for its tag,
followed by its value inline. A node is the child of the closest preceding node one level up.

Parsing then serialising an untouched scene gives back the exact input bytes (checked on every shipped avatar), so
edits can change a value's width, rename strings or add strings: the sizes and the count are recomputed on write.
"""
import struct
from dataclasses import dataclass, field
from pathlib import Path

MAGIC = bytes.fromhex("29760145cdcc8c3f00000000")
HEADER_SIZE = 60

# type -> (count size, element size, signed) for lists, (0, size, signed) for scalars.
_INT_LISTS = {0x1A: (1, 1, True), 0x23: (1, 1, False), 0x11A: (1, 2, False), 0x21A: (1, 3, True),
              0x5A: (2, 1, False), 0x63: (2, 1, False), 0x15A: (2, 2, False), 0xA3: (3, 1, False)}
_INT_SCALARS = {0x1B: (1, True), 0x11B: (2, False), 0x21B: (3, False), 0x31B: (4, False)}
_FLOAT_LISTS = {0x12: 1, 0x52: 2}
_STR_LISTS = {0x0A: 1, 0x4A: 2}
# Widening order used when a new value no longer fits the original type.
_SCALAR_LADDER = (0x1B, 0x11B, 0x21B, 0x31B)
_LIST_LADDER = (0x1A, 0x11A, 0x21A)


class SceneError(ValueError):
    pass


@dataclass(eq=False)
class Node:
    level: int
    type: int
    tag: str
    value: object = None
    extra: object = None  # second payload of the compound types (0x0F, 0x1F, 0x16)
    children: list["Node"] = field(default_factory=list)

    def find(self, tag: str) -> "Node | None":
        return next((c for c in self.children if c.tag == tag), None)

    def findall(self, tag: str) -> list["Node"]:
        return [c for c in self.children if c.tag == tag]

    def get(self, tag: str, default=None):
        node = self.find(tag)
        return default if node is None else node.value

    def iter(self, tag: str | None = None):
        if tag is None or self.tag == tag:
            yield self
        for c in self.children:
            yield from c.iter(tag)

    def set_ints(self, values) -> None:
        """Replace an int scalar or int list, widening the stored type if a value no longer fits."""
        values = [int(v) for v in values] if isinstance(values, (list, tuple)) else int(values)
        if self.type in _INT_SCALARS:
            self.type = _fit(self.type, [values], _SCALAR_LADDER, lambda t: (_INT_SCALARS[t][0], _INT_SCALARS[t][1]))
        elif self.type in _INT_LISTS:
            self.type = _fit(self.type, values, _LIST_LADDER, lambda t: _INT_LISTS[t][1:])
        else:
            raise SceneError(f"{self.tag}: not an int node (type {self.type:#x})")
        self.value = values

    def set_floats(self, values) -> None:
        if self.type == 0x13:
            self.value = float(values)
        elif self.type in _FLOAT_LISTS:
            self.value = [float(v) for v in values]
        else:
            raise SceneError(f"{self.tag}: not a float node (type {self.type:#x})")


def _fits(v: int, size: int, signed: bool) -> bool:
    lo, hi = (-(1 << (8 * size - 1)), (1 << (8 * size - 1)) - 1) if signed else (0, (1 << (8 * size)) - 1)
    return lo <= v <= hi


def _fit(current: int, values, ladder, spec) -> int:
    size, signed = spec(current)
    if all(_fits(v, size, signed) for v in values):
        return current
    start = ladder.index(current) + 1 if current in ladder else 0
    for t in ladder[start:]:
        size, signed = spec(t)
        if all(_fits(v, size, signed) for v in values):
            return t
    raise SceneError(f"values out of range for any int type: {min(values)}..{max(values)}")


class Scene:
    def __init__(self, strings: list[str], root: Node):
        self.strings = strings
        self.root = root

    @property
    def top(self) -> Node:
        """The level-0 node holding the pools (SceneTreeNodePool, MaterialPool, VertexBufferPool...)."""
        return self.root.children[0]

    # ------------------------------------------------------------------ read
    @classmethod
    def load(cls, path: str | Path) -> "Scene":
        return cls.from_bytes(Path(path).read_bytes())

    @classmethod
    def from_bytes(cls, data: bytes) -> "Scene":
        if data[:12] != MAGIC:
            raise SceneError("not an Octane scene (bad magic)")
        st_size, tree_size, count = struct.unpack_from("<3I", data, 12)
        if HEADER_SIZE + st_size + tree_size != len(data):
            raise SceneError("scene sizes do not match the file length")
        strings = [s.decode("utf-8", "surrogateescape") for s in data[HEADER_SIZE:HEADER_SIZE + st_size].split(b"\0")[:-1]]
        root = Node(-1, 0, "")
        stack = [root]
        p, end, n = HEADER_SIZE + st_size, len(data), 0
        while p < end:
            flag, si = struct.unpack_from("<HH", data, p)
            p += 4
            level, t = divmod(flag, 0x400)
            node = Node(level, t, strings[si])
            p = _read_value(node, data, p, strings)
            while stack[-1].level >= level:
                stack.pop()
            stack[-1].children.append(node)
            stack.append(node)
            n += 1
        if n != count:
            raise SceneError(f"node count mismatch ({n} read, header says {count})")
        return cls(strings, root)

    # ----------------------------------------------------------------- write
    def to_bytes(self) -> bytes:
        index = {s: i for i, s in enumerate(self.strings)}
        strings = list(self.strings)

        def sid(s: str) -> int:
            if s not in index:
                index[s] = len(strings)
                strings.append(s)
            return index[s]

        out = bytearray()
        count = 0
        for node in self.root.iter():
            if node is self.root:
                continue
            out += struct.pack("<HH", node.level << 10 | node.type, sid(node.tag))
            out += _write_value(node, sid)
            count += 1
        table = b"".join(s.encode("utf-8", "surrogateescape") + b"\0" for s in strings)
        if len(strings) > 0xFFFF:
            raise SceneError("string table too large")
        return MAGIC + struct.pack("<3I", len(table), len(out), count) + bytes(36) + table + bytes(out)

    def save(self, path: str | Path) -> None:
        Path(path).write_bytes(self.to_bytes())

    def replace_strings(self, old: str, new: str) -> int:
        """Rewrite `old` inside every string value (not tags). Returns the number of values changed."""
        changed = 0
        for node in self.root.iter():
            if node.type in (0x05, 0x0B, 0x0F, 0x1F, 0x16) and isinstance(node.value, str) and old in node.value:
                node.value = node.value.replace(old, new)
                changed += 1
            elif node.type in _STR_LISTS and any(old in s for s in node.value):
                node.value = [s.replace(old, new) for s in node.value]
                changed += 1
        return changed


def _read_value(node: Node, d: bytes, p: int, strings: list[str]) -> int:
    t = node.type
    u = lambda size: int.from_bytes(d[p:p + size], "little")
    if t == 0x01:
        return p
    if t in (0x05, 0x0B):
        node.value = strings[u(2)]
        return p + 2
    if t == 0x0F:
        node.value, node.extra = strings[u(2)], strings[int.from_bytes(d[p + 2:p + 4], "little")]
        return p + 4
    if t == 0x1F:
        node.value, node.extra = strings[u(2)], d[p + 2]
        return p + 3
    if t == 0x16:
        node.value = strings[u(2)]
        n = d[p + 2]
        node.extra = list(struct.unpack_from(f"<{n}f", d, p + 3))
        return p + 3 + 4 * n
    if t == 0x13:
        node.value = struct.unpack_from("<f", d, p)[0]
        return p + 4
    if t in _INT_SCALARS:
        size, signed = _INT_SCALARS[t]
        node.value = int.from_bytes(d[p:p + size], "little", signed=signed)
        return p + size
    if t in _INT_LISTS:
        cs, es, signed = _INT_LISTS[t]
        n = u(cs)
        p += cs
        node.value = [int.from_bytes(d[p + i * es:p + (i + 1) * es], "little", signed=signed) for i in range(n)]
        return p + n * es
    if t in _FLOAT_LISTS:
        cs = _FLOAT_LISTS[t]
        n = u(cs)
        node.value = list(struct.unpack_from(f"<{n}f", d, p + cs))
        return p + cs + 4 * n
    if t in _STR_LISTS:
        cs = _STR_LISTS[t]
        n = u(cs)
        p += cs
        node.value = [strings[int.from_bytes(d[p + 2 * i:p + 2 * i + 2], "little")] for i in range(n)]
        return p + 2 * n
    raise SceneError(f"unknown value type {t:#x} at {p - 4:#x}")


def _write_value(node: Node, sid) -> bytes:
    t, v = node.type, node.value
    if t == 0x01:
        return b""
    if t in (0x05, 0x0B):
        return struct.pack("<H", sid(v))
    if t == 0x0F:
        return struct.pack("<HH", sid(v), sid(node.extra))
    if t == 0x1F:
        return struct.pack("<HB", sid(v), node.extra)
    if t == 0x16:
        return struct.pack("<HB", sid(v), len(node.extra)) + struct.pack(f"<{len(node.extra)}f", *node.extra)
    if t == 0x13:
        return struct.pack("<f", v)
    if t in _INT_SCALARS:
        size, signed = _INT_SCALARS[t]
        return int(v).to_bytes(size, "little", signed=signed)
    if t in _INT_LISTS:
        cs, es, signed = _INT_LISTS[t]
        return len(v).to_bytes(cs, "little") + b"".join(int(x).to_bytes(es, "little", signed=signed) for x in v)
    if t in _FLOAT_LISTS:
        return len(v).to_bytes(_FLOAT_LISTS[t], "little") + struct.pack(f"<{len(v)}f", *v)
    if t in _STR_LISTS:
        return len(v).to_bytes(_STR_LISTS[t], "little") + b"".join(struct.pack("<H", sid(s)) for s in v)
    raise SceneError(f"unknown value type {t:#x}")
