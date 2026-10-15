"""Lossless reader/writer for the game's compiled Lua 5.1 chunks (.dnax, .lua): lets a string constant change length.

Chunk layout (this game's header: little endian, int/size_t/Instruction 4 bytes, lua_Number an 8-byte double):
header (12 B), then the main function prototype, recursively:
    source (string), linedefined, lastlinedefined (int), nups, numparams, is_vararg, maxstacksize (byte),
    code (int n + n instructions), constants (int n + tagged values), prototypes (int n + nested),
    lineinfo (int n + n ints), locvars (int n + (string, int, int)), upvalues (int n + strings).
A string is a size_t length including its trailing NUL (0 = no string) followed by the bytes.
"""
import struct
from dataclasses import dataclass, field

HEADER = b"\x1bLuaQ\x00\x01\x04\x04\x04\x08\x00"
NIL, BOOL, NUMBER, STRING = 0, 1, 3, 4


class LuacError(ValueError):
    pass


@dataclass(eq=False)
class Proto:
    source: bytes | None
    line_defined: int
    last_line: int
    header: bytes  # nups, numparams, is_vararg, maxstacksize
    code: bytes
    constants: list = field(default_factory=list)  # (tag, value)
    protos: list["Proto"] = field(default_factory=list)
    lineinfo: bytes = b""
    locvars: list = field(default_factory=list)  # (name, startpc, endpc)
    upvalues: list = field(default_factory=list)

    def walk(self):
        yield self
        for p in self.protos:
            yield from p.walk()


class _Reader:
    def __init__(self, data: bytes):
        self.d, self.p = data, 0

    def take(self, n: int) -> bytes:
        if self.p + n > len(self.d):
            raise LuacError("truncated chunk")
        out = self.d[self.p:self.p + n]
        self.p += n
        return out

    def int(self) -> int:
        return struct.unpack("<i", self.take(4))[0]

    def string(self) -> bytes | None:
        n = struct.unpack("<I", self.take(4))[0]
        return None if n == 0 else self.take(n)[:-1]

    def proto(self) -> Proto:
        p = Proto(self.string(), self.int(), self.int(), self.take(4), b"")
        p.code = self.take(4 * self.int())
        for _ in range(self.int()):
            tag = self.take(1)[0]
            if tag == NIL:
                p.constants.append((NIL, None))
            elif tag == BOOL:
                p.constants.append((BOOL, self.take(1)[0]))
            elif tag == NUMBER:
                p.constants.append((NUMBER, self.take(8)))
            elif tag == STRING:
                p.constants.append((STRING, self.string()))
            else:
                raise LuacError(f"unknown constant tag {tag}")
        p.protos = [self.proto() for _ in range(self.int())]
        p.lineinfo = self.take(4 * self.int())
        p.locvars = [(self.string(), self.int(), self.int()) for _ in range(self.int())]
        p.upvalues = [self.string() for _ in range(self.int())]
        return p


def _string(s: bytes | None) -> bytes:
    return struct.pack("<I", 0) if s is None else struct.pack("<I", len(s) + 1) + s + b"\0"


def _write(p: Proto) -> bytes:
    out = [_string(p.source), struct.pack("<ii", p.line_defined, p.last_line), p.header,
           struct.pack("<i", len(p.code) // 4), p.code, struct.pack("<i", len(p.constants))]
    for tag, value in p.constants:
        out.append(bytes([tag]))
        if tag == BOOL:
            out.append(bytes([value]))
        elif tag == NUMBER:
            out.append(value)
        elif tag == STRING:
            out.append(_string(value))
    out.append(struct.pack("<i", len(p.protos)))
    out += [_write(c) for c in p.protos]
    out += [struct.pack("<i", len(p.lineinfo) // 4), p.lineinfo, struct.pack("<i", len(p.locvars))]
    out += [_string(n) + struct.pack("<ii", a, b) for n, a, b in p.locvars]
    out.append(struct.pack("<i", len(p.upvalues)))
    out += [_string(u) for u in p.upvalues]
    return b"".join(out)


def load(data: bytes) -> Proto:
    if data[:12] != HEADER:
        raise LuacError("not a Lua 5.1 chunk with the game's header")
    r = _Reader(data)
    r.take(12)
    main = r.proto()
    if r.p != len(data):
        raise LuacError("trailing bytes after the main function")
    return main


def dump(main: Proto) -> bytes:
    return HEADER + _write(main)


def string_constants(main: Proto) -> list[bytes]:
    return [v for p in main.walk() for t, v in p.constants if t == STRING]


def rewrite_strings(main: Proto, rewrite) -> int:
    """Apply rewrite(bytes) -> bytes | None to every string constant; returns how many changed."""
    changed = 0
    for p in main.walk():
        for i, (tag, value) in enumerate(p.constants):
            if tag == STRING:
                new = rewrite(value)
                if new is not None and new != value:
                    p.constants[i] = (STRING, new)
                    changed += 1
    return changed
