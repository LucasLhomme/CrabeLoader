"""Developer script: regenerate src/assets/shader_layouts.json from a folder of decrypted game .mtb files.

A material's data block has a size and a sampler layout fixed by its shader, which the bundle does not record.
Both are recovered from the game's own bundles:
  * record size: a single-material bundle gives its shader's size directly; a multi-material bundle whose other
    shaders are known gives the last one (repeated until nothing new is learnt);
  * texture slots: the sampler texture indices are 8-byte integers at a fixed offset of the record; in a bundle,
    every texture is sampled by some material, so the right offset makes the indices cover the whole TEXB list.
Usage: python tools/build_shader_table.py <decrypted root>
"""
import json
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from src.core.material_bundle import MaterialBundle  # noqa: E402

OUT = Path(__file__).resolve().parents[1] / "src" / "assets" / "shader_layouts.json"
MAX_SLOTS = 8


def load(root: Path) -> list[MaterialBundle]:
    out = []
    for p in root.rglob("*.mtb"):
        try:
            b = MaterialBundle.from_bytes(p.read_bytes())
        except (ValueError, struct.error, IndexError):
            continue
        if b.hashes:
            out.append(b)
    return out


def record_sizes(bundles) -> dict[str, int]:
    sizes: dict[str, Counter] = defaultdict(Counter)
    known: dict[str, int] = {}
    changed = True
    while changed:
        changed = False
        for b in bundles:
            shaders = [b.shaders[i] for i in b.shader_ids]
            unknown = {s for s in shaders if s not in known}
            if len(unknown) != 1:
                continue
            s = unknown.pop()
            rest = len(b.data) - sum(known[x] for x in shaders if x != s)
            n = shaders.count(s)
            if rest > 0 and rest % n == 0 and (rest // n) % 16 == 0:
                sizes[s][rest // n] += 1
        for s, c in sizes.items():
            if s not in known:
                known[s] = c.most_common(1)[0][0]
                changed = True
    return known


def slots(record: bytes, offset: int, count: int) -> list[int]:
    return [struct.unpack_from("<q", record, offset + 8 * i)[0] for i in range(count)]


def _run(rec: bytes, off: int, ntex: int) -> int:
    """Longest run of plausible texture indices at `off` (trailing -1 dropped)."""
    n = 0
    while n < MAX_SLOTS and off + 8 * (n + 1) <= len(rec):
        x = slots(rec, off, n + 1)[-1]
        if not (x == -1 or 0 <= x < ntex):
            break
        n += 1
    while n and slots(rec, off, n)[-1] == -1:
        n -= 1
    return n


def texture_layouts(bundles, sizes) -> dict[str, dict]:
    parsed = []
    for b in bundles:
        shaders = [b.shaders[i] for i in b.shader_ids]
        if any(s not in sizes for s in shaders) or sum(sizes[s] for s in shaders) != len(b.data):
            continue
        records, p = [], 0
        for s in shaders:
            records.append((s, b.data[p:p + sizes[s]]))
            p += sizes[s]
        parsed.append((records, len(b.textures)))

    known: dict[str, tuple[int, int, int]] = {}
    while True:
        votes: dict[str, Counter] = defaultdict(Counter)
        for records, ntex in parsed:
            unknown = {s for s, _ in records if s not in known}
            if len(unknown) != 1:
                continue
            s = unknown.pop()
            base_used = {x for t, r in records if t in known
                         for x in slots(r, known[t][0], known[t][1]) if x >= 0}
            # Blocks are 16-aligned and start with their texture array: keep the aligned offset that makes the
            # bundle's materials sample every one of its textures.
            first = next(r for t, r in records if t == s)
            for off in range(0, len(first) - 8, 16):
                n = min(_run(r, off, ntex) for t, r in records if t == s)
                if n == 0:
                    continue
                used = base_used | {x for t, r in records if t == s for x in slots(r, off, n) if x >= 0}
                if used == set(range(ntex)):
                    votes[s][(off, n)] += 1
        if not votes:
            break
        for s, c in votes.items():
            (off, n), count = c.most_common(1)[0]
            known[s] = (off, n, count)

    table = {}
    for s, size in sizes.items():
        entry = {"record_size": size}
        if s in known:
            off, n, count = known[s]
            entry.update(texture_offset=off, texture_slots=n, samples=count)
        table[s] = entry
    return table


if __name__ == "__main__":
    bundles = load(Path(sys.argv[1]))
    sizes = record_sizes(bundles)
    table = texture_layouts(bundles, sizes)
    OUT.write_text(json.dumps(table, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    with_tex = sum("texture_offset" in v for v in table.values())
    print(f"{len(bundles)} bundles, {len(table)} shaders sized, {with_tex} with texture slots -> {OUT}")
