"""Headless Blender step: bone-heat skinning weights for a welded mesh.

Usage: blender -b --factory-startup --python blender_heat_weights.py -- in.npz out.npz
in.npz  : verts (N,3), faces (M,3), bone_names (B,), heads (B,3), tails (B,3), parents (B,)
out.npz : weights (N,B) float32, rows follow the input vertex order

The mesh is built with from_pydata (no file import), so there is no axis conversion and the vertex order is kept.
"""
import sys

import bpy
import numpy as np

argv = sys.argv[sys.argv.index("--") + 1:]
src, dst = argv[0], argv[1]
data = np.load(src, allow_pickle=True)
verts = data["verts"].astype(float)
faces = data["faces"].astype(int)
names = [str(n) for n in data["bone_names"]]
heads = data["heads"].astype(float)
tails = data["tails"].astype(float)
parents = [str(p) for p in data["parents"]]

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene

mesh = bpy.data.meshes.new("character")
mesh.from_pydata(verts.tolist(), [], faces.tolist())
mesh.update()
mesh_obj = bpy.data.objects.new("character", mesh)
scene.collection.objects.link(mesh_obj)

arm = bpy.data.armatures.new("rig")
arm_obj = bpy.data.objects.new("rig", arm)
scene.collection.objects.link(arm_obj)
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode="EDIT")
for name, head, tail in zip(names, heads, tails):
    bone = arm.edit_bones.new(name)
    bone.head = head.tolist()
    bone.tail = tail.tolist()
for name, parent in zip(names, parents):
    if parent:
        arm.edit_bones[name].parent = arm.edit_bones[parent]
        arm.edit_bones[name].use_connect = False
bpy.ops.object.mode_set(mode="OBJECT")

bpy.ops.object.select_all(action="DESELECT")
mesh_obj.select_set(True)
arm_obj.select_set(True)
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.parent_set(type="ARMATURE_AUTO")

weights = np.zeros((len(verts), len(names)), dtype=np.float32)
group_to_bone = {vg.index: names.index(vg.name) for vg in mesh_obj.vertex_groups if vg.name in names}
for v in mesh.vertices:
    for g in v.groups:
        col = group_to_bone.get(g.group)
        if col is not None:
            weights[v.index, col] = g.weight

np.savez(dst, weights=weights)
print(f"[heat] {len(verts)} verts, {len(names)} bones, unweighted={(weights.sum(1) <= 1e-6).sum()}")
