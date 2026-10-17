"""Clean a model with Blender (merge pieces, fix scale/orientation, recover textures, FBX support) before import.

Runs src/scripts/blender_clean.py in a background Blender. The result goes to a temporary folder that is deleted when
the app closes, so no cleaned copy is left next to the user's files."""
import atexit
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from src.core.paths import SCRIPTS_DIR

CLEAN_SCRIPT = SCRIPTS_DIR / "blender_clean.py"
BAKE_SCRIPT = SCRIPTS_DIR / "bake_skinned_glb.py"
NEEDS_BLENDER = (".fbx",)
"""Formats the built-in loader cannot read reliably: they always go through Blender."""
_temp_dirs: list[Path] = []


def _cleanup():
    for folder in _temp_dirs:
        shutil.rmtree(folder, ignore_errors=True)


atexit.register(_cleanup)


def is_skinned_glb(path: Path) -> bool:
    """True when the .glb holds a skeleton: it must be frozen in rest pose before Blender sees it."""
    try:
        with open(path, "rb") as f:
            head = f.read(20)
            if head[:4] != b"glTF" or head[16:20] != b"JSON":
                return False
            f.seek(12)
            length = struct.unpack("<I", f.read(4))[0]
            f.read(4)
            return b'"skins"' in f.read(length)
    except OSError:
        return False


def dae_to_glb(path: Path, out: Path) -> Path:
    """Blender 5 dropped its COLLADA importer: the built-in loader reads the .dae (pieces merged, textures packed
    into one atlas) and hands Blender an equivalent .glb. The material is written as plain PBR (white factor, no
    metal) so the colours come back unchanged."""
    import numpy as np
    import trimesh

    from src.core.model import load_source
    src = load_source(path)
    mat = trimesh.visual.material.PBRMaterial(baseColorTexture=src.texture, baseColorFactor=[255, 255, 255, 255],
                                              metallicFactor=0.0, roughnessFactor=1.0)
    uv = np.c_[src.uvs[:, 0], 1 - src.uvs[:, 1]]
    trimesh.Trimesh(src.vertices, src.faces, visual=trimesh.visual.TextureVisuals(uv=uv, material=mat),
                    process=False).export(out)
    return out


def clean_model(path: str | Path, blender: str | Path, max_vertices: int = 65000, max_triangles: int = 0):
    """Return (cleaned .glb path in a temp folder, list of report lines). Raises RuntimeError with details on failure."""
    path, blender = Path(path), Path(blender)
    if not blender.is_file():
        raise RuntimeError("Blender not found: set blender.exe in the Rig & Export tab.")
    work = Path(tempfile.mkdtemp(prefix="di_clean_"))
    _temp_dirs.append(work)
    flags = subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0
    report: list[str] = []
    source = path
    if path.suffix.lower() == ".glb" and is_skinned_glb(path):
        baked = work / "baked.glb"
        proc = subprocess.run([sys.executable, str(BAKE_SCRIPT), str(path), str(baked)], capture_output=True,
                              text=True, errors="replace", creationflags=flags)
        if proc.returncode != 0 or not baked.is_file():
            raise RuntimeError("Freezing the skeleton failed:\n" + (proc.stdout + proc.stderr)[-800:])
        report.append("Rigged model: skeleton frozen in rest pose.")
        source = baked
    if path.suffix.lower() == ".dae":
        try:
            source = dae_to_glb(path, work / "from_dae.glb")
        except Exception as exc:  # noqa: BLE001
            raise RuntimeError(f"Reading the .dae failed: {exc}") from exc
        report.append("COLLADA (.dae): converted to .glb for Blender.")
    out = work / "clean.glb"
    proc = subprocess.run([str(blender), "--background", "--factory-startup", "--python-exit-code", "1",
                           "--python", str(CLEAN_SCRIPT), "--", str(source), str(out), str(max_vertices), str(max_triangles)],
                          capture_output=True, text=True, encoding="utf-8", errors="replace", creationflags=flags)
    lines = (proc.stdout + "\n" + proc.stderr).splitlines()
    clean_lines = [ln[len("[clean] "):] for ln in lines if ln.startswith("[clean]")]
    if proc.returncode != 0 or not out.is_file():
        tail = [ln for ln in lines if ln.strip()][-10:]
        raise RuntimeError("Blender could not clean this model:\n" + "\n".join(clean_lines[-8:] + tail))
    ignored = ("Modèle debout", "Pose non évaluée")  # the Creator stands Z-up files up by itself
    report += [ln for ln in clean_lines if ln.startswith(("ATTENTION", "Texture", "Aucune texture", "  autres", "Décimation"))
               and not any(word in ln for word in ignored)]
    return out, report
