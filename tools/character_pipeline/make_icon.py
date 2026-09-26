"""make_icon.py - Avalanche Octane HUD Icon Generator for Disney Infinity 3.0.

Converts any source image (PNG/JPEG) into a valid Octane .tbody texture (DDS DXT5)
and generates a companion .mtb material definition for character and item icons.

Usage:
    python make_icon.py input.png --name SOR_Sora --out ./icons
"""
import argparse
import hashlib
import io
import struct
from pathlib import Path
from PIL import Image


def compute_texture_hash(name: str) -> str:
    """Computes a deterministic 16-hex character hash (64-bit) for the texture."""
    h = hashlib.sha256(name.encode("utf-8")).hexdigest()
    return h[:16]


def create_dds_dxt5(img: Image.Image) -> bytes:
    """Generates standard DDS DXT5 bytes with full mipmap chain."""
    img = img.convert("RGBA")
    # Ensure power of two
    w, h = img.size
    target_w = 1 << (w - 1).bit_length()
    target_h = 1 << (h - 1).bit_length()
    if (w, h) != (target_w, target_h):
        img = img.resize((target_w, target_h), Image.Resampling.LANCZOS)

    levels = []
    cur = img
    fmt = "DXT5"
    while True:
        buf = io.BytesIO()
        cur.save(buf, "DDS", pixel_format=fmt)
        # Skip 128-byte DDS header from PIL buffer to extract raw compressed blocks
        raw_payload = buf.getvalue()[128:]
        levels.append(raw_payload)
        if min(cur.size) <= 4:
            break
        cur = cur.resize((cur.size[0] // 2, cur.size[1] // 2), Image.Resampling.LANCZOS)

    size = img.size
    header = struct.pack(
        "<4s7I44s2I4s5I5I",
        b"DDS ",
        124,
        0x000A1007,
        size[1],
        size[0],
        len(levels[0]),
        0,
        len(levels),
        b"\0" * 44,
        32,
        0x4,
        fmt.encode(),
        0,
        0,
        0,
        0,
        0,
        0x401008,
        0,
        0,
        0,
        0
    )
    return header + b"".join(levels)


def build_icon_mtb(tex_hash_hex: str, icon_name: str) -> bytes:
    """Constructs a standard 80-byte DI3 Material/Texture Bundle (.mtb) for HUD icons."""
    # Standard Octane HUD icon MTB template
    tex_hash_bytes = bytes.fromhex(tex_hash_hex)
    mat_guid = hashlib.md5(icon_name.encode("utf-8")).digest()
    mat_hash = hashlib.sha1(icon_name.encode("utf-8")).digest()[:4]

    header = bytearray(80)
    header[0:4] = b"MTB\0"
    header[4:8] = struct.pack("<I", 1)  # Version 1
    header[8:12] = b"MATP"              # Material properties section
    header[12:16] = mat_hash           # MATP hash for engine cache
    header[16:32] = mat_guid           # Material GUID
    header[32:40] = tex_hash_bytes     # Diffuse slot texture hash
    header[40:48] = b"\0" * 8          # Normal slot (none for flat HUD icon)
    header[48:52] = struct.pack("<I", 0x00010000) # Flags: 2D HUD UI layer
    return bytes(header)


def main():
    parser = argparse.ArgumentParser(description="Disney Infinity 3.0 Icon Generator")
    parser.add_argument("input", help="Source image (PNG or JPG)")
    parser.add_argument("--name", required=True, help="Internal icon identifier (e.g. SOR_Sora)")
    parser.add_argument("--out", default=".", help="Output directory for generated .tbody and .mtb")
    args = parser.parse_args()

    input_path = Path(args.input)
    if not input_path.exists():
        raise FileNotFoundError(f"Input file not found: {input_path}")

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    img = Image.open(input_path)
    tex_hash = compute_texture_hash(args.name)
    dds_bytes = create_dds_dxt5(img)
    mtb_bytes = build_icon_mtb(tex_hash, args.name)

    tbody_path = out_dir / f"{tex_hash}.tbody"
    mtb_path = out_dir / f"{args.name}.mtb"

    tbody_path.write_bytes(dds_bytes)
    mtb_path.write_bytes(mtb_bytes)

    print(f"[OK] Generated HUD Icon for '{args.name}':")
    print(f"  Texture:  {tbody_path} (Hash: {tex_hash})")
    print(f"  Material: {mtb_path} (Size: {len(mtb_bytes)} bytes)")


if __name__ == "__main__":
    main()
