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

# Base script directory
PIPELINE_DIR = Path(__file__).resolve().parent

# Copied untouched: geometry, material bundle and animation overrides hold no path to rename.
COPIED_EXTENSIONS = (".mtb", "_0.vbuf", "_0.ibuf", ".animtreeoverrides")


def resolve_game_dir(custom_path: str | None = None) -> Path:
    """Finds the Disney Infinity 3.0 game installation directory."""
    candidates: list[Path] = []
    if custom_path:
        candidates.append(Path(custom_path))

    # Check current working directory
    if (Path.cwd() / "DisneyInfinity3.exe").exists() or (Path.cwd() / "assets" / "characters").exists():
        candidates.append(Path.cwd())

    # Check script parent directory
    if (PIPELINE_DIR / "DisneyInfinity3.exe").exists() or (PIPELINE_DIR / "assets" / "characters").exists():
        candidates.append(PIPELINE_DIR)

    # Default Steam installation path
    candidates.append(Path(r"D:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition"))

    for path in candidates:
        if path.exists() and (path / "assets" / "characters").exists():
            return path.resolve()
        if (path / "DisneyInfinity3.exe").exists():
            return path.resolve()

    return candidates[0] if candidates else Path(r"D:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition")


def resolve_tools(custom_quickbms: str | None = None, custom_script: str | None = None) -> tuple[Path, Path]:
    """Resolves valid paths for quickbms.exe and disney_infinity_new.bms."""
    quickbms_candidates: list[Path] = []
    if custom_quickbms:
        quickbms_candidates.append(Path(custom_quickbms))

    # Environment variables
    if os.environ.get("QUICKBMS_PATH"):
        quickbms_candidates.append(Path(os.environ["QUICKBMS_PATH"]))
    if os.environ.get("CRABELOADER_DIR"):
        quickbms_candidates.append(Path(os.environ["CRABELOADER_DIR"]) / "tools" / "QuickBMS" / "quickbms.exe")

    # Relative to script location (standard repo layout: tools/character_pipeline/../QuickBMS)
    quickbms_candidates.extend([
        PIPELINE_DIR.parent / "QuickBMS" / "quickbms.exe",
        PIPELINE_DIR / "QuickBMS" / "quickbms.exe",
        PIPELINE_DIR / "quickbms.exe",
    ])

    # Relative to current working directory
    quickbms_candidates.extend([
        Path.cwd() / "tools" / "QuickBMS" / "quickbms.exe",
        Path.cwd() / "QuickBMS" / "quickbms.exe",
        Path.cwd() / "quickbms.exe",
    ])

    # Check system PATH
    which_bms = shutil.which("quickbms.exe") or shutil.which("quickbms")
    if which_bms:
        quickbms_candidates.append(Path(which_bms))

    # Known dev repository fallback
    quickbms_candidates.append(Path(r"e:\Dev\DIM2\CrabeLoader\tools\QuickBMS\quickbms.exe"))

    quickbms_path: Path | None = None
    for cand in quickbms_candidates:
        if cand.is_file():
            quickbms_path = cand.resolve()
            break

    if not quickbms_path:
        tested_paths = "\n".join(f"  - {p}" for p in quickbms_candidates[:6])
        sys.exit(
            f"\n[ERROR] QuickBMS executable ('quickbms.exe') was not found!\n"
            f"Searched candidate locations:\n{tested_paths}\n\n"
            f"Fixes:\n"
            f"  1. Run this script directly from the CrabeLoader repo: tools/character_pipeline/build_character.py\n"
            f"  2. Or pass the path explicitly: --quickbms <path_to_quickbms.exe>\n"
            f"  3. Or place the 'QuickBMS' folder alongside this script or in your current working directory."
        )

    # Resolve BMS Script
    script_candidates: list[Path] = []
    if custom_script:
        script_candidates.append(Path(custom_script))

    # Next to resolved quickbms
    script_candidates.append(quickbms_path.parent / "disney_infinity_new.bms")

    # Script location relatives
    script_candidates.extend([
        PIPELINE_DIR.parent / "QuickBMS" / "disney_infinity_new.bms",
        PIPELINE_DIR / "QuickBMS" / "disney_infinity_new.bms",
        PIPELINE_DIR / "disney_infinity_new.bms",
        Path.cwd() / "tools" / "QuickBMS" / "disney_infinity_new.bms",
        Path.cwd() / "QuickBMS" / "disney_infinity_new.bms",
        Path(r"e:\Dev\DIM2\CrabeLoader\tools\QuickBMS\disney_infinity_new.bms"),
    ])

    bms_script_path: Path | None = None
    for cand in script_candidates:
        if cand.is_file():
            bms_script_path = cand.resolve()
            break

    if not bms_script_path:
        sys.exit(
            f"\n[ERROR] Disney Infinity QuickBMS script ('disney_infinity_new.bms') was not found!\n"
            f"Fixes:\n"
            f"  1. Pass --bms-script <path_to_disney_infinity_new.bms>\n"
            f"  2. Or ensure 'disney_infinity_new.bms' is present in: {quickbms_path.parent}"
        )

    return quickbms_path, bms_script_path


def resolve_make_icon() -> Path | None:
    """Finds make_icon.py across common locations."""
    candidates = [
        PIPELINE_DIR / "make_icon.py",
        PIPELINE_DIR.parent / "character_pipeline" / "make_icon.py",
        Path.cwd() / "tools" / "character_pipeline" / "make_icon.py",
        Path(r"e:\Dev\DIM2\CrabeLoader\tools\character_pipeline\make_icon.py"),
    ]
    for cand in candidates:
        if cand.is_file():
            return cand.resolve()
    return None


def extract_archive(quickbms: Path, bms_script: Path, archive: Path, pattern: str, out_dir: Path):
    if not archive.exists():
        raise FileNotFoundError(f"Archive not found: {archive}")
    out_dir.mkdir(parents=True, exist_ok=True)
    selector = [] if pattern == "*" else ["-f", pattern]
    cmd = [str(quickbms), "-Y", *selector, str(bms_script), str(archive), str(out_dir)]
    try:
        subprocess.run(cmd, check=True, capture_output=True, text=True)
    except subprocess.CalledProcessError as e:
        print(f"\n[ERROR] QuickBMS failed while extracting '{pattern}' from '{archive.name}' (exit code {e.returncode}):", file=sys.stderr)
        if e.stdout:
            print(f"[QuickBMS stdout]:\n{e.stdout}", file=sys.stderr)
        if e.stderr:
            print(f"[QuickBMS stderr]:\n{e.stderr}", file=sys.stderr)
        raise


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


def create_deploy_script(mod_dir: Path, mod_id: str, game_dir: Path):
    """Creates a standardized deploy.ps1 for the mod."""
    script_content = f"""$ErrorActionPreference = "Stop"
$gameDir = "{str(game_dir)}"
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
    parser.add_argument("--game-dir", default=None, help="Path to Disney Infinity 3.0 installation folder")
    parser.add_argument("--quickbms", default=None, help="Explicit path to quickbms.exe")
    parser.add_argument("--bms-script", default=None, help="Explicit path to disney_infinity_new.bms")
    args = parser.parse_args()

    base = args.base
    name = args.name
    mod_id = args.mod

    lbase = base.lower()
    lname = name.lower()

    # Renames happen in place inside binary files, so every path must keep its length.
    if len(name) != len(base):
        sys.exit(f"--name '{name}' must be as long as --base '{base}' ({len(base)} characters).")

    # Resolve dependencies and environment
    game_dir = resolve_game_dir(args.game_dir)
    characters_dir = game_dir / "assets" / "characters"
    dnax_archive = characters_dir / "characters.zip"

    quickbms, bms_script = resolve_tools(args.quickbms, args.bms_script)

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
    print(f"  Game Path:    {game_dir}")
    print(f"  QuickBMS:     {quickbms}")

    # 1. Extract base template from vanilla zip if not already unpacked
    vanilla_zip = characters_dir / lbase / f"{lbase}.zip"
    if not vanilla_zip.exists():
        raise FileNotFoundError(f"Vanilla character zip not found: {vanilla_zip}")

    temp_extract = out_root / "_temp_base"
    print(f"[1/4] Extracting base files from {vanilla_zip.name}...")
    extract_archive(quickbms, bms_script, vanilla_zip, f"{lbase}/*", temp_extract)
    extract_archive(quickbms, bms_script, dnax_archive, f"characters/{lbase}.dnax", temp_extract)

    src_dir = temp_extract / lbase
    dnax_file = temp_extract / "characters" / f"{lbase}.dnax"
    if not dnax_file.exists():
        raise FileNotFoundError(f"{lbase}.dnax not found in {dnax_archive}")

    # 2. Clone the actor: model, material and DNA, renaming only the clone's own paths
    print(f"[2/4] Cloning model, material and DNA files...")
    clone_actor(src_dir, dnax_file, char_dest, base, name)

    # Clean up temp base
    shutil.rmtree(temp_extract, ignore_errors=True)

    # 3. Process icon if provided
    icon_name = name
    if args.icon and Path(args.icon).exists():
        print(f"[3/4] Generating HUD icon from {args.icon}...")
        icon_out = out_root / "textures"
        icon_out.mkdir(exist_ok=True)
        make_icon_script = resolve_make_icon()
        if make_icon_script and make_icon_script.exists():
            subprocess.run([
                sys.executable,
                str(make_icon_script),
                str(args.icon),
                "--name", icon_name,
                "--out", str(icon_out)
            ], check=True)
        else:
            print(f"  [WARN] make_icon.py could not be found; skipping icon generation.")

    # 4. Generate declarative mod.json
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
    create_deploy_script(out_root, mod_id, game_dir)

    print(f"\n[SUCCESS] Character package built at: {out_root.resolve()}")
    if args.deploy:
        game_mods = game_dir / "mods" / mod_id
        print(f"Deploying directly to: {game_mods}")
        if game_mods.exists():
            shutil.rmtree(game_mods)
        shutil.copytree(out_root, game_mods)
        print(f"[OK] Deployed to game directory.")


if __name__ == "__main__":
    main()