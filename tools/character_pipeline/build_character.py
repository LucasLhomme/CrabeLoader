"""build_character.py - Unified 3D Character Pipeline for Disney Infinity 3.0.

Automates custom character creation:
1. Clones base humanoid actor templates (EMP_Luke, etc.) and establishes new actor identity.
2. Derives deterministic SKU_ID and configures declarative mod.json.
3. Generates HUD icon texture (.tbody) and .mtb bundle if an icon image is provided.
4. Generates loose VFS layout ready for CrabeLoader V2 without editing vanilla archives.
5. Emits self-contained deploy.ps1 script.

Usage:
    python build_character.py --base EMP_Luke --name SOR_Sora --mod sora [--icon icon.png] [--deploy]
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

# Paths
PIPELINE_DIR = Path(__file__).resolve().parent
TOOLS_DIR = PIPELINE_DIR.parent
QUICKBMS = TOOLS_DIR / "QuickBMS" / "quickbms.exe"
BMS_SCRIPT = TOOLS_DIR / "QuickBMS" / "disney_infinity_new.bms"
GAME_DIR = Path(r"D:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition")
CHARACTERS_DIR = GAME_DIR / "assets" / "characters"
# The .dnax files live in the shared characters archive, not in the actor's own zip.
DNAX_ARCHIVE = CHARACTERS_DIR / "characters.zip"

# Copied untouched: geometry, material bundle and animation overrides hold no path to rename.
COPIED_EXTENSIONS = (".mtb", "_0.vbuf", "_0.ibuf", ".animtreeoverrides")


def extract_archive(archive: Path, pattern: str, out_dir: Path):
    out_dir.mkdir(parents=True, exist_ok=True)
    selector = [] if pattern == "*" else ["-f", pattern]
    cmd = [str(QUICKBMS), "-Y", *selector, str(BMS_SCRIPT), str(archive), str(out_dir)]
    subprocess.run(cmd, check=True, capture_output=True)


def rename_tokens(data: bytes, renames: list[tuple[bytes, bytes]], label: str) -> bytes:
    """Replaces each exact token, case-insensitively. Every token must be found at least once."""
    for old, new in renames:
        assert len(old) == len(new), f"Length mismatch: {old!r} vs {new!r}"
        data, count = re.subn(re.escape(old), new, data, flags=re.IGNORECASE)
        if count == 0:
            print(f"  [WARN] {label}: '{old.decode(errors='replace')}' not found; left as the base.")
    return data


def clone_actor(src_dir: Path, dnax_file: Path, char_dest: Path, base: str, name: str):
    """Clones the base actor under the new name, measured against a clone that loads in game.

    Only the paths to the clone's own files and the body material are renamed. Everything
    else still names the base on purpose: animation clips (EMP_Luke_Combo_Y), the eye
    material, costumes and the base's other archives only exist under the base's name.
    """
    lbase, lname = base.lower(), name.lower()
    # A distinct body material gives the clone its own MATP hash, so its textures can be
    # swapped without touching the base. Keeps the base's tail (SOR_Luke for SOR_Sora from
    # EMP_Luke) unless both share a prefix, where only the full name stays distinct.
    tag = name[:3] + base[3:] if name[:3].lower() != base[:3].lower() else name

    for ext in COPIED_EXTENSIONS:
        src_file = src_dir / f"{lbase}{ext}"
        if src_file.exists():
            shutil.copyfile(src_file, char_dest / f"{lname}{ext}")

    bent = (src_dir / f"{lbase}.bent").read_bytes()
    bent = rename_tokens(bent, [
        (f"characters/{lbase}/{lbase}.oct\0".encode(), f"characters/{lname}/{lname}.oct\0".encode()),
    ], ".bent")
    (char_dest / f"{lname}.bent").write_bytes(bent)

    oct_data = (src_dir / f"{lbase}.oct").read_bytes()
    oct_data = rename_tokens(oct_data, [
        (f"\0{lbase}.mtb\0".encode(), f"\0{lname}.mtb\0".encode()),
        (f"\0{lbase}_0.vbuf\0".encode(), f"\0{lname}_0.vbuf\0".encode()),
        (f"\0{lbase}_0.ibuf\0".encode(), f"\0{lname}_0.ibuf\0".encode()),
        (f"materials__{lbase}__".encode(), f"materials__{tag.lower()}__".encode()),
    ], ".oct")
    # The body material's own name is the base name plus an index (EMP_Luke3); the eye's is not.
    oct_data, count = re.subn(b"(Material\0)" + re.escape(base.encode()) + rb"(?=\d*\0)",
                              b"\\g<1>" + tag.encode(), oct_data, flags=re.IGNORECASE)
    if count == 0:
        print("  [WARN] .oct: body material name not found; the clone shares the base's material.")
    (char_dest / f"{lname}.oct").write_bytes(oct_data)

    dnax = dnax_file.read_bytes()
    dnax = rename_tokens(dnax, [
        (f"characters/{base}/{base}\0".encode(), f"characters/{name}/{name}\0".encode()),
        (f"characters/{base}/{base}.animtreeoverrides\0".encode(),
         f"characters/{name}/{name}.animtreeoverrides\0".encode()),
    ], ".dnax")
    (char_dest / f"{lname}.dnax").write_bytes(dnax)


def create_deploy_script(mod_dir: Path, mod_id: str):
    """Creates a standardized deploy.ps1 for the mod."""
    script_content = f"""$ErrorActionPreference = "Stop"
$gameDir = "D:\\SteamLibrary\\steamapps\\common\\Disney Infinity 3.0 Gold Edition"
$target = Join-Path $gameDir "mods\\{mod_id}"

if (!(Test-Path $gameDir)) {{
    Write-Error "Game folder not found: $gameDir"
    exit 1
}}

Write-Host "Deploying {mod_id} -> $target" -ForegroundColor Cyan
if (Test-Path $target) {{ Remove-Item -Path $target -Recurse -Force }}
New-Item -ItemType Directory -Force -Path $target | Out-Null

Copy-Item -Path "$PSScriptRoot\\*" -Destination $target -Recurse -Force -Exclude "deploy.ps1"
Write-Host "[OK] {mod_id} deployed successfully!" -ForegroundColor Green
"""
    (mod_dir / "deploy.ps1").write_text(script_content, encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Disney Infinity 3.0 Character Build Pipeline")
    parser.add_argument("--base", default="EMP_Luke", help="Base character actor (e.g. EMP_Luke)")
    parser.add_argument("--name", required=True, help="New character actor name (e.g. SOR_Sora)")
    parser.add_argument("--mod", required=True, help="Mod folder name under mods/ (e.g. sora)")
    parser.add_argument("--out", default=None, help="Output destination folder (default: mods/<mod>)")
    parser.add_argument("--icon", default=None, help="Optional icon image (PNG/JPG)")
    parser.add_argument("--deploy", action="store_true", help="Deploy directly to game mods folder")
    args = parser.parse_args()

    base = args.base
    name = args.name
    mod_id = args.mod

    lbase = base.lower()
    lname = name.lower()

    # Renames happen in place inside binary files, so every path must keep its length.
    if len(name) != len(base):
        sys.exit(f"--name '{name}' must be as long as --base '{base}' ({len(base)} characters).")

    if args.out:
        out_root = Path(args.out)
    else:
        out_root = Path("mods") / mod_id

    out_root.mkdir(parents=True, exist_ok=True)
    char_dest = out_root / "characters" / lname
    char_dest.mkdir(parents=True, exist_ok=True)

    print(f"=== DIM2 Character Pipeline ===")
    print(f"  Base Actor:   {base} ({lbase})")
    print(f"  New Actor:    {name} ({lname})")
    print(f"  Target Mod:   {mod_id} -> {out_root}")

    # 1. Extract base template from vanilla zip if not already unpacked
    vanilla_zip = CHARACTERS_DIR / lbase / f"{lbase}.zip"
    if not vanilla_zip.exists():
        raise FileNotFoundError(f"Vanilla character zip not found: {vanilla_zip}")

    temp_extract = out_root / "_temp_base"
    print(f"[1/4] Extracting base files from {vanilla_zip.name}...")
    extract_archive(vanilla_zip, f"{lbase}/*", temp_extract)
    extract_archive(DNAX_ARCHIVE, f"characters/{lbase}.dnax", temp_extract)

    src_dir = temp_extract / lbase
    dnax_file = temp_extract / "characters" / f"{lbase}.dnax"
    if not dnax_file.exists():
        raise FileNotFoundError(f"{lbase}.dnax not found in {DNAX_ARCHIVE}")

    # 2. Clone the actor: model, material and DNA, renaming only the clone's own paths
    print(f"[2/4] Cloning model, material and DNA files...")
    clone_actor(src_dir, dnax_file, char_dest, base, name)

    # Clean up temp base
    shutil.rmtree(temp_extract, ignore_errors=True)

    # 4. Process icon if provided
    icon_name = name
    if args.icon and Path(args.icon).exists():
        print(f"[3/4] Generating HUD icon from {args.icon}...")
        icon_out = out_root / "textures"
        icon_out.mkdir(exist_ok=True)
        make_icon_script = PIPELINE_DIR / "make_icon.py"
        subprocess.run([
            sys.executable,
            str(make_icon_script),
            str(args.icon),
            "--name", icon_name,
            "--out", str(icon_out)
        ], check=True)

    # 5. Generate declarative mod.json
    print(f"[4/4] Generating declarative mod.json and deployment script...")
    mod_manifest = {
        "id": mod_id,
        "name": f"{name} Mod",
        "version": "1.0.0",
        "description": f"Custom standalone character {name} based on {base}.",
        "character": {
            "name": name,
            "baseCharacter": base,
            "icon": icon_name
        }
    }
    (out_root / "mod.json").write_text(json.dumps(mod_manifest, indent=2), encoding="utf-8")

    # Generate deploy.ps1
    create_deploy_script(out_root, mod_id)

    print(f"\n[SUCCESS] Character package built at: {out_root.resolve()}")
    if args.deploy:
        game_mods = GAME_DIR / "mods" / mod_id
        print(f"Deploying directly to: {game_mods}")
        if game_mods.exists():
            shutil.rmtree(game_mods)
        shutil.copytree(out_root, game_mods)
        print(f"[OK] Deployed to game directory.")


if __name__ == "__main__":
    main()
