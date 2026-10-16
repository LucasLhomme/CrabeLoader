"""Write a rigged character into <game>/mods/<slug>/ as a self-contained CrabeLoader mod, then check it."""
import json
import uuid
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable

import numpy as np
from PIL import Image

from src.core.material_bundle import MaterialBundle, character_bundle
from src.core.mod_writer import Identity, backup_existing, ship_motions_if_needed, write_text_files
from src.core.octane import build_buffers, clone_bent, clone_dnax, material_count, patch_oct
from src.core.octane_scene import Scene
from src.core.paths import WORK_DIR
from src.core.rig import RigResult
from src.core.textures import build_texture_archive, choose_texture_ids, export_loose_textures


@dataclass
class InstallReport:
    mod_dir: Path
    actor: str
    backup: Path | None
    checks: list[tuple[bool, str]] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return all(passed for passed, _ in self.checks)


from src.core.npc_config import EntityConfig
from src.core.abilities import CharacterAbilities
import shutil


def install(rig: RigResult, texture: Image.Image, ident: Identity, game_dir: Path,
            log: Callable[[str], None], entity_config: EntityConfig | None = None,
            abilities: CharacterAbilities | None = None,
            use_legacy_zip: bool = False, icon: Image.Image | None = None) -> InstallReport:
    base = rig.base
    game_dir = Path(game_dir)
    if not (game_dir / "assets").is_dir():
        raise FileNotFoundError(f"{game_dir} is not the game folder (no assets subfolder)")
    mod_dir = game_dir / "mods" / ident.slug

    ids = choose_texture_ids(ident.actor, game_dir, ident.slug)
    log(f"Base: {base.display_name} ({base.actor}, model {base.model}, {len(base.meshes)} mesh(es))")
    if entity_config and not entity_config.is_player:
        log(f"Entity: {entity_config.summary()}")
    if abilities:
        log(f"Gameplay: {abilities.summary(base)}")
    log(f"Textures: diffuse {ids.diffuse}.tbody, normal {ids.normal}.tbody (prefix {ids.prefix})")

    # Build everything in memory first so a failure never leaves a half-written mod behind.
    vbuf, ibuf, layout = build_buffers(base, rig)
    n_materials = material_count(base)
    uuids = [str(uuid.uuid5(uuid.NAMESPACE_URL, f"DisneyInfinityCreator/{ident.lower}/{k}")) for k in range(n_materials)]
    files = {
        f"{ident.lower}_0.vbuf": vbuf,
        f"{ident.lower}_0.ibuf": ibuf,
        f"{ident.lower}.oct": patch_oct(base, rig, ident.actor, vbuf, ibuf, layout),
        f"{ident.lower}.mtb": character_bundle(n_materials, ids.diffuse, ids.normal, ids.material_hash, uuids),
        f"{ident.lower}.bent": clone_bent(base, ident.actor),
        f"{ident.lower}.dnax": clone_dnax(base, ident.actor),
    }

    # Borrow animtree overrides if available from abilities combat source or base
    animtree_src = abilities.get_effective_animtree_source(base) if abilities else base
    animtree_bytes = None
    if animtree_src and animtree_src.animtree_file:
        animtree_bytes = animtree_src.animtree_file[1].read_bytes()
    elif base.animtree_file:
        animtree_bytes = base.animtree_file[1].read_bytes()

    if animtree_bytes:
        files[f"{ident.lower}.animtreeoverrides"] = animtree_bytes

    backup = backup_existing(mod_dir)
    report = InstallReport(mod_dir, ident.actor, backup)
    if backup:
        log(f"Previous version of the mod backed up to {backup}")
    char_dir = mod_dir / "characters" / ident.lower
    char_dir.mkdir(parents=True)
    for name, data in files.items():
        (char_dir / name).write_bytes(data)
    write_text_files(mod_dir, ident, base, entity_config, abilities)

    if abilities and abilities.custom_animations:
        custom_dir = char_dir / "custom_anims"
        custom_dir.mkdir(parents=True, exist_ok=True)
        manifest_anims = {}
        for slot, ca in abilities.custom_animations.items():
            manifest_anims[slot] = ca.to_dict()
            if ca.source_path and Path(ca.source_path).is_file():
                src_p = Path(ca.source_path)
                dest_p = custom_dir / f"{slot}_{src_p.name}"
                shutil.copy2(src_p, dest_p)
                manifest_anims[slot]["exported_file"] = dest_p.name
        (custom_dir / "manifest.json").write_text(json.dumps(manifest_anims, indent=2), encoding="utf-8")
        log(f"Packaged {len(abilities.custom_animations)} custom animation track(s)")

    log("Model files, manifest and patches written")

    log("Writing CrabeLoader V2 loose VFS textures (.tbody)...")
    export_loose_textures(ids, texture, mod_dir)
    report.notes.append(f"textures: loose VFS {ids.diffuse}.tbody, {ids.normal}.tbody")

    if icon is not None:
        from src.core.icon import write_icon
        written = write_icon(icon, ident.actor, mod_dir)
        log(f"Character icon written ({', '.join(p.name for p in written)})")
        report.notes.append("icon: head picture + HUD texture (experimental: the selection grid may keep its default icon)")

    if use_legacy_zip:
        log("Rebuilding legacy encrypted texture archive (optional fallback)...")
        tex_zip = mod_dir / "textures" / f"{ids.prefix}.zip"
        entries = build_texture_archive(ids, texture, game_dir, WORK_DIR / "textures", tex_zip)
        report.notes.append(f"textures/{ids.prefix}.zip: {entries} entries (vanilla + 2)")

    note = ship_motions_if_needed(base, game_dir, mod_dir)
    report.notes.append(note)
    log(note)

    _verify(report, rig, ident, mod_dir, ids.prefix, n_materials)
    return report



def _verify(report: InstallReport, rig: RigResult, ident: Identity, mod_dir: Path, prefix: str,
            n_materials: int) -> None:
    c = report.checks
    base = rig.base
    char_dir = mod_dir / "characters" / ident.lower
    vbuf = (char_dir / f"{ident.lower}_0.vbuf").read_bytes()
    ibuf = np.frombuffer((char_dir / f"{ident.lower}_0.ibuf").read_bytes(), "<u2")
    scene = Scene.load(char_dir / f"{ident.lower}.oct")
    geometry = [n for n in scene.top.find("SceneTreeNodePool").children if n.get("Type") == "Geometry"]

    # Every primitive's ranges stay inside the new buffers; the body primitive holds the whole custom mesh.
    ranges_ok, body = True, None
    for gi, node in enumerate(geometry):
        for pi, p in enumerate(node.find("Primitives").children):
            idata, vdata = p.get("Idata"), p.get("Vdata")
            n = vdata[1]
            seg = ibuf[idata[1] // 2:idata[1] // 2 + idata[3]]
            ranges_ok &= (vdata[3] + n * vdata[4] <= len(vbuf) and vdata[6] + n * vdata[7] <= len(vbuf)
                          and len(seg) == idata[3] and int(seg.min()) >= idata[2] and int(seg.max()) < idata[2] + n)
            if gi == base.body_mesh_index and pi == base.body_primitive.index:
                body = (vdata, idata)
    c.append((ranges_ok, f"{sum(len(g.find('Primitives').children) for g in geometry)} primitives inside "
                         f"vbuf ({len(vbuf)} B) and ibuf ({len(ibuf) // 3} triangles)"))
    c.append((body is not None and body[0][1] == len(rig.baked), f"body primitive: {len(rig.baked)} vertices"))
    s1 = np.frombuffer(vbuf[body[0][6]:body[0][6] + body[0][1] * 24], np.uint8).reshape(-1, 24)
    wsum = s1[:, 4:8].astype(int).sum(1)
    c.append((bool((wsum == 255).all()), f"skinning weights: {int((wsum != 255).sum())} vertex(es) without full weight"))
    bones = int(s1[:, :4].max())
    c.append((bones < len(base.bone_names), f"bone indices ≤ {bones} (palette of {len(base.bone_names)} bones)"))
    err = rig.bind_roundtrip_error()
    c.append((err < 1e-4, f"bind-pose skinning: max error {err:.1e}"))
    pos = rig.baked
    c.append((0.3 < pos[:, 1].max() < 5.0, f"in-game height {pos[:, 1].max():.2f} ({base.display_name} "
                                           f"{base.fit_height:.2f})"))
    names = [b.get("FileName") for pool in ("VertexBufferPool", "IndexBufferPool", "MaterialBundlePool")
             for b in scene.top.find(pool).children if b.get("FileName")]
    c.append((sorted(names) == sorted([f"{ident.lower}_0.vbuf", f"{ident.lower}_0.ibuf", f"{ident.lower}.mtb"]),
              ".oct points to the clone's buffers and material bundle"))
    mtb = MaterialBundle.from_bytes((char_dir / f"{ident.lower}.mtb").read_bytes())
    c.append((len(mtb.hashes) == n_materials == len(scene.top.find("MaterialPool").children),
              f".mtb: {n_materials} material(s), matching the .oct"))
    bent = Scene.load(char_dir / f"{ident.lower}.bent")
    c.append((f"characters/{ident.lower}/{ident.lower}.oct" in bent.strings, ".bent points to the clone's model"))
    manifest = json.loads((mod_dir / "mod.json").read_text(encoding="utf-8"))
    tex_ok = len(list((mod_dir / "textures").glob("*.tbody"))) >= 2 or (mod_dir / f"textures/{prefix}.zip").is_file()
    c.append((tex_ok, "textures verified (.tbody loose VFS / .zip)"))
    for rel in ("skilltrees/ActorList.patch", "skilltrees/DataMap.patch", f"characters/{ident.actor}.lua",
                f"characters/{ident.lower}/{ident.lower}.dnax"):
        c.append(((mod_dir / rel).is_file(), rel))

