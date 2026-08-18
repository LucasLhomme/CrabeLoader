#!/usr/bin/env python3
"""
di3_gateway.py -- read and edit Disney Infinity 3.0 gateway files.

The gateway is the game's *figure registry*: it is what turns a sku_id into a
recognised Disney Infinity figure. Without a slot, the game refuses to play the
character with "Figurine Disney Infinity manquante", however correct the
catalog row and the actor data are. See characters/README.md.

Each slot's key is the encrypted sku_id; its fields are name / sku / version /
TagType / keysum / displayName / playset. Everything is AES-128-CBC with a
fixed key and IV (so it is deterministic), PKCS7, and each plaintext carries a
one-byte type tag: 1 = number (little-endian double follows), 2 = string,
3 = bool.

No third-party package is required: it uses `cryptography` or `pycryptodome`
when present and falls back to a small built-in AES-128 otherwise.

Commands
--------
  check    verify the crypto reproduces a file byte-for-byte (run this first)
  list     list the AVATAR slots
  dump     decrypt every slot to readable text
  freesku  suggest unused sku_ids
  add      clone an existing slot into a new sku_id, across every gateway file

Typical use, adding Mace Windu to a whole install:

  python di3_gateway.py check   --game "D:\\...\\Disney Infinity 3.0 Gold Edition"
  python di3_gateway.py freesku --game "D:\\...\\Disney Infinity 3.0 Gold Edition"
  python di3_gateway.py add     --game "D:\\...\\Disney Infinity 3.0 Gold Edition" \\
      --sku 1000320 --name TCW_MaceWindu --clone-from 1000201 --write
"""

from __future__ import annotations

import argparse
import datetime as _dt
import glob
import os
import re
import shutil
import struct
import sys

KEY = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
IV = b"0123456789012345"

TAG_NUMBER, TAG_STRING, TAG_BOOL = 1, 2, 3


# --------------------------------------------------------------------------
# AES-128-CBC. Uses a real library when available; the pure-Python fallback
# keeps this script usable on a bare interpreter, which matters for a tool
# meant to still work years from now.
# --------------------------------------------------------------------------
def _load_backend():
    try:
        from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

        def enc(block_data: bytes) -> bytes:
            c = Cipher(algorithms.AES(KEY), modes.CBC(IV)).encryptor()
            return c.update(block_data) + c.finalize()

        def dec(block_data: bytes) -> bytes:
            c = Cipher(algorithms.AES(KEY), modes.CBC(IV)).decryptor()
            return c.update(block_data) + c.finalize()

        return enc, dec, "cryptography"
    except Exception:
        pass
    try:
        from Crypto.Cipher import AES as _AES

        def enc(block_data: bytes) -> bytes:
            return _AES.new(KEY, _AES.MODE_CBC, IV).encrypt(block_data)

        def dec(block_data: bytes) -> bytes:
            return _AES.new(KEY, _AES.MODE_CBC, IV).decrypt(block_data)

        return enc, dec, "pycryptodome"
    except Exception:
        pass
    return _pure_enc, _pure_dec, "built-in"


_SBOX = [
    0x63,0x7C,0x77,0x7B,0xF2,0x6B,0x6F,0xC5,0x30,0x01,0x67,0x2B,0xFE,0xD7,0xAB,0x76,
    0xCA,0x82,0xC9,0x7D,0xFA,0x59,0x47,0xF0,0xAD,0xD4,0xA2,0xAF,0x9C,0xA4,0x72,0xC0,
    0xB7,0xFD,0x93,0x26,0x36,0x3F,0xF7,0xCC,0x34,0xA5,0xE5,0xF1,0x71,0xD8,0x31,0x15,
    0x04,0xC7,0x23,0xC3,0x18,0x96,0x05,0x9A,0x07,0x12,0x80,0xE2,0xEB,0x27,0xB2,0x75,
    0x09,0x83,0x2C,0x1A,0x1B,0x6E,0x5A,0xA0,0x52,0x3B,0xD6,0xB3,0x29,0xE3,0x2F,0x84,
    0x53,0xD1,0x00,0xED,0x20,0xFC,0xB1,0x5B,0x6A,0xCB,0xBE,0x39,0x4A,0x4C,0x58,0xCF,
    0xD0,0xEF,0xAA,0xFB,0x43,0x4D,0x33,0x85,0x45,0xF9,0x02,0x7F,0x50,0x3C,0x9F,0xA8,
    0x51,0xA3,0x40,0x8F,0x92,0x9D,0x38,0xF5,0xBC,0xB6,0xDA,0x21,0x10,0xFF,0xF3,0xD2,
    0xCD,0x0C,0x13,0xEC,0x5F,0x97,0x44,0x17,0xC4,0xA7,0x7E,0x3D,0x64,0x5D,0x19,0x73,
    0x60,0x81,0x4F,0xDC,0x22,0x2A,0x90,0x88,0x46,0xEE,0xB8,0x14,0xDE,0x5E,0x0B,0xDB,
    0xE0,0x32,0x3A,0x0A,0x49,0x06,0x24,0x5C,0xC2,0xD3,0xAC,0x62,0x91,0x95,0xE4,0x79,
    0xE7,0xC8,0x37,0x6D,0x8D,0xD5,0x4E,0xA9,0x6C,0x56,0xF4,0xEA,0x65,0x7A,0xAE,0x08,
    0xBA,0x78,0x25,0x2E,0x1C,0xA6,0xB4,0xC6,0xE8,0xDD,0x74,0x1F,0x4B,0xBD,0x8B,0x8A,
    0x70,0x3E,0xB5,0x66,0x48,0x03,0xF6,0x0E,0x61,0x35,0x57,0xB9,0x86,0xC1,0x1D,0x9E,
    0xE1,0xF8,0x98,0x11,0x69,0xD9,0x8E,0x94,0x9B,0x1E,0x87,0xE9,0xCE,0x55,0x28,0xDF,
    0x8C,0xA1,0x89,0x0D,0xBF,0xE6,0x42,0x68,0x41,0x99,0x2D,0x0F,0xB0,0x54,0xBB,0x16,
]
_INV_SBOX = [0] * 256
for _i, _v in enumerate(_SBOX):
    _INV_SBOX[_v] = _i
_RCON = [0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36]


def _xtime(a: int) -> int:
    a <<= 1
    return (a ^ 0x1B) & 0xFF if a & 0x100 else a


def _mul(a: int, b: int) -> int:
    r = 0
    while b:
        if b & 1:
            r ^= a
        a = _xtime(a)
        b >>= 1
    return r


def _expand_key(key: bytes) -> list[list[int]]:
    w = [list(key[i * 4:i * 4 + 4]) for i in range(4)]
    for i in range(4, 44):
        t = list(w[i - 1])
        if i % 4 == 0:
            t = t[1:] + t[:1]
            t = [_SBOX[b] for b in t]
            t[0] ^= _RCON[i // 4 - 1]
        w.append([w[i - 4][j] ^ t[j] for j in range(4)])
    return [sum(w[r * 4:r * 4 + 4], []) for r in range(11)]


_RK = _expand_key(KEY)


def _encrypt_block(b: bytes) -> bytes:
    s = list(b)
    s = [s[i] ^ _RK[0][i] for i in range(16)]
    for rnd in range(1, 11):
        s = [_SBOX[x] for x in s]
        # ShiftRows (column-major state, as in FIPS-197)
        s = [s[(i + (i % 4) * 4) % 16] for i in range(16)]
        if rnd != 10:
            ns = []
            for c in range(4):
                col = s[c * 4:c * 4 + 4]
                ns += [
                    _mul(col[0], 2) ^ _mul(col[1], 3) ^ col[2] ^ col[3],
                    col[0] ^ _mul(col[1], 2) ^ _mul(col[2], 3) ^ col[3],
                    col[0] ^ col[1] ^ _mul(col[2], 2) ^ _mul(col[3], 3),
                    _mul(col[0], 3) ^ col[1] ^ col[2] ^ _mul(col[3], 2),
                ]
            s = ns
        s = [s[i] ^ _RK[rnd][i] for i in range(16)]
    return bytes(s)


def _decrypt_block(b: bytes) -> bytes:
    s = list(b)
    s = [s[i] ^ _RK[10][i] for i in range(16)]
    for rnd in range(9, -1, -1):
        s = [s[(i - (i % 4) * 4) % 16] for i in range(16)]      # InvShiftRows
        s = [_INV_SBOX[x] for x in s]
        s = [s[i] ^ _RK[rnd][i] for i in range(16)]
        if rnd != 0:
            ns = []
            for c in range(4):
                col = s[c * 4:c * 4 + 4]
                ns += [
                    _mul(col[0], 14) ^ _mul(col[1], 11) ^ _mul(col[2], 13) ^ _mul(col[3], 9),
                    _mul(col[0], 9) ^ _mul(col[1], 14) ^ _mul(col[2], 11) ^ _mul(col[3], 13),
                    _mul(col[0], 13) ^ _mul(col[1], 9) ^ _mul(col[2], 14) ^ _mul(col[3], 11),
                    _mul(col[0], 11) ^ _mul(col[1], 13) ^ _mul(col[2], 9) ^ _mul(col[3], 14),
                ]
            s = ns
    return bytes(s)


def _pure_enc(data: bytes) -> bytes:
    out, prev = b"", IV
    for i in range(0, len(data), 16):
        blk = bytes(x ^ y for x, y in zip(data[i:i + 16], prev))
        prev = _encrypt_block(blk)
        out += prev
    return out


def _pure_dec(data: bytes) -> bytes:
    out, prev = b"", IV
    for i in range(0, len(data), 16):
        blk = data[i:i + 16]
        out += bytes(x ^ y for x, y in zip(_decrypt_block(blk), prev))
        prev = blk
    return out


_ENC, _DEC, BACKEND = _load_backend()


def encrypt(plain: bytes) -> bytes:
    pad = 16 - (len(plain) % 16)
    return _ENC(plain + bytes([pad]) * pad)


def decrypt(cipher: bytes) -> bytes:
    if not cipher or len(cipher) % 16:
        raise ValueError(f"ciphertext length {len(cipher)} is not a multiple of 16")
    out = _DEC(cipher)
    pad = out[-1]
    if 1 <= pad <= 16 and out[-pad:] == bytes([pad]) * pad:
        out = out[:-pad]
    return out


def enc_str(s: str) -> bytes:
    return encrypt(bytes([TAG_STRING]) + s.encode("latin-1"))


def enc_num(x: float) -> bytes:
    return encrypt(bytes([TAG_NUMBER]) + struct.pack("<d", x))


def describe(plain: bytes) -> str:
    """Human-readable rendering of a decrypted value."""
    if not plain:
        return "<empty>"
    tag, body = plain[0], plain[1:]
    if tag == TAG_STRING:
        return body.decode("latin-1")
    if tag == TAG_NUMBER and len(body) == 8:
        v = struct.unpack("<d", body)[0]
        return f"{v:g}" if v == int(v) else repr(v)
    if tag == TAG_BOOL:
        return f"bool:{body.hex()}"
    return f"<tag {tag}:{body.hex()}>"


# --------------------------------------------------------------------------
# Lua string literals
# --------------------------------------------------------------------------
_LUA_ESC = {"a": 7, "b": 8, "f": 12, "n": 10, "r": 13, "t": 9, "v": 11,
            "\\": 92, '"': 34, "'": 39, "[": 91, "]": 93}


def lua_unescape(s: str) -> bytes:
    out, i, n = bytearray(), 0, len(s)
    while i < n:
        c = s[i]
        if c != "\\":
            out.append(ord(c) & 0xFF)
            i += 1
            continue
        i += 1
        if i >= n:
            break
        c = s[i]
        if c.isdigit():
            num = ""
            while i < n and s[i].isdigit() and len(num) < 3:
                num += s[i]
                i += 1
            out.append(int(num) & 0xFF)
        else:
            out.append(_LUA_ESC.get(c, ord(c) & 0xFF))
            i += 1
    return bytes(out)


def lua_escape(b: bytes) -> str:
    """
    Lua literal for arbitrary bytes.

    Decimal escapes are ALWAYS padded to three digits. Lua consumes up to three
    digits after a backslash, so "\\11" followed by a literal '9' would read
    back as \\119 -- a corruption that is invisible until the game misbehaves.
    """
    out = ['"']
    for x in b:
        if x == 0x22:
            out.append('\\"')
        elif x == 0x5C:
            out.append("\\\\")
        elif 0x20 <= x <= 0x7E:
            out.append(chr(x))
        else:
            out.append(f"\\{x:03d}")
    out.append('"')
    lit = "".join(out)
    if lua_unescape(lit[1:-1]) != b:
        raise AssertionError(f"lua_escape is not round-trip safe for {b.hex()}")
    return lit


# --------------------------------------------------------------------------
# Gateway parsing
#
# Keys and values arrive both as literals and through reused locals, so string
# assignments must be tracked as the file is read:
#     L2_1 = "<slotkey>"; L3_1 = {}; L3_1["<f>"] = "<v>"; L3_1[L5]=L4
#     L1_1[L2_1] = L3_1
# --------------------------------------------------------------------------
_V = r"L\d+(?:_\d+)?"
_S = r'"((?:[^"\\]|\\.)*)"'
_RE_NEW = re.compile(rf"^\s*({_V})\s*=\s*\{{\}}\s*$")
_RE_SET = re.compile(rf"^\s*({_V})\s*=\s*{_S}\s*$")
_RE_LL = re.compile(rf"^\s*({_V})\[{_S}\]\s*=\s*{_S}\s*$")
_RE_LV = re.compile(rf"^\s*({_V})\[{_S}\]\s*=\s*({_V})\s*$")
_RE_VL = re.compile(rf"^\s*({_V})\[({_V})\]\s*=\s*{_S}\s*$")
_RE_VV = re.compile(rf"^\s*({_V})\[({_V})\]\s*=\s*({_V})\s*$")


class Slot:
    __slots__ = ("key", "fields", "line", "outer", "key_var", "tbl_var")

    def __init__(self, key, fields, line, outer, key_var, tbl_var):
        self.key, self.fields, self.line = key, fields, line
        self.outer, self.key_var, self.tbl_var = outer, key_var, tbl_var

    @property
    def sku(self) -> str:
        return describe(decrypt(self.key))

    def decoded(self) -> list[tuple[str, bytes]]:
        return [(describe(decrypt(f)), decrypt(v)) for f, v in self.fields]

    def get(self, name: str):
        for k, v in self.decoded():
            if k == name:
                return describe(v)
        return None


def read_lines(path: str) -> list[str]:
    with open(path, "r", encoding="latin-1", newline="") as fh:
        return fh.read().split("\n")


def parse(path: str) -> list[Slot]:
    tables: dict[str, list] = {}
    vals: dict[str, bytes] = {}
    slots: list[Slot] = []
    for idx, raw in enumerate(read_lines(path)):
        line = raw.rstrip("\r")
        m = _RE_NEW.match(line)
        if m:
            tables[m[1]] = []
            vals.pop(m[1], None)
            continue
        m = _RE_LL.match(line)
        if m:
            tables.setdefault(m[1], []).append((lua_unescape(m[2]), lua_unescape(m[3])))
            continue
        m = _RE_LV.match(line)
        if m:
            if tables.get(m[3]):
                slots.append(Slot(lua_unescape(m[2]), list(tables[m[3]]), idx, m[1], None, m[3]))
            elif m[3] in vals:
                tables.setdefault(m[1], []).append((lua_unescape(m[2]), vals[m[3]]))
            continue
        m = _RE_VL.match(line)
        if m:
            if m[2] in vals:
                tables.setdefault(m[1], []).append((vals[m[2]], lua_unescape(m[3])))
            continue
        m = _RE_VV.match(line)
        if m:
            if tables.get(m[3]) and m[2] in vals:
                slots.append(Slot(vals[m[2]], list(tables[m[3]]), idx, m[1], m[2], m[3]))
            elif m[2] in vals and m[3] in vals:
                tables.setdefault(m[1], []).append((vals[m[2]], vals[m[3]]))
            continue
        m = _RE_SET.match(line)
        if m:
            vals[m[1]] = lua_unescape(m[2])
    return slots


def gateway_files(args) -> list[str]:
    if args.file:
        return list(args.file)
    if not args.game:
        sys.exit("give --game <game folder> or --file <gateway.lua> [...]")
    found = sorted(glob.glob(os.path.join(args.game, "assets", "gateway*.lua")))
    if not found:
        sys.exit(f"no gateway*.lua under {os.path.join(args.game, 'assets')}")
    return found


def is_avatar(s: Slot) -> bool:
    return (s.get("TagType") or "").upper() == "AVATAR"


# --------------------------------------------------------------------------
# Commands
# --------------------------------------------------------------------------
def cmd_check(args) -> int:
    bad = 0
    for path in gateway_files(args):
        slots = parse(path)
        keys_ok = keys_bad = f_ok = f_bad = n_ok = n_bad = 0
        for s in slots:
            if enc_str(describe(decrypt(s.key))) == s.key:
                keys_ok += 1
            else:
                keys_bad += 1
            for f, v in s.fields:
                if enc_str(describe(decrypt(f))) == f:
                    f_ok += 1
                else:
                    f_bad += 1
                pv = decrypt(v)
                if pv[:1] == bytes([TAG_STRING]):
                    (f_ok, f_bad) = (f_ok + 1, f_bad) if enc_str(pv[1:].decode("latin-1")) == v else (f_ok, f_bad + 1)
                elif pv[:1] == bytes([TAG_NUMBER]) and len(pv) == 9:
                    (n_ok, n_bad) = (n_ok + 1, n_bad) if enc_num(struct.unpack("<d", pv[1:])[0]) == v else (n_ok, n_bad + 1)
        total_bad = keys_bad + f_bad + n_bad
        bad += total_bad
        print(f"{os.path.basename(path):<18} {len(slots):>4} slots | "
              f"keys {keys_ok}/{keys_ok + keys_bad} | strings {f_ok}/{f_ok + f_bad} | "
              f"numbers {n_ok}/{n_ok + n_bad}  ->  {'OK' if total_bad == 0 else 'MISMATCH'}")
    print(f"\nbackend: {BACKEND}")
    if bad:
        print("FAILED -- do not write with this build.")
        return 1
    print("Crypto reproduces every file exactly. Safe to write.")
    return 0


def cmd_list(args) -> int:
    path = gateway_files(args)[0]
    rows = [s for s in parse(path) if is_avatar(s)]
    rows.sort(key=lambda s: s.sku)
    print(f"{os.path.basename(path)} -- {len(rows)} AVATAR slots\n")
    print(f"{'sku_id':<10} {'name':<30} {'displayName':<24} playset")
    for s in rows:
        print(f"{s.sku:<10} {(s.get('name') or ''):<30} {(s.get('displayName') or ''):<24} {(s.get('playset') or '')[:44]}")
    return 0


def cmd_dump(args) -> int:
    path = gateway_files(args)[0]
    shown = 0
    for s in parse(path):
        body = "\n".join(f"     {k:<16} = {describe(v)}" for k, v in s.decoded())
        text = f"[{s.sku}]\n{body}"
        if args.filter and args.filter.lower() not in text.lower():
            continue
        print(text + "\n")
        shown += 1
    if args.filter:
        print(f"# {shown} slots matching {args.filter!r}")
    return 0


def cmd_freesku(args) -> int:
    used = collect_used_skus(gateway_files(args), args.game)
    lo, hi = SKU_RANGE
    print(f"{len(used)} sku_id already in use (gateway + catalog)")
    free = [str(n) for n in range(lo, min(hi, lo + 60) + 1) if str(n) not in used]
    print(f"\nfree in {lo}-{hi} (first {len(free)} shown):")
    print("  " + ", ".join(free[:24]) + (" ..." if len(free) > 24 else ""))
    if args.name:
        print(f"\nwould allocate for {args.name!r}: {allocate_sku(args.name, used)}")
    print("\n`register` allocates one for you -- you rarely need this command.")
    return 0


def collect_used_skus(files: list[str], game: str | None) -> set[str]:
    used: set[str] = set()
    for path in files:
        used |= {s.sku for s in parse(path)}
    if game:
        cat = os.path.join(game, "assets", "presentation", "virtualreaderpc_data.lua")
        if os.path.exists(cat):
            with open(cat, "r", encoding="latin-1", errors="replace") as fh:
                used |= set(re.findall(r'sku_id\s*=\s*"(\d+)"', fh.read()))
    for extra in ("1000300", "1000301"):      # Breeze uses these for Dory/Nemo
        used.add(extra)
    return used


SKU_RANGE = (1000340, 1000999)


def hash_sku(name: str) -> str:
    """
    Derive a sku_id from the character name.

    Must stay byte-identical to Gateway::allocateSku (src/gateway.cpp) and to
    the Lua mirror in src/api/12_virtualreader.lua, since all three compute the
    id for the same character independently. Hence multiply-and-add rather than
    anything XOR-based: Lua 5.1 has no bitwise operators, and the intermediate
    values here stay inside a double's exact-integer range so every language
    agrees.
    """
    lo, hi = SKU_RANGE
    h = 0
    for b in name.encode("utf-8"):
        h = (h * 31 + b) % 2147483648
    return str(lo + h % (hi - lo + 1))


def allocate_sku(name: str, used: set[str]) -> str:
    """Hashed id for `name`, scanning forward if that one is already taken."""
    lo, hi = SKU_RANGE
    span = hi - lo + 1
    start = int(hash_sku(name))
    for i in range(span):
        cand = str(lo + ((start - lo + i) % span))
        if cand not in used:
            return cand
    raise SystemExit(f"no free sku_id left in {lo}-{hi}")


CHARACTER_TEMPLATE = '''\
-- Generated by di3_gateway.py register on {date}.
--
-- sku_id {sku} was allocated automatically and is already registered in every
-- assets/gateway*.lua as a figure named "{name}". The two must stay in sync:
-- changing the sku here without re-running `register` brings back the
-- "Figurine Disney Infinity manquante" popup.
--
-- Name must match an ActorList actor with Type = "Avatar"
-- (assets/gamedb/core/*_actors.lua). See characters/README.md.
Crabe.VirtualReader.exposeCharacter({{
    Name = "{name}",
    sku_id = "{sku}",{tree}{icon}{desc}{meta}
}})
'''


def cmd_register(args) -> int:
    """Allocate a sku, write the gateway slots, and emit the characters/ file."""
    files = gateway_files(args)
    used = collect_used_skus(files, args.game)
    sku = args.sku or allocate_sku(args.name, used)
    if sku in used:
        sys.exit(f"sku {sku} is already taken")

    out_dir = args.out or os.path.dirname(os.path.abspath(__file__))
    out_file = os.path.join(out_dir, f"CRABE_{args.name}.lua")

    print(f"character : {args.name}")
    print(f"sku_id    : {sku}" + ("" if args.sku else "  (allocated automatically)"))
    print(f"clone from: {args.clone_from}")
    print(f"lua file  : {out_file}")
    print()

    def opt(key, val):
        return f'\n    {key} = "{val}",' if val else ""

    body = CHARACTER_TEMPLATE.format(
        date=_dt.date.today().isoformat(),
        name=args.name,
        sku=sku,
        tree=opt("ProgressionTree", args.tree),
        icon=opt("Icon", args.icon),
        desc=opt("Description", args.description),
        meta=opt("MetaData", args.metadata),
    )

    gw_args = argparse.Namespace(
        game=args.game, file=args.file, sku=sku, name=args.name,
        clone_from=args.clone_from, display=args.display,
        backup=args.backup, write=args.write,
    )
    rc = cmd_add(gw_args)
    if rc != 0:
        return rc

    if not args.write:
        print("\n--- would write " + os.path.basename(out_file) + " ---")
        print(body, end="")
        print("--- dry run: add --write to apply ---")
        return 0

    if os.path.exists(out_file) and not args.force:
        sys.exit(f"{out_file} already exists (use --force to overwrite)")
    with open(out_file, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(body)
    print(f"\nwrote {out_file}")
    print("Deploy characters/ to the game folder, then launch.")
    return 0


def cmd_add(args) -> int:
    files = gateway_files(args)
    stamp = _dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = args.backup or os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                             "..", "backups", f"gateway_{args.sku}_{stamp}")
    backup_dir = os.path.abspath(backup_dir)

    plans = []
    for path in files:
        slots = parse(path)
        if any(s.sku == args.sku for s in slots):
            print(f"{os.path.basename(path):<18} sku {args.sku} already present -- skipping")
            continue
        src = next((s for s in slots if s.sku == args.clone_from), None)
        if src is None:
            print(f"{os.path.basename(path):<18} clone source {args.clone_from} NOT FOUND -- skipping")
            continue
        outer, key_var = src.outer, (src.key_var or "L2_1")
        tbl_var, val_var = src.tbl_var, "L4_1"

        block = [f"{key_var} = {lua_escape(enc_str(args.sku))}", f"{tbl_var} = {{}}"]
        for k, pv in src.decoded():
            if k == "name":
                v = enc_str(args.name)
            elif k == "displayName" and args.display:
                v = enc_str(args.display)
            else:
                v = encrypt(pv)
            block.append(f"{val_var} = {lua_escape(v)}")
            block.append(f"{tbl_var}[{lua_escape(enc_str(k))}] = {val_var}")
        block.append(f"{outer}[{key_var}] = {tbl_var}")
        plans.append((path, src, block))
        print(f"{os.path.basename(path):<18} clone {args.clone_from} ({src.get('name')}) at line {src.line + 1} "
              f"-> {args.sku} \"{args.name}\", +{len(block)} lines")

    if not plans:
        print("\nnothing to do")
        return 0
    if not args.write:
        print(f"\ndry run -- add --write to apply (backups would go to {backup_dir})")
        return 0

    os.makedirs(backup_dir, exist_ok=True)
    for path, src, block in plans:
        shutil.copy2(path, os.path.join(backup_dir, os.path.basename(path)))
        lines = read_lines(path)
        at = src.line + 1
        out = lines[:at] + block + lines[at:]
        with open(path, "w", encoding="latin-1", newline="") as fh:
            fh.write("\n".join(out))
        again = parse(path)
        new = next((s for s in again if s.sku == args.sku), None)
        status = f"OK ({len(again)} slots, name={new.get('name')})" if new else "!! re-parse FAILED"
        print(f"{os.path.basename(path):<18} written -> {status}")
    print(f"\nbackups: {backup_dir}")
    print("Reminder: the engine reads .zip over loose files. If assets/startup.zip\n"
          "still exists, these edits are ignored -- see characters/README.md.")
    return 0


def main() -> int:
    p = argparse.ArgumentParser(description="Read and edit DI3 gateway (figure registry) files.")
    sub = p.add_subparsers(dest="cmd", required=True)

    def common(sp):
        sp.add_argument("--game", help="game folder (finds assets/gateway*.lua)")
        sp.add_argument("--file", nargs="*", help="explicit gateway .lua path(s)")

    for name, fn, helptext in (
        ("register", cmd_register, "allocate a sku, write the gateway slots and the characters/ file"),
        ("check", cmd_check, "verify the crypto reproduces the files exactly"),
        ("list", cmd_list, "list AVATAR slots"),
        ("dump", cmd_dump, "decrypt all slots"),
        ("freesku", cmd_freesku, "show unused sku_ids"),
        ("add", cmd_add, "clone a slot into a given sku_id (low-level; see register)"),
    ):
        sp = sub.add_parser(name, help=helptext)
        common(sp)
        sp.set_defaults(func=fn)
        if name == "dump":
            sp.add_argument("--filter", help="only show slots containing this text")
        if name == "freesku":
            sp.add_argument("--name", help="also show which id would be allocated for this name")
        if name in ("add", "register"):
            sp.add_argument("--name", required=True, help="catalog Name, e.g. TCW_MaceWindu")
            sp.add_argument("--clone-from", required=True, help="sku_id of an existing avatar slot")
            sp.add_argument("--display", help="gateway displayName (default: keep the clone source's)")
            sp.add_argument("--backup", help="backup folder (default ../backups/gateway_<sku>_<stamp>)")
            sp.add_argument("--write", action="store_true", help="actually modify the files")
            sp.add_argument("--sku", required=(name == "add"),
                            help="sku_id; omit on register to allocate one automatically")
        if name == "register":
            sp.add_argument("--tree", help="ProgressionTree, e.g. IN3_TCW_MaceWindu")
            sp.add_argument("--icon", help="Icon; omit to fall back to HUD_PlayerIcons_Default")
            sp.add_argument("--description", help="localisation key for the description")
            sp.add_argument("--metadata", help="brand/filter tags, e.g. StarWars,Franchise_TCW")
            sp.add_argument("--out", help="folder for the generated .lua (default: this folder)")
            sp.add_argument("--force", action="store_true", help="overwrite an existing .lua")

    args = p.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
