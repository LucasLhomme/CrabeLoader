"""Nettoyage automatique d'un modèle 3D pour Disney Infinity Creator.

Lancé par Blender en arrière-plan :
    blender --background --python blender_clean.py -- entrée sortie.glb [max_sommets]
Formats acceptés en entrée : .glb .gltf .obj .fbx, ou un .zip qui en contient un.
"""
import math
import os
import sys
import tempfile
import zipfile

import bpy
import bmesh
from mathutils import Matrix, Vector

MODEL_EXT = (".glb", ".gltf", ".obj", ".fbx")
fatal_errors = []


def log(msg):
    print(f"[clean] {msg}", flush=True)


def check(ok, label, detail="", fatal=False):
    """Une ligne de rapport : OK / ATTENTION / ÉCHEC."""
    tag = "OK       " if ok else ("ÉCHEC    " if fatal else "ATTENTION")
    log(f"{tag} {label}" + (f" - {detail}" if detail else ""))
    if not ok and fatal:
        fatal_errors.append(label)


def parse_args():
    argv = sys.argv
    args = argv[argv.index("--") + 1:] if "--" in argv else []
    if len(args) < 2:
        raise SystemExit("Usage : -- entrée sortie.glb [max_sommets]")
    return args[0], args[1], int(args[2]) if len(args) > 2 else 65000, int(args[3]) if len(args) > 3 else 0


def find_model(path):
    """Renvoie le fichier de modèle à importer (extrait le .zip si besoin)."""
    if os.path.splitext(path)[1].lower() != ".zip":
        return path
    folder = tempfile.mkdtemp(prefix="di_clean_")
    with zipfile.ZipFile(path) as z:
        z.extractall(folder)
    found = []
    for root, _, files in os.walk(folder):
        found += [os.path.join(root, f) for f in files if f.lower().endswith(MODEL_EXT)]
    if not found:
        raise SystemExit("Aucun .glb/.gltf/.obj/.fbx trouvé dans ce .zip.")
    found.sort(key=lambda p: (MODEL_EXT.index(os.path.splitext(p)[1].lower()), -os.path.getsize(p)))
    return found[0]


def import_model(path):
    ext = os.path.splitext(path)[1].lower()
    if ext in (".glb", ".gltf"):
        bpy.ops.import_scene.gltf(filepath=path)
    elif ext == ".obj":
        bpy.ops.wm.obj_import(filepath=path)
    elif ext == ".fbx":
        bpy.ops.import_scene.fbx(filepath=path)
    else:
        raise SystemExit(f"Format non pris en charge : {ext}")


IMAGE_EXT = (".png", ".jpg", ".jpeg", ".tga", ".bmp", ".webp")


def usable_image(image):
    return image is not None and (image.has_data or (image.filepath and os.path.isfile(bpy.path.abspath(image.filepath))))


def find_images(folder):
    found = []
    for root, _, files in os.walk(folder):
        found += [os.path.join(root, f) for f in files if f.lower().endswith(IMAGE_EXT)]
    return found


def repair_textures(meshes, folder):
    """Textures perdues à l'import (fichier .mtl absent, chemin cassé) : les retrouve à côté du modèle.

    1. une texture référencée mais introuvable est cherchée par son nom dans le dossier du modèle ;
    2. un morceau sans aucune texture reçoit l'image du dossier (la plus grosse, ou celle dont le nom ressemble)."""
    candidates = find_images(folder)
    if not candidates:
        return
    by_name = {os.path.basename(p).lower(): p for p in candidates}
    for mat in bpy.data.materials:
        if mat.node_tree:
            for node in mat.node_tree.nodes:
                if node.type == "TEX_IMAGE" and node.image and not usable_image(node.image):
                    name = os.path.basename(node.image.filepath.replace("\\", "/")).lower() or node.image.name.lower()
                    if name in by_name:
                        node.image.filepath = by_name[name]
                        node.image.reload()
                        log(f"Texture retrouvée à côté du modèle : {name}")
    bare = [o for o in meshes if not any(
        s.material and s.material.node_tree and any(n.type == "TEX_IMAGE" and usable_image(n.image)
                                                    for n in s.material.node_tree.nodes)
        for s in o.material_slots)]
    if not bare:
        return
    stem = os.path.splitext(os.path.basename(bpy.context.scene.get("model_name", "")))[0].lower()
    candidates.sort(key=lambda p: (stem not in os.path.basename(p).lower() if stem else 0, -os.path.getsize(p)))
    chosen = candidates[0]
    image = bpy.data.images.load(chosen, check_existing=True)
    mat = bpy.data.materials.new("texture_retrouvee")
    try:
        mat.use_nodes = True
    except Exception:
        pass
    tree = mat.node_tree
    bsdf = next((n for n in tree.nodes if n.type == "BSDF_PRINCIPLED"), None) or tree.nodes.new("ShaderNodeBsdfPrincipled")
    tex = tree.nodes.new("ShaderNodeTexImage")
    tex.image = image
    tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    for o in bare:
        o.data.materials.clear()
        o.data.materials.append(mat)
    log(f"Aucune texture liée au modèle : {os.path.basename(chosen)} appliquée à {len(bare)} morceau(x)")
    if len(candidates) > 1:
        log("  autres images du dossier (non utilisées) : " + ", ".join(os.path.basename(p) for p in candidates[1:6]))


def count_textures(obj):
    images = set()
    for slot in obj.material_slots:
        mat = slot.material
        if mat and mat.use_nodes:
            images |= {n.image.name for n in mat.node_tree.nodes if n.type == "TEX_IMAGE" and n.image}
    return len(images)


def world_bbox(o):
    pts = [o.matrix_world @ Vector(c) for c in o.bound_box]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return lo, hi


def bake_rest_pose(meshes):
    """Modèle déjà rigué (Sketchfab...) : fige la pose de repos, sinon les sommets bruts sont faux."""
    armatures = [o for o in bpy.context.scene.objects if o.type == "ARMATURE"]
    for arm in armatures:
        arm.data.pose_position = "REST"
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    baked = 0
    for o in meshes:
        if o.modifiers:
            new_me = bpy.data.meshes.new_from_object(o.evaluated_get(dg), preserve_all_data_layers=True,
                                                     depsgraph=dg)
            o.modifiers.clear()
            o.data = new_me
            baked += 1
    return len(armatures), baked


def drop_outliers(meshes):
    """Retire les morceaux très éloignés ou démesurés par rapport au plus gros morceau."""
    info = []
    for o in meshes:
        lo, hi = world_bbox(o)
        info.append((o, (lo + hi) / 2, hi - lo, len(o.data.vertices)))
    main = max(info, key=lambda t: t[3])
    ref = max(main[2])
    kept = []
    for o, center, size, n in info:
        far = (center - main[1]).length > 4 * ref or max(size) > 6 * ref
        log(f"  {o.name} : {n} sommets, taille {size.x:.1f} x {size.y:.1f} x {size.z:.1f}"
            + (" -> RETIRÉ (hors de proportion)" if far else ""))
        if far:
            bpy.data.objects.remove(o, do_unlink=True)
        else:
            kept.append(o)
    return kept


def verify_export(dst, expected_textures):
    """Réimporte le .glb exporté pour vérifier qu'il est valide."""
    size = os.path.getsize(dst) if os.path.isfile(dst) else 0
    check(size > 0, "Fichier exporté créé", f"{size / 1e6:.1f} Mo", fatal=True)
    if size == 0:
        return
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=dst)
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    verts = sum(len(o.data.vertices) for o in meshes)
    images = len(bpy.data.images)
    check(len(meshes) == 1 and verts > 0, "Le .glb se réimporte correctement",
          f"{len(meshes)} maillage(s), {verts} sommets", fatal=True)
    check(images >= expected_textures, "Textures présentes dans le .glb",
          f"{images} image(s) pour {expected_textures} attendue(s)")


def main():
    src, dst, max_verts, max_tris = parse_args()
    if os.path.isfile(dst):
        os.remove(dst)
    bpy.ops.wm.read_factory_settings(use_empty=True)

    model = find_model(src)
    log(f"Import de {os.path.basename(model)}")
    import_model(model)
    bpy.context.view_layer.update()  # matrices à jour avant de détacher les parents
    scene = bpy.context.scene

    meshes = [o for o in scene.objects if o.type == "MESH"]
    scene["model_name"] = os.path.basename(model)
    repair_textures(meshes, os.path.dirname(os.path.abspath(model)))
    others = len(scene.objects) - len(meshes)
    if not meshes:
        raise SystemExit("Aucun maillage trouvé dans ce fichier.")
    log(f"{len(meshes)} morceau(x) de maillage, {others} objet(s) vide(s)/autre(s)")

    # 0. Modèle déjà rigué : figer la pose de repos
    armatures, baked = bake_rest_pose(meshes)
    if armatures or baked:
        log(f"Modèle rigué : {armatures} squelette(s), {baked} maillage(s) figé(s) en pose de repos")
    if armatures:
        check(False, "Modèle déjà rigué",
              "le nettoyage peut le déformer : essaie d'abord de l'importer tel quel dans Creator")

    # 1. Détacher chaque maillage de ses parents (en gardant sa position)
    for o in meshes:
        world = o.matrix_world.copy()
        o.parent = None
        o.matrix_world = world
        if o.data.users > 1:
            o.data = o.data.copy()
        o.vertex_groups.clear()  # Creator refait son propre squelette
        if o.data.shape_keys:
            o.shape_key_clear()
    bpy.context.view_layer.update()

    # 2. Supprimer ce qui n'est pas un maillage (objets vides, armatures...)
    for o in list(scene.objects):
        if o not in meshes:
            bpy.data.objects.remove(o, do_unlink=True)

    # 2b. Retirer les morceaux aberrants (très éloignés ou démesurés par rapport au corps)
    log("Morceaux :")
    meshes = drop_outliers(meshes)

    # 3. Fusionner tous les morceaux en un seul objet
    target = meshes[0]
    if len(meshes) > 1:
        with bpy.context.temp_override(active_object=target, object=target,
                                       selected_objects=meshes, selected_editable_objects=meshes):
            bpy.ops.object.join()
    target = [o for o in scene.objects if o.type == "MESH"][0]

    # 4. Appliquer position / rotation / échelle
    with bpy.context.temp_override(active_object=target, object=target,
                                   selected_objects=[target], selected_editable_objects=[target]):
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # 5. Fusionner les doublons (distance proportionnelle à la taille) + sommets orphelins
    me = target.data
    xs, ys, zs = ([v.co[i] for v in me.vertices] for i in range(3))
    diag = math.dist((min(xs), min(ys), min(zs)), (max(xs), max(ys), max(zs)))
    before = len(me.vertices)
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=max(diag * 2e-5, 1e-9))
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bm.to_mesh(me)
    bm.free()
    me.update()
    log(f"Doublons : {before - len(me.vertices)} sommet(s) fusionné(s), {len(loose)} orphelin(s) retiré(s)")

    # 6. Réduire les polygones si nécessaire (marge pour les coutures UV)
    limit = int(max_verts * 0.75)
    count = len(me.vertices)
    if count > limit:
        dec = target.modifiers.new("Decimate", "DECIMATE")
        dec.ratio = limit / count
        with bpy.context.temp_override(active_object=target, object=target):
            bpy.ops.object.modifier_apply(modifier=dec.name)
        log(f"Décimation : {count} -> {len(target.data.vertices)} sommets (les détails fins peuvent s'adoucir)")

    # 6b. Budget de triangles (performances en jeu : un modèle trop lourd fait chuter les FPS)
    if max_tris > 0:
        me.calc_loop_triangles()
        tris = len(me.loop_triangles)
        if tris > max_tris:
            dec = target.modifiers.new("DecimateBudget", "DECIMATE")
            dec.ratio = max_tris / tris
            dec.use_collapse_triangulate = True
            with bpy.context.temp_override(active_object=target, object=target):
                bpy.ops.object.modifier_apply(modifier=dec.name)
            me = target.data
            me.calc_loop_triangles()
            log(f"Décimation (budget) : {tris} -> {len(me.loop_triangles)} triangles")

    # 7. Poser le modèle sur le sol, centré
    me = target.data
    xs, ys, zs = ([v.co[i] for v in me.vertices] for i in range(3))
    me.transform(Matrix.Translation(((-(min(xs) + max(xs)) / 2), (-(min(ys) + max(ys)) / 2), -min(zs))))
    me.update()
    width, depth, height = max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs)
    me.calc_loop_triangles()

    # 8. Rapport de vérification
    log("--- Rapport de vérification ---")
    textures = count_textures(target)
    check(len(scene.objects) == 1, "Un seul objet dans la scène", f"{len(scene.objects)} objet(s)")
    check(all(abs(s - 1) < 1e-6 for s in target.scale) and all(abs(r) < 1e-6 for r in target.rotation_euler),
          "Transformations appliquées (échelle 1, rotation 0)")
    check(len(me.vertices) <= max_verts, "Nombre de sommets sous la limite",
          f"{len(me.vertices)} / {max_verts} (les coutures UV en ajouteront au rig)")
    check(len(me.loop_triangles) > 0, "Triangles présents", f"{len(me.loop_triangles)} triangles", fatal=True)
    check(len(me.uv_layers) > 0, "Carte UV présente", "sans UV, les textures ne s'afficheront pas")
    check(len(target.material_slots) > 0, "Matériaux présents", f"{len(target.material_slots)} matériau(x)")
    check(textures > 0, "Textures détectées", f"{textures} image(s) - sinon le modèle sera gris")
    dims = sorted((width, depth, height), reverse=True)
    stretch = dims[0] / dims[1] if dims[1] else 99
    check(stretch <= 8, "Proportions plausibles pour un personnage",
          f"dimension max {dims[0]:.0f} contre {dims[1]:.0f} (rapport {stretch:.1f}, limite 8) "
          "- modèle étiré ou morceau parasite", fatal=True)
    check(height >= max(width, depth), "Modèle debout (la hauteur est la plus grande dimension)",
          f"largeur {width:.2f} / profondeur {depth:.2f} / hauteur {height:.2f}")
    ratio = max(width, depth) / height if height else 0
    if height >= max(width, depth):
        check(ratio >= 0.5, "Bras écartés (T-pose ou A-pose probable)",
              f"rapport largeur/hauteur {ratio:.2f} - sous 0,5 les bras sont sans doute le long du corps")
    else:
        check(False, "Pose non évaluée", "le modèle n'est pas debout, corrige d'abord son orientation")

    # 9. Export + vérification en réimportant le fichier produit
    bpy.ops.object.select_all(action="DESELECT")
    bpy.ops.export_scene.gltf(filepath=dst, export_format="GLB")
    verify_export(dst, textures)

    if fatal_errors:
        if os.path.isfile(dst):
            os.remove(dst)
        log("ÉCHEC : " + " ; ".join(fatal_errors))
        sys.exit(1)
    log(f"Exporté : {dst}")


main()
