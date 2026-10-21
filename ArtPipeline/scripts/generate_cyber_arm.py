"""
CrapsCrapsCraps — Procedural 3D Synth-Noir Cybernetic Left Arm Generator (Blender 5.2)
Generates SM_CyberArm_Left at 10x scale (cm) with:
- Beveled Charcoal Titanium Forearm Housing & Recessed Conduit Spine
- Chrono-Cuff Wrist Collar
- Articulated Cyber-Palm Plate
- 5 Segmented Cybernetic Digits (Thumb + 4 articulated fingers)
- 4 Faceted Knuckle Ring Bands

Usage:
  "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" --background --python ArtPipeline/scripts/generate_cyber_arm.py
"""

from __future__ import annotations
import math
import os
import sys
import bpy
import bmesh
from mathutils import Vector, Matrix

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
EXPORT_DIR = os.path.join(PROJECT_ROOT, "ArtPipeline", "exports")
CONTENT_DIR = os.path.join(PROJECT_ROOT, "Content", "Meshes")

os.makedirs(EXPORT_DIR, exist_ok=True)
os.makedirs(CONTENT_DIR, exist_ok=True)

# Reset Scene
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0

# Material Setup
def create_material(name, color, metallic, roughness, emissive=(0,0,0,1), emissive_strength=0.0):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = color
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
        if "Emission Color" in bsdf.inputs:
            bsdf.inputs["Emission Color"].default_value = emissive
            bsdf.inputs["Emission Strength"].default_value = emissive_strength
    return mat

mat_armor = create_material("M_Cyber_Armor", (0.035, 0.040, 0.045, 1.0), 0.85, 0.30)
mat_joints = create_material("M_Cyber_Joints", (0.08, 0.085, 0.095, 1.0), 0.95, 0.20)
mat_conduit = create_material("M_Cyber_Conduit", (0.0, 0.85, 1.0, 1.0), 0.1, 0.15, (0.0, 0.85, 1.0, 1.0), 5.0)
mat_gold = create_material("M_Cyber_GoldTrim", (1.0, 0.78, 0.12, 1.0), 0.90, 0.25, (1.0, 0.78, 0.12, 1.0), 1.5)

mesh = bpy.data.meshes.new(name="SM_CyberArm_Left_Mesh")
bm = bmesh.new()

# Material slot index helpers
# 0 = Armor, 1 = Joints/Hydraulics, 2 = Neon Conduit, 3 = Gold Trim

def add_box(bm, center, size, mat_idx=0):
    start_idx = len(bm.faces)
    bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Translation(center) @ Matrix.Diagonal((*size, 1.0)))
    for f in bm.faces[start_idx:]:
        f.material_index = mat_idx

def add_cylinder(bm, center, radius, depth, mat_idx=0, segments=16, rot_y=0.0, rot_z=0.0):
    start_idx = len(bm.faces)
    rot_mat = Matrix.Rotation(rot_z, 4, 'Z') @ Matrix.Rotation(rot_y, 4, 'Y')
    xform = Matrix.Translation(center) @ rot_mat
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments,
                          radius1=radius, radius2=radius, depth=depth, matrix=xform)
    for f in bm.faces[start_idx:]:
        f.material_index = mat_idx

def add_sphere(bm, center, radius, mat_idx=1, segs=12, rings=8):
    start_idx = len(bm.faces)
    bmesh.ops.create_uvsphere(bm, u_segments=segs, v_segments=rings, radius=radius,
                              matrix=Matrix.Translation(center))
    for f in bm.faces[start_idx:]:
        f.material_index = mat_idx

# -------------------------------------------------------------------------
# 1. FOREARM ARMOR SLEEVE & CONDUIT (extends backward from wrist X = -4 to X = -28)
# -------------------------------------------------------------------------
# Main forearm hull (tapered)
add_box(bm, center=(-16.0, -2.0, 3.5), size=(24.0, 10.5, 7.5), mat_idx=0)
# Top armor carapace
add_box(bm, center=(-16.0, -2.0, 7.5), size=(22.0, 9.0, 1.6), mat_idx=0)
# Recessed Neon Power Conduit running along forearm top spine
add_box(bm, center=(-16.0, -2.0, 8.4), size=(20.0, 2.2, 0.6), mat_idx=2)
# Lateral hydraulic tubing
add_cylinder(bm, center=(-16.0, 3.6, 3.5), radius=1.0, depth=22.0, mat_idx=1, rot_y=math.radians(90.0))
add_cylinder(bm, center=(-16.0, -7.6, 3.5), radius=1.0, depth=22.0, mat_idx=1, rot_y=math.radians(90.0))

# -------------------------------------------------------------------------
# 2. WRIST CHRONO CUFF & JOINT (around X = -4.0 to X = 0.0)
# -------------------------------------------------------------------------
# Wrist swivel joint
add_sphere(bm, center=(-4.0, 0.0, 3.0), radius=3.8, mat_idx=1)
# Chrono collar cuff ring
add_cylinder(bm, center=(-2.0, 0.0, 3.5), radius=5.2, depth=3.2, mat_idx=0, segments=24, rot_y=math.radians(90.0))
# Chrono cuff digital display plate (gold bezel + cyan screen)
add_box(bm, center=(-2.0, 0.0, 8.9), size=(2.8, 4.4, 0.8), mat_idx=3)
add_box(bm, center=(-2.0, 0.0, 9.4), size=(2.2, 3.6, 0.4), mat_idx=2)

# -------------------------------------------------------------------------
# 3. ARTICULATED CYBER-PALM PLATE (X = 0.0 to X = 12.0)
# -------------------------------------------------------------------------
# Palm chassis core
add_box(bm, center=(5.5, 0.0, 2.2), size=(11.0, 11.5, 4.2), mat_idx=0)
# Dorsal carbon-fiber hand plate
add_box(bm, center=(5.5, 0.0, 4.5), size=(9.5, 10.0, 1.2), mat_idx=0)
# Dorsal neon knuckle power trace
add_box(bm, center=(6.0, 0.0, 5.2), size=(7.0, 1.8, 0.4), mat_idx=2)

# -------------------------------------------------------------------------
# 4. ARTICULATED DIGITS & KNUCKLE RINGS
# -------------------------------------------------------------------------
# 4 fingers: Index (Y=4.2), Middle (Y=1.4), Ring (Y=-1.4), Pinky (Y=-4.2)
finger_y_coords = [4.2, 1.4, -1.4, -4.2]
finger_lengths = [11.0, 12.2, 11.2, 9.2]

for i, fy in enumerate(finger_y_coords):
    flen = finger_lengths[i]
    knuckle_x = 11.5
    knuckle_z = 2.8

    # Knuckle ball joint
    add_sphere(bm, center=(knuckle_x, fy, knuckle_z), radius=1.6, mat_idx=1)

    # Knuckle Ring Band (Seats on proximal joint)
    # Ring 0 = Cyan/Neon Signet, Ring 1 = Cobalt, Ring 2 = Violet, Ring 3 = Gold
    add_cylinder(bm, center=(knuckle_x + 1.2, fy, knuckle_z + 0.1),
                 radius=2.1, depth=1.6, mat_idx=3 if i == 3 else (2 if i == 0 else 1),
                 segments=18, rot_y=math.radians(90.0))

    # Proximal phalanx (sloping slightly forward-down)
    prox_len = flen * 0.55
    add_cylinder(bm, center=(knuckle_x + (prox_len * 0.5), fy, knuckle_z - 0.4),
                 radius=1.3, depth=prox_len, mat_idx=0, segments=12, rot_y=math.radians(82.0))

    # Intermediate knuckle joint
    mid_x = knuckle_x + prox_len
    mid_z = knuckle_z - 1.2
    add_sphere(bm, center=(mid_x, fy, mid_z), radius=1.3, mat_idx=1)

    # Distal phalanx (curling down to grip the table rail)
    dist_len = flen * 0.45
    add_cylinder(bm, center=(mid_x + (dist_len * 0.45), fy, mid_z - (dist_len * 0.35)),
                 radius=1.1, depth=dist_len, mat_idx=0, segments=12, rot_y=math.radians(55.0))

    # Tactile rubber fingertip pad
    add_sphere(bm, center=(mid_x + dist_len * 0.8, fy, mid_z - dist_len * 0.65), radius=1.0, mat_idx=1)

# Thumb (anchored laterally at X=3.5, Y=6.2)
add_sphere(bm, center=(3.5, 6.2, 1.8), radius=1.8, mat_idx=1)
add_cylinder(bm, center=(6.5, 7.8, 1.0), radius=1.4, depth=6.0, mat_idx=0, segments=12,
             rot_y=math.radians(70.0), rot_z=math.radians(28.0))
add_sphere(bm, center=(9.2, 9.2, 0.2), radius=1.3, mat_idx=1)
add_cylinder(bm, center=(11.5, 9.8, -0.6), radius=1.1, depth=4.5, mat_idx=0, segments=12,
             rot_y=math.radians(45.0), rot_z=math.radians(15.0))

# -------------------------------------------------------------------------
# FINALIZE BMESH & EXPORT FBX
# -------------------------------------------------------------------------
bm.to_mesh(mesh)
bm.free()

obj = bpy.data.objects.new("SM_CyberArm_Left", mesh)
scene.collection.objects.link(obj)

# Assign materials to object
for m in [mat_armor, mat_joints, mat_conduit, mat_gold]:
    obj.data.materials.append(m)

# Set smooth shading
for poly in obj.data.polygons:
    poly.use_smooth = True

bpy.context.view_layer.objects.active = obj
obj.select_set(True)

export_fbx_path = os.path.join(CONTENT_DIR, "SM_CyberArm_Left.fbx")
export_pipeline_path = os.path.join(EXPORT_DIR, "SM_CyberArm_Left.fbx")

bpy.ops.export_scene.fbx(
    filepath=export_fbx_path,
    use_selection=True,
    global_scale=1.0,
    apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_NONE',
    axis_forward='-Z',
    axis_up='Y'
)

bpy.ops.export_scene.fbx(
    filepath=export_pipeline_path,
    use_selection=True,
    global_scale=1.0,
    apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_NONE',
    axis_forward='-Z',
    axis_up='Y'
)

print(f"[CYBER_ARM] Successfully exported: {export_fbx_path}")
print(f"[CYBER_ARM] Vertices: {len(mesh.vertices)}, Polygons: {len(mesh.polygons)}")
