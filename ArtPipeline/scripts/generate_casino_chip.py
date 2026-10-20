"""
CrapsCrapsCraps — Procedural 3D Casino Chip Generator (Blender 5.2)
Generates SM_CasinoChip_v1 at 10x scale (39cm diameter, 3.4cm thickness)
with beveled rim, recessed face well, and edge stripes.

Usage:
  "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" --background --python ArtPipeline/scripts/generate_casino_chip.py
"""

from __future__ import annotations
import math
import os
import sys
import bpy
import bmesh
from mathutils import Vector, Matrix

# ==============================================================================
# CONFIGURATION (10x SCALE: 1.0 Blender unit = 100 Unreal units / cm)
# ==============================================================================
CHIP_DIAMETER = 0.39       # 39cm diameter (0.39m)
CHIP_RADIUS = CHIP_DIAMETER * 0.5  # 0.195m
CHIP_THICKNESS = 0.034     # 3.4cm thickness (0.034m)
CHIP_HALF_THICK = CHIP_THICKNESS * 0.5
RIM_WIDTH = 0.032          # 3.2cm outer rim ring
RIM_INSET_DEPTH = 0.0035   # 3.5mm recessed center well
BEVEL_RADIUS = 0.003       # 3mm edge chamfer
SEGMENTS = 64

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
EXPORT_DIR = os.path.join(PROJECT_ROOT, "ArtPipeline", "exports")
CONTENT_DIR = os.path.join(PROJECT_ROOT, "Content", "Meshes")

os.makedirs(EXPORT_DIR, exist_ok=True)
os.makedirs(CONTENT_DIR, exist_ok=True)

# ==============================================================================
# SCENE SETUP
# ==============================================================================
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0

# Create materials
def create_material(name: str, base_color: tuple, metallic: float, roughness: float, emissive_color: tuple = (0,0,0,1), emissive_strength: float = 0.0):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = base_color
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
        if "Emission Color" in bsdf.inputs:
            bsdf.inputs["Emission Color"].default_value = emissive_color
            bsdf.inputs["Emission Strength"].default_value = emissive_strength
    return mat

mat_base = create_material("M_Chip_Base", (0.04, 0.04, 0.06, 1.0), 0.2, 0.45)
mat_inlay = create_material("M_Chip_Inlay", (0.0, 0.96, 0.83, 1.0), 0.1, 0.2, (0.0, 0.96, 0.83, 1.0), 4.0)
mat_stripe = create_material("M_Chip_Stripe", (1.0, 0.72, 0.01, 1.0), 0.4, 0.3, (1.0, 0.72, 0.01, 1.0), 2.5)

# ==============================================================================
# GEOMETRY GENERATION VIA BMESH
# ==============================================================================
mesh = bpy.data.meshes.new(name="SM_CasinoChip_v1_Mesh")
bm = bmesh.new()

# Generate cylinder with top/bottom rings for recessed face
bmesh.ops.create_cone(
    bm,
    cap_ends=True,
    cap_tris=False,
    segments=SEGMENTS,
    radius1=CHIP_RADIUS,
    radius2=CHIP_RADIUS,
    depth=CHIP_THICKNESS,
    matrix=Matrix.Translation((0, 0, 0))
)

# Identify top and bottom cap faces (Z > 0 and Z < 0)
top_faces = [f for f in bm.faces if f.normal.z > 0.7]
bot_faces = [f for f in bm.faces if f.normal.z < -0.7]
side_faces = [f for f in bm.faces if abs(f.normal.z) <= 0.7]

# Inset the top and bottom faces to create the raised outer rim
res_top = bmesh.ops.inset_region(bm, faces=top_faces, thickness=RIM_WIDTH, depth=-RIM_INSET_DEPTH)
res_bot = bmesh.ops.inset_region(bm, faces=bot_faces, thickness=RIM_WIDTH, depth=RIM_INSET_DEPTH)

# Assign material slots
# 0: Base, 1: Inlay (center well), 2: Stripe (rim notches)
for f in bm.faces:
    f.material_index = 0

# Center recessed faces get Inlay material
for f in res_top.get('faces', []):
    f.material_index = 1
for f in res_bot.get('faces', []):
    f.material_index = 1

# Stripe pattern around circumference: 6 edge stripes
for f in side_faces:
    angle = math.atan2(f.calc_center_median().y, f.calc_center_median().x)
    # 6 stripes around 2*PI: check stripe phase
    stripe_phase = (angle / (2.0 * math.pi) * 6.0) % 1.0
    if stripe_phase < 0.22:
        f.material_index = 2

# Calculate smooth normals
bm.faces.ensure_lookup_table()
for f in bm.faces:
    f.smooth = True

bm.to_mesh(mesh)
bm.free()

chip_obj = bpy.data.objects.new("SM_CasinoChip_v1", mesh)
bpy.context.collection.objects.link(chip_obj)

chip_obj.data.materials.append(mat_base)
chip_obj.data.materials.append(mat_inlay)
chip_obj.data.materials.append(mat_stripe)

# Select and export
bpy.ops.object.select_all(action='DESELECT')
chip_obj.select_set(True)
bpy.context.view_layer.objects.active = chip_obj

fbx_export_path = os.path.join(EXPORT_DIR, "SM_CasinoChip_v1.fbx")
fbx_content_path = os.path.join(CONTENT_DIR, "SM_CasinoChip_v1.fbx")

bpy.ops.export_scene.fbx(
    filepath=fbx_export_path,
    use_selection=True,
    global_scale=1.0,
    axis_forward='-Z',
    axis_up='Y',
    apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_ALL',
    mesh_smooth_type='FACE'
)

# Also copy/save directly to Content/Meshes
import shutil
shutil.copyfile(fbx_export_path, fbx_content_path)
print(f"SUCCESS: Exported SM_CasinoChip_v1 to {fbx_export_path} and {fbx_content_path}")
