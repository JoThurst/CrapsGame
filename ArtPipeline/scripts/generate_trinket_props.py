"""
CrapsCrapsCraps — Procedural 3D Trinket Props Generator (Blender 5.2)
Generates:
1. SM_RailProp_WhiskeyTumbler: Faceted lowball crystal glass with glowing amber bourbon and ice cube.
2. SM_TrinketRing_Signet: Luxury high-roller signet ring with gold band and faceted cut gemstone bezel.

Scale: 10x Casino Scale (cm)
Usage:
  "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" --background --python ArtPipeline/scripts/generate_trinket_props.py
"""

from __future__ import annotations
import math
import os
import bpy
import bmesh
from mathutils import Vector, Matrix

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
EXPORT_DIR = os.path.join(PROJECT_ROOT, "ArtPipeline", "exports")
CONTENT_DIR = os.path.join(PROJECT_ROOT, "Content", "Meshes")

os.makedirs(EXPORT_DIR, exist_ok=True)
os.makedirs(CONTENT_DIR, exist_ok=True)

def create_material(name, color, metallic=0.0, roughness=0.5, emissive=(0,0,0,1), emissive_strength=0.0, alpha=1.0):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = color
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
        if "Transmission Weight" in bsdf.inputs:
            bsdf.inputs["Transmission Weight"].default_value = 1.0 - alpha
        elif "Transmission" in bsdf.inputs:
            bsdf.inputs["Transmission"].default_value = 1.0 - alpha
        if "Emission Color" in bsdf.inputs:
            bsdf.inputs["Emission Color"].default_value = emissive
            bsdf.inputs["Emission Strength"].default_value = emissive_strength
    return mat

# =========================================================================
# ASSET 1: SM_RailProp_WhiskeyTumbler
# =========================================================================
def generate_whiskey_tumbler():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1.0

    mat_glass = create_material("M_Glass_Crystal", (0.9, 0.92, 0.95, 0.6), 0.1, 0.05, alpha=0.3)
    mat_bourbon = create_material("M_Bourbon_Liquid", (0.95, 0.52, 0.05, 0.85), 0.05, 0.1, (1.0, 0.60, 0.08, 1.0), 3.0, alpha=0.7)
    mat_ice = create_material("M_Ice_Cube", (0.85, 0.95, 1.0, 0.8), 0.05, 0.08, (0.4, 0.8, 1.0, 1.0), 1.0, alpha=0.5)

    mesh = bpy.data.meshes.new(name="SM_RailProp_WhiskeyTumbler_Mesh")
    bm = bmesh.new()

    # 1. Heavy Crystal Base (octagonal faceted base)
    base_h = 1.8
    start = len(bm.faces)
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=8,
                          radius1=4.2, radius2=4.2, depth=base_h,
                          matrix=Matrix.Translation((0, 0, base_h * 0.5)))
    for f in bm.faces[start:]:
        f.material_index = 0

    # 2. Fluted Glass Wall
    wall_h = 7.2
    start = len(bm.faces)
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=16,
                          radius1=4.0, radius2=4.4, depth=wall_h,
                          matrix=Matrix.Translation((0, 0, base_h + wall_h * 0.5)))
    for f in bm.faces[start:]:
        f.material_index = 0

    # 3. Bourbon Liquid Core
    liq_h = 4.8
    start = len(bm.faces)
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=16,
                          radius1=3.5, radius2=3.8, depth=liq_h,
                          matrix=Matrix.Translation((0, 0, base_h + liq_h * 0.5)))
    for f in bm.faces[start:]:
        f.material_index = 1

    # 4. Floating Ice Cube (beveled cube rotated at an angle)
    ice_rot = Matrix.Rotation(math.radians(22.0), 4, 'Z') @ Matrix.Rotation(math.radians(14.0), 4, 'X')
    ice_xform = Matrix.Translation((0.5, -0.4, base_h + liq_h - 0.8)) @ ice_rot @ Matrix.Diagonal((2.2, 2.2, 2.2, 1.0))
    start = len(bm.faces)
    bmesh.ops.create_cube(bm, size=1.0, matrix=ice_xform)
    for f in bm.faces[start:]:
        f.material_index = 2

    bm.to_mesh(mesh)
    bm.free()

    obj = bpy.data.objects.new("SM_RailProp_WhiskeyTumbler", mesh)
    scene.collection.objects.link(obj)
    obj.data.materials.append(mat_glass)
    obj.data.materials.append(mat_bourbon)
    obj.data.materials.append(mat_ice)

    for p in obj.data.polygons:
        p.use_smooth = True

    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)

    export_path_content = os.path.join(CONTENT_DIR, "SM_RailProp_WhiskeyTumbler.fbx")
    export_path_art = os.path.join(EXPORT_DIR, "SM_RailProp_WhiskeyTumbler.fbx")

    for exp_path in [export_path_content, export_path_art]:
        bpy.ops.export_scene.fbx(
            filepath=exp_path,
            use_selection=True,
            global_scale=0.01,
            apply_unit_scale=True,
            apply_scale_options='FBX_SCALE_NONE',
            axis_forward='-Z',
            axis_up='Y'
        )
    print(f"[WHISKEY_TUMBLER] Exported: {export_path_content}")

# =========================================================================
# ASSET 2: SM_TrinketRing_Signet
# =========================================================================
def generate_signet_ring():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1.0

    mat_band = create_material("M_Ring_GoldBand", (1.0, 0.80, 0.15, 1.0), 0.92, 0.22)
    mat_gem = create_material("M_Ring_Gemstone", (0.0, 0.90, 1.0, 1.0), 0.1, 0.15, (0.0, 0.90, 1.0, 1.0), 4.5)

    mesh = bpy.data.meshes.new(name="SM_TrinketRing_Signet_Mesh")
    bm = bmesh.new()

    # 1. Ring Band (Torus-like hollow ring)
    # Modeled aligned along X-axis so fingers slip along X:
    # Outer radius = 1.85 cm, Inner radius = 1.38 cm, Width along X = 1.2 cm
    band_width = 1.2
    outer_r = 1.85
    start = len(bm.faces)
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=24,
                          radius1=outer_r, radius2=outer_r, depth=band_width,
                          matrix=Matrix.Rotation(math.radians(90.0), 4, 'Y'))
    for f in bm.faces[start:]:
        f.material_index = 0

    # 2. Raised Signet Bezel Plate (+Z)
    bezel_xf = Matrix.Translation((0.0, 0.0, outer_r + 0.15)) @ Matrix.Diagonal((1.3, 1.4, 0.4, 1.0))
    start = len(bm.faces)
    bmesh.ops.create_cube(bm, size=1.0, matrix=bezel_xf)
    for f in bm.faces[start:]:
        f.material_index = 0

    # 3. Faceted Cut Gemstone Face
    gem_xf = Matrix.Translation((0.0, 0.0, outer_r + 0.45)) @ Matrix.Diagonal((1.0, 1.1, 0.3, 1.0))
    start = len(bm.faces)
    bmesh.ops.create_cube(bm, size=1.0, matrix=gem_xf)
    for f in bm.faces[start:]:
        f.material_index = 1

    bm.to_mesh(mesh)
    bm.free()

    obj = bpy.data.objects.new("SM_TrinketRing_Signet", mesh)
    scene.collection.objects.link(obj)
    obj.data.materials.append(mat_band)
    obj.data.materials.append(mat_gem)

    for p in obj.data.polygons:
        p.use_smooth = True

    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)

    export_path_content = os.path.join(CONTENT_DIR, "SM_TrinketRing_Signet.fbx")
    export_path_art = os.path.join(EXPORT_DIR, "SM_TrinketRing_Signet.fbx")

    for exp_path in [export_path_content, export_path_art]:
        bpy.ops.export_scene.fbx(
            filepath=exp_path,
            use_selection=True,
            global_scale=0.01,
            apply_unit_scale=True,
            apply_scale_options='FBX_SCALE_NONE',
            axis_forward='-Z',
            axis_up='Y'
        )
    print(f"[SIGNET_RING] Exported: {export_path_content}")

if __name__ == "__main__":
    generate_whiskey_tumbler()
    generate_signet_ring()
    print("ALL TRINKET PROPS GENERATED SUCCESSFULLY!")
