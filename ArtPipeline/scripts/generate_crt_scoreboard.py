"""
CrapsCrapsCraps — Reusable Blender-to-UE5.3 Procedural Asset Generation Pipeline
First Asset: CRT Synth-Noir Casino Scoreboard Enclosure (v1 Simple Testable Milestone)

Usage:
  1. Headless CLI:
     "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" --background --python ArtPipeline/scripts/generate_crt_scoreboard.py
  2. Inside running Blender (or via Blender MCP execute_code):
     import runpy
     runpy.run_path(r"C:/Code/Unreal Projects/Craps/CrapsCrapsCraps/ArtPipeline/scripts/generate_crt_scoreboard.py")
"""

from __future__ import annotations

import json
import math
import os
import sys
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

import bmesh
import bpy
from mathutils import Matrix, Vector


# ==============================================================================
# 1. PIPELINE CONFIGURATION & PALETTE (CRT SYNTH-NOIR)
# ==============================================================================

@dataclass
class ScoreboardConfig:
    """Parameterized dimensions & export configuration for the CRT Scoreboard."""
    asset_name: str = "SM_CRT_Scoreboard_v1"
    blend_filename: str = "CRT_Scoreboard_v1.blend"
    fbx_filename: str = "SM_CRT_Scoreboard_v1.fbx"
    manifest_filename: str = "SM_CRT_Scoreboard_v1_manifest.json"

    # Dimensions in meters (1.0 Blender meter = 100 Unreal cm).
    # Sized for the 10x scale CrapsCrapsCraps table (table width = 8.80m / 880cm).
    total_width: float = 3.60     # Y axis span (-1.80m .. +1.80m = 360cm)
    total_height: float = 1.40    # Z axis span (0.00m .. +1.40m = 140cm)
    total_depth: float = 0.48     # X axis span (-0.28m .. +0.20m = 48cm)

    # Frame & recess parameters (deep 11.5cm shadow wells for recessed CRT tubes)
    bay_recess_depth: float = 0.115
    bezel_thickness: float = 0.032
    bezel_depth: float = 0.068
    bankroll_bezel_thickness: float = 0.046
    bankroll_bezel_depth: float = 0.088
    crt_bulge: float = 0.030      # Pronounced +X barrel curvature on CRT glass tubes
    crt_glass_depth: float = 0.018
    screen_grid_x: int = 10       # Subdivisions along screen width for smooth CRT curvature
    screen_grid_y: int = 6        # Subdivisions along screen height

    # Bevel settings for readable industrial edges
    hull_bevel_width: float = 0.008
    hull_bevel_segments: int = 2
    trim_bevel_width: float = 0.005
    trim_bevel_segments: int = 2


@dataclass
class MaterialSpec:
    name: str
    base_color: Tuple[float, float, float, float]
    metallic: float = 0.0
    roughness: float = 0.5
    emission_color: Tuple[float, float, float, float] = (0.0, 0.0, 0.0, 1.0)
    emission_strength: float = 0.0
    roughness_variation: float = 0.08
    noise_scale: float = 18.0
    is_crt_screen: bool = False
    coat_weight: float = 0.0


# Visual Bible: CRT Synth-Noir Palette (DESIGN.md) — Refined for Dark Brushed Metal & Smoked CRT Glass
PALETTE: Dict[str, MaterialSpec] = {
    "Hull_DarkSteel": MaterialSpec(
        name="MI_Hull_DarkSteel",
        base_color=(0.020, 0.023, 0.029, 1.0),   # Heavy dark gunmetal / oiled industrial steel
        metallic=0.90,
        roughness=0.46,
        roughness_variation=0.14,
        noise_scale=24.0,
    ),
    "Trim_TarnishedBrass": MaterialSpec(
        name="MI_Trim_TarnishedBrass",
        base_color=(0.560, 0.395, 0.135, 1.0),   # Aged tarnished casino brass
        metallic=0.93,
        roughness=0.33,
        roughness_variation=0.12,
        noise_scale=32.0,
    ),
    "Accent_Oxblood": MaterialSpec(
        name="MI_Accent_Oxblood",
        base_color=(0.135, 0.014, 0.022, 1.0),   # Deep burgundy / oxblood baked industrial enamel
        metallic=0.22,
        roughness=0.34,
        roughness_variation=0.09,
        noise_scale=20.0,
        coat_weight=0.35,
    ),
    "Cavity_MatteBlack": MaterialSpec(
        name="MI_Cavity_MatteBlack",
        base_color=(0.007, 0.008, 0.011, 1.0),   # Deep shadow recess & rubber CRT mask liner
        metallic=0.15,
        roughness=0.88,
        roughness_variation=0.06,
        noise_scale=40.0,
    ),
    "Screen_Bankroll": MaterialSpec(
        name="MI_Screen_Bankroll_Cyan",
        base_color=(0.002, 0.015, 0.018, 1.0),   # Near-black smoked teal CRT phosphor glass
        metallic=0.04,
        roughness=0.09,
        emission_color=(0.0, 0.850, 0.740, 1.0),  # Restrained Phosphor Cyan standby glow
        emission_strength=0.055,
        is_crt_screen=True,
        coat_weight=0.12,
    ),
    "Screen_Ante": MaterialSpec(
        name="MI_Screen_Ante_Amber",
        base_color=(0.010, 0.006, 0.002, 1.0),   # Near-black smoked amber CRT glass
        metallic=0.04,
        roughness=0.09,
        emission_color=(1.0, 0.620, 0.020, 1.0),  # Restrained Amber standby glow
        emission_strength=0.045,
        is_crt_screen=True,
        coat_weight=0.12,
    ),
    "Screen_UnbankedPool": MaterialSpec(
        name="MI_Screen_UnbankedPool_Cyan",
        base_color=(0.002, 0.012, 0.015, 1.0),   # Near-black deep smoked teal CRT glass
        metallic=0.04,
        roughness=0.10,
        emission_color=(0.0, 0.700, 0.840, 1.0),  # Restrained Deep Cyan standby glow
        emission_strength=0.042,
        is_crt_screen=True,
        coat_weight=0.12,
    ),
    "Screen_Multiplier": MaterialSpec(
        name="MI_Screen_Multiplier_Amber",
        base_color=(0.012, 0.004, 0.006, 1.0),   # Near-black smoked crimson-amber CRT glass
        metallic=0.04,
        roughness=0.10,
        emission_color=(1.0, 0.360, 0.040, 1.0),  # Restrained Warm Amber standby glow
        emission_strength=0.048,
        is_crt_screen=True,
        coat_weight=0.12,
    ),
    "Emissive_Cyan": MaterialSpec(
        name="MI_Emissive_PhosphorCyan",
        base_color=(0.02, 0.45, 0.40, 1.0),
        metallic=0.0,
        roughness=0.12,
        emission_color=(0.0, 0.961, 0.831, 1.0),
        emission_strength=2.2,
    ),
    "Emissive_Amber": MaterialSpec(
        name="MI_Emissive_HighVoltageAmber",
        base_color=(0.50, 0.32, 0.02, 1.0),
        metallic=0.0,
        roughness=0.12,
        emission_color=(1.0, 0.718, 0.012, 1.0),
        emission_strength=2.2,
    ),
}


@dataclass
class CompartmentLayout:
    """Defines a modular inset CRT display compartment on the front face (+X)."""
    key: str
    screen_object_name: str
    bezel_object_name: str
    material_key: str
    indicator_mat_key: str
    y_min: float
    y_max: float
    z_min: float
    z_max: float
    is_hero: bool = False

    @property
    def center_y(self) -> float:
        return 0.5 * (self.y_min + self.y_max)

    @property
    def center_z(self) -> float:
        return 0.5 * (self.z_min + self.z_max)

    @property
    def width(self) -> float:
        return self.y_max - self.y_min

    @property
    def height(self) -> float:
        return self.z_max - self.z_min


# ==============================================================================
# 2. REUSABLE BLENDER -> UNREAL ENGINE 5.3 PIPELINE CORE
# ==============================================================================

class UE5AssetPipeline:
    """
    Reusable procedural modeling and export harness for Unreal Engine 5.3.
    Enforces:
      - Metric units (1.0m = 100 Unreal cm, Z-Up, +X Forward)
      - Named collection hierarchy
      - Principled BSDF materials with UE5 `MI_` naming convention
      - Clean quad/tri topology (no >4-vert n-gons), bevels, and non-overlapping UVs
      - FBX export settings tailored for UE5 StaticMesh / Blueprint assembly
    """

    def __init__(self, project_root: str, root_collection_name: str):
        self.project_root = os.path.abspath(project_root)
        self.blend_dir = os.path.join(self.project_root, "ArtPipeline", "blend")
        self.export_dir = os.path.join(self.project_root, "ArtPipeline", "exports")
        os.makedirs(self.blend_dir, exist_ok=True)
        os.makedirs(self.export_dir, exist_ok=True)

        self.root_collection_name = root_collection_name
        self.collections: Dict[str, bpy.types.Collection] = {}
        self.materials: Dict[str, bpy.types.Material] = {}

    def reset_scene(self) -> None:
        """Clears all existing scene objects, meshes, materials, and collections for a deterministic build."""
        if bpy.ops.object.mode_set.poll():
            bpy.ops.object.mode_set(mode="OBJECT")

        for obj in list(bpy.data.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
        for mesh in list(bpy.data.meshes):
            bpy.data.meshes.remove(mesh, do_unlink=True)
        for mat in list(bpy.data.materials):
            bpy.data.materials.remove(mat, do_unlink=True)
        for col in list(bpy.data.collections):
            bpy.data.collections.remove(col, do_unlink=True)
        for cam in list(bpy.data.cameras):
            bpy.data.cameras.remove(cam, do_unlink=True)
        for lgt in list(bpy.data.lights):
            bpy.data.lights.remove(lgt, do_unlink=True)

        scene = bpy.context.scene
        scene.unit_settings.system = "METRIC"
        scene.unit_settings.scale_length = 1.0
        scene.unit_settings.length_unit = "METERS"

        root_col = bpy.data.collections.new(self.root_collection_name)
        scene.collection.children.link(root_col)
        self.collections["ROOT"] = root_col

    def create_subcollection(self, key: str, name: str) -> bpy.types.Collection:
        col = bpy.data.collections.new(name)
        self.collections["ROOT"].children.link(col)
        self.collections[key] = col
        return col

    def build_materials(self, specs: Dict[str, MaterialSpec]) -> None:
        """Creates Principled BSDF materials with subtle procedural roughness variation & CRT scanlines."""
        for key, spec in specs.items():
            mat = bpy.data.materials.new(name=spec.name)
            mat.use_nodes = True
            mat.diffuse_color = spec.base_color
            mat.metallic = spec.metallic
            mat.roughness = spec.roughness

            nodes = mat.node_tree.nodes
            links = mat.node_tree.links
            bsdf = nodes.get("Principled BSDF")
            if bsdf is None:
                bsdf = nodes.new(type="ShaderNodeBsdfPrincipled")

            bsdf.inputs["Base Color"].default_value = spec.base_color
            bsdf.inputs["Metallic"].default_value = spec.metallic
            bsdf.inputs["Roughness"].default_value = spec.roughness

            if "Coat Weight" in bsdf.inputs:
                bsdf.inputs["Coat Weight"].default_value = spec.coat_weight
            if "Coat Roughness" in bsdf.inputs:
                bsdf.inputs["Coat Roughness"].default_value = 0.08

            if "Emission Color" in bsdf.inputs:
                bsdf.inputs["Emission Color"].default_value = spec.emission_color
            elif "Emission" in bsdf.inputs:
                bsdf.inputs["Emission"].default_value = spec.emission_color

            if "Emission Strength" in bsdf.inputs:
                bsdf.inputs["Emission Strength"].default_value = spec.emission_strength

            # Add subtle procedural brushed-metal / patina roughness variation for physical surfaces
            if not spec.is_crt_screen and spec.roughness_variation > 0.0:
                tex_coord = nodes.new(type="ShaderNodeTexCoord")
                mapping = nodes.new(type="ShaderNodeMapping")
                # Stretch along Y for horizontal brushed grain on brass & steel
                mapping.inputs["Scale"].default_value = (1.0, 4.5, 1.5)

                noise = nodes.new(type="ShaderNodeTexNoise")
                noise.inputs["Scale"].default_value = spec.noise_scale
                noise.inputs["Detail"].default_value = 6.0
                noise.inputs["Roughness"].default_value = 0.65

                ramp = nodes.new(type="ShaderNodeValToRGB")
                r_low = max(0.04, spec.roughness - spec.roughness_variation * 0.5)
                r_high = min(0.96, spec.roughness + spec.roughness_variation * 0.5)
                ramp.color_ramp.elements[0].position = 0.25
                ramp.color_ramp.elements[0].color = (r_low, r_low, r_low, 1.0)
                ramp.color_ramp.elements[1].position = 0.75
                ramp.color_ramp.elements[1].color = (r_high, r_high, r_high, 1.0)

                bump = nodes.new(type="ShaderNodeBump")
                bump.inputs["Strength"].default_value = 0.025
                bump.inputs["Distance"].default_value = 0.002

                links.new(tex_coord.outputs["Object"], mapping.inputs["Vector"])
                links.new(mapping.outputs["Vector"], noise.inputs["Vector"])
                links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
                links.new(ramp.outputs["Color"], bsdf.inputs["Roughness"])
                links.new(noise.outputs["Fac"], bump.inputs["Height"])
                links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

            # Add subtle CRT scanline & phosphor glass depth for screen materials
            elif spec.is_crt_screen:
                tex_coord = nodes.new(type="ShaderNodeTexCoord")
                wave = nodes.new(type="ShaderNodeTexWave")
                wave.wave_type = "BANDS"
                wave.bands_direction = "Y"
                wave.inputs["Scale"].default_value = 48.0
                wave.inputs["Distortion"].default_value = 0.0

                math_mul = nodes.new(type="ShaderNodeMath")
                math_mul.operation = "MULTIPLY"
                math_mul.inputs[1].default_value = spec.emission_strength

                links.new(tex_coord.outputs["UV"], wave.inputs["Vector"])
                links.new(wave.outputs["Fac"], math_mul.inputs[0])
                if "Emission Strength" in bsdf.inputs:
                    links.new(math_mul.outputs["Value"], bsdf.inputs["Emission Strength"])

            self.materials[key] = mat

    def link_object_to_collection(self, obj: bpy.types.Object, collection_key: str) -> None:
        target_col = self.collections[collection_key]
        for col in list(obj.users_collection):
            col.objects.unlink(obj)
        target_col.objects.link(obj)

    def create_mesh_object(
        self,
        name: str,
        bm: bmesh.types.BMesh,
        collection_key: str,
        material_key: Optional[str] = None,
        origin: Tuple[float, float, float] = (0.0, 0.0, 0.0),
        bevel_width: float = 0.0,
        bevel_segments: int = 2,
        auto_uv: bool = True,
    ) -> bpy.types.Object:
        """Converts a BMesh into a scene Object with optional bevel, n-gon triangulation, and UVs."""
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

        if bevel_width > 0.0 and len(bm.edges) > 0:
            bmesh.ops.bevel(
                bm,
                geom=list(bm.edges),
                offset=bevel_width,
                offset_type="OFFSET",
                segments=1,
                profile=0.5,
                affect="EDGES",
                clamp_overlap=False,
            )

        # Triangulate any n-gons (>4 vertices) so FBX tangent-space export never warns
        ngons = [f for f in bm.faces if len(f.verts) > 4]
        if ngons:
            bmesh.ops.triangulate(bm, faces=ngons)

        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

        mesh = bpy.data.meshes.new(name)
        bm.to_mesh(mesh)
        bm.free()

        for poly in mesh.polygons:
            poly.use_smooth = False

        obj = bpy.data.objects.new(name, mesh)
        obj.location = Vector(origin)
        self.link_object_to_collection(obj, collection_key)

        if material_key and material_key in self.materials:
            mesh.materials.append(self.materials[material_key])

        if auto_uv:
            self.generate_box_uvs(mesh)

        return obj

    @staticmethod
    def generate_box_uvs(mesh: bpy.types.Mesh, scale: float = 1.0) -> None:
        """Deterministic triplanar box UV projection in BMesh (no context/mode switching needed)."""
        bm = bmesh.new()
        bm.from_mesh(mesh)
        uv_layer = bm.loops.layers.uv.verify()

        for face in bm.faces:
            n = face.normal
            ax, ay, az = abs(n.x), abs(n.y), abs(n.z)
            for loop in face.loops:
                co = loop.vert.co * scale
                if ax >= ay and ax >= az:
                    u, v = co.y, co.z
                elif ay >= ax and ay >= az:
                    u, v = co.x, co.z
                else:
                    u, v = co.x, co.y
                loop[uv_layer].uv = (u, v)

        bm.to_mesh(mesh)
        bm.free()

    def export_blend(self, filename: str) -> str:
        blend_path = os.path.join(self.blend_dir, filename)
        bpy.ops.wm.save_as_mainfile(filepath=blend_path, compress=True)
        return blend_path

    def export_fbx(self, filename: str) -> str:
        fbx_path = os.path.join(self.export_dir, filename)
        bpy.ops.object.select_all(action="DESELECT")
        for obj in bpy.data.objects:
            if obj.type == "MESH":
                obj.select_set(True)

        bpy.ops.export_scene.fbx(
            filepath=fbx_path,
            use_selection=True,
            global_scale=1.0,
            apply_unit_scale=True,
            apply_scale_options="FBX_SCALE_NONE",
            axis_forward="X",
            axis_up="Z",
            object_types={"MESH"},
            use_mesh_modifiers=True,
            mesh_smooth_type="FACE",
            use_tspace=True,
            add_leaf_bones=False,
            path_mode="AUTO",
        )
        return fbx_path


# ==============================================================================
# 3. GEOMETRY BUILDERS FOR CRT SCOREBOARD ENCLOSURE (v1)
# ==============================================================================

def get_compartment_layouts(cfg: ScoreboardConfig) -> List[CompartmentLayout]:
    """
    Returns the 4 inset CRT display compartments arranged on the scoreboard face:
      - Top Left (HERO): BANKROLL (Smoked Teal/Cyan — visually emphasized primary bay)
      - Top Right:       ANTE QUOTA (Smoked Amber)
      - Bottom Left:     UNBANKED POOL (Smoked Cyan)
      - Bottom Right:    MULTIPLIER (Smoked Crimson-Amber Overdrive)
    """
    return [
        CompartmentLayout(
            key="Bankroll",
            screen_object_name="SM_Scoreboard_Screen_Bankroll",
            bezel_object_name="SM_Scoreboard_Bezel_Bankroll",
            material_key="Screen_Bankroll",
            indicator_mat_key="Emissive_Cyan",
            y_min=-1.52,
            y_max=-0.10,
            z_min=0.76,
            z_max=1.22,
            is_hero=True,
        ),
        CompartmentLayout(
            key="Ante",
            screen_object_name="SM_Scoreboard_Screen_Ante",
            bezel_object_name="SM_Scoreboard_Bezel_Ante",
            material_key="Screen_Ante",
            indicator_mat_key="Emissive_Amber",
            y_min=0.14,
            y_max=1.52,
            z_min=0.77,
            z_max=1.20,
            is_hero=False,
        ),
        CompartmentLayout(
            key="UnbankedPool",
            screen_object_name="SM_Scoreboard_Screen_UnbankedPool",
            bezel_object_name="SM_Scoreboard_Bezel_UnbankedPool",
            material_key="Screen_UnbankedPool",
            indicator_mat_key="Emissive_Cyan",
            y_min=-1.52,
            y_max=-0.40,
            z_min=0.19,
            z_max=0.61,
            is_hero=False,
        ),
        CompartmentLayout(
            key="Multiplier",
            screen_object_name="SM_Scoreboard_Screen_Multiplier",
            bezel_object_name="SM_Scoreboard_Bezel_Multiplier",
            material_key="Screen_Multiplier",
            indicator_mat_key="Emissive_Amber",
            y_min=-0.16,
            y_max=1.52,
            z_min=0.19,
            z_max=0.61,
            is_hero=False,
        ),
    ]


def append_box_to_bmesh(
    bm: bmesh.types.BMesh,
    x_min: float,
    x_max: float,
    y_min: float,
    y_max: float,
    z_min: float,
    z_max: float,
) -> None:
    """Appends an axis-aligned box spanning [x_min..x_max, y_min..y_max, z_min..z_max] into a BMesh."""
    cx = 0.5 * (x_min + x_max)
    cy = 0.5 * (y_min + y_max)
    cz = 0.5 * (z_min + z_max)
    sx = x_max - x_min
    sy = y_max - y_min
    sz = z_max - z_min

    mat = (
        Matrix.Translation((cx, cy, cz))
        @ Matrix.Diagonal((sx, sy, sz, 1.0))
    )
    bmesh.ops.create_cube(bm, size=1.0, matrix=mat)


def build_frame_strip_bmesh(
    bm: bmesh.types.BMesh,
    x_min: float,
    x_max: float,
    y_min: float,
    y_max: float,
    z_min: float,
    z_max: float,
    border: float,
) -> None:
    """Adds a 4-bar picture-frame bezel into `bm` around [y_min..y_max, z_min..z_max]."""
    append_box_to_bmesh(bm, x_min, x_max, y_min - border, y_max + border, z_max, z_max + border)
    append_box_to_bmesh(bm, x_min, x_max, y_min - border, y_max + border, z_min - border, z_min)
    append_box_to_bmesh(bm, x_min, x_max, y_min - border, y_min, z_min, z_max)
    append_box_to_bmesh(bm, x_min, x_max, y_max, y_max + border, z_min, z_max)


def build_enclosure_hull(pipeline: UE5AssetPipeline, cfg: ScoreboardConfig, layouts: List[CompartmentLayout]) -> List[bpy.types.Object]:
    """
    Builds the heavy 1980s pinball/analog instrumentation enclosure (`01_Enclosure_Hull`):
      - `SM_Scoreboard_Hull_Main`: Dark brushed steel cabinet with segmented armor panels, visible
        panel seam channels, stepped rear high-voltage transformer spine, and lower control deck.
      - `SM_Scoreboard_Cowl_Oxblood`: Burgundy enamel pinball backbox shroud with deep overhanging
        glare visor, segmented side cheeks, and heavy corner bumper bolsters.
      - `SM_Scoreboard_Hull_BayLiner`: Deep recessed matte-black CRT shadow wells, inner monitor
        masks/shrouds framing each curved CRT glass tube, and recessed switch bay pockets.
    Origin of all enclosure meshes: bottom-center (0, 0, 0).
    """
    created: List[bpy.types.Object] = []

    half_w = cfg.total_width * 0.5
    h = cfg.total_height
    x_back = -0.28
    x_front = 0.20
    x_recess = x_front - cfg.bay_recess_depth
    b_pad = cfg.bezel_thickness + 0.006
    hero_pad = cfg.bankroll_bezel_thickness + 0.008
    seam = 0.012  # Physical panel seam gap between bolted steel plates

    # 1. Main Dark Brushed Steel Chassis with Segmented Panel Seams
    bm_hull = bmesh.new()
    # Solid recessed structural core behind the display wells (x_back .. x_recess)
    append_box_to_bmesh(bm_hull, x_back, x_recess, -half_w + 0.04, half_w - 0.04, 0.04, h - 0.04)

    # Stepped rear industrial transformer housings (split into Left & Right high-voltage packs with center seam)
    append_box_to_bmesh(bm_hull, x_back - 0.06, x_back, -half_w + 0.22, -0.08, 0.14, h - 0.14)
    append_box_to_bmesh(bm_hull, x_back - 0.06, x_back, 0.08, half_w - 0.22, 0.14, h - 0.14)

    # Recessed seam backing plate so panel gaps show dark steel depth underneath
    append_box_to_bmesh(bm_hull, x_recess, x_front - 0.014, -half_w + 0.05, half_w - 0.05, 0.05, 0.19 - b_pad)
    append_box_to_bmesh(bm_hull, x_recess, x_front - 0.014, -half_w + 0.05, half_w - 0.05, 0.61 + b_pad, 0.76 - hero_pad)
    append_box_to_bmesh(bm_hull, x_recess, x_front - 0.014, -half_w + 0.05, half_w - 0.05, 1.22 + hero_pad, h - 0.05)

    # Segmented Front Steel Armor Plates (x_front - 0.014 .. x_front) separated by `seam` gaps:
    # Bottom Instrument Apron (3 segmented steel deck plates)
    bot_z1 = 0.19 - b_pad
    append_box_to_bmesh(bm_hull, x_front - 0.014, x_front, -half_w + 0.06, -0.52 - seam, 0.05, bot_z1)
    append_box_to_bmesh(bm_hull, x_front - 0.014, x_front + 0.008, -0.52 + seam, 0.18 - seam, 0.05, bot_z1)
    append_box_to_bmesh(bm_hull, x_front - 0.014, x_front, 0.18 + seam, half_w - 0.06, 0.05, bot_z1)

    # Center Horizontal Bulkhead Bridge (3 segmented plates across the waist)
    mid_z0 = 0.61 + b_pad
    mid_z1 = 0.76 - hero_pad
    append_box_to_bmesh(bm_hull, x_front - 0.014, x_front, -half_w + 0.06, -0.38 - seam, mid_z0, mid_z1)
    append_box_to_bmesh(bm_hull, x_front - 0.014, x_front + 0.006, -0.38 + seam, 0.12 - seam, mid_z0, mid_z1)
    append_box_to_bmesh(bm_hull, x_front - 0.014, x_front, 0.12 + seam, half_w - 0.06, mid_z0, mid_z1)

    # Top Header Bulkhead (2 segmented plates above Bankroll and Ante)
    top_z0 = 1.22 + hero_pad
    append_box_to_bmesh(bm_hull, x_front - 0.014, x_front, -half_w + 0.06, -0.02 - seam, top_z0, h - 0.05)
    append_box_to_bmesh(bm_hull, x_front - 0.014, x_front, 0.02 + seam, half_w - 0.06, 1.20 + b_pad, h - 0.05)

    # Left & Right Outer Vertical Instrument Stiles (split at waist seam)
    left_y1 = -1.52 - hero_pad
    right_y0 = 1.52 + b_pad
    append_box_to_bmesh(bm_hull, x_recess, x_front, -half_w + 0.05, left_y1, bot_z1 + seam, 0.68 - seam)
    append_box_to_bmesh(bm_hull, x_recess, x_front, -half_w + 0.05, left_y1, 0.68 + seam, top_z0 - seam)
    append_box_to_bmesh(bm_hull, x_recess, x_front, right_y0, half_w - 0.05, bot_z1 + seam, 0.68 - seam)
    append_box_to_bmesh(bm_hull, x_recess, x_front, right_y0, half_w - 0.05, 0.68 + seam, 1.20 + b_pad - seam)

    # Top-Center Vertical Bridge Pillar (between Hero Bankroll and Ante)
    append_box_to_bmesh(bm_hull, x_recess, x_front + 0.006, -0.10 + hero_pad, 0.14 - b_pad, mid_z1 + seam, top_z0 - seam)

    # Bottom-Center Vertical Instrument Pillar (between Unbanked Pool and Multiplier, housing recessed switches)
    append_box_to_bmesh(bm_hull, x_recess, x_front + 0.006, -0.40 + b_pad, -0.16 - b_pad, bot_z1 + seam, mid_z0 - seam)

    hull_obj = pipeline.create_mesh_object(
        name="SM_Scoreboard_Hull_Main",
        bm=bm_hull,
        collection_key="HULL",
        material_key="Hull_DarkSteel",
        origin=(0.0, 0.0, 0.0),
        bevel_width=cfg.hull_bevel_width,
        bevel_segments=cfg.hull_bevel_segments,
    )
    created.append(hull_obj)

    # 2. Heavy Burgundy Enamel Pinball Backbox Shroud & Deep Glare Visor (`SM_Scoreboard_Cowl_Oxblood`)
    bm_cowl = bmesh.new()
    cowl_x0 = x_back + 0.015
    cowl_x1 = x_front + 0.030

    # Deep overhanging top sun-visor brow (projects +9.5cm past the front face to cast dramatic shadow over the CRTs)
    append_box_to_bmesh(bm_cowl, cowl_x0, x_front + 0.095, -half_w - 0.01, half_w + 0.01, h - 0.045, h + 0.025)
    # Stepped upper crown riser over the Hero Bankroll side (asymmetric 1980s pinball backbox silhouette accent)
    append_box_to_bmesh(bm_cowl, cowl_x0 + 0.06, x_front + 0.065, -half_w + 0.08, -0.04, h + 0.020, h + 0.062)

    # Bottom heavy base plinth (segmented with center recessed channel)
    append_box_to_bmesh(bm_cowl, cowl_x0, cowl_x1 + 0.015, -half_w - 0.01, -0.35, -0.015, 0.052)
    append_box_to_bmesh(bm_cowl, cowl_x0, cowl_x1 + 0.005, -0.33, 0.33, -0.015, 0.045)
    append_box_to_bmesh(bm_cowl, cowl_x0, cowl_x1 + 0.015, 0.35, half_w + 0.01, -0.015, 0.052)

    # Segmented Left & Right Armored Side Cheeks + Upper Visor Side Blinders
    for side in (-1.0, 1.0):
        y_in = side * (half_w - 0.048)
        y_out = side * (half_w + 0.024)
        ym0, ym1 = min(y_in, y_out), max(y_in, y_out)
        # Lower cheek plate
        append_box_to_bmesh(bm_cowl, cowl_x0, cowl_x1, ym0, ym1, 0.04, 0.68 - seam * 0.5)
        # Upper cheek plate with forward visor wing extension
        append_box_to_bmesh(bm_cowl, cowl_x0, x_front + 0.070, ym0, ym1, 0.68 + seam * 0.5, h - 0.04)
        # Heavy corner bumper bolsters (top & bottom corners)
        yb_out = side * (half_w + 0.036)
        yb0, yb1 = min(y_in, yb_out), max(y_in, yb_out)
        append_box_to_bmesh(bm_cowl, cowl_x1 - 0.08, x_front + 0.050, yb0, yb1, h - 0.16, h + 0.03)
        append_box_to_bmesh(bm_cowl, cowl_x1 - 0.08, cowl_x1 + 0.025, yb0, yb1, -0.02, 0.15)

    cowl_obj = pipeline.create_mesh_object(
        name="SM_Scoreboard_Cowl_Oxblood",
        bm=bm_cowl,
        collection_key="HULL",
        material_key="Accent_Oxblood",
        origin=(0.0, 0.0, 0.0),
        bevel_width=0.009,
        bevel_segments=2,
    )
    created.append(cowl_obj)

    # 3. Deep Recessed CRT Shadow Wells, Physical CRT Monitor Masks & Recessed Switch Pocket (`SM_Scoreboard_Hull_BayLiner`)
    bm_liner = bmesh.new()
    for comp in layouts:
        pad = hero_pad if comp.is_hero else b_pad
        # Back shadow plate behind the CRT glass
        append_box_to_bmesh(
            bm_liner,
            x_recess - 0.012,
            x_recess + 0.006,
            comp.y_min - pad,
            comp.y_max + pad,
            comp.z_min - pad,
            comp.z_max + pad,
        )
        # Deep CRT monitor shroud walls (lining the 11.5cm tunnel from x_recess to x_front)
        build_frame_strip_bmesh(
            bm_liner,
            x_min=x_recess + 0.004,
            x_max=x_front - 0.004,
            y_min=comp.y_min + 0.018,
            y_max=comp.y_max - 0.018,
            z_min=comp.z_min + 0.018,
            z_max=comp.z_max - 0.018,
            border=0.026,
        )
        # Inner CRT rubber/matte phosphor border mask resting directly in front of the CRT glass perimeter
        build_frame_strip_bmesh(
            bm_liner,
            x_min=x_recess + 0.026,
            x_max=x_recess + 0.048,
            y_min=comp.y_min + 0.030,
            y_max=comp.y_max - 0.030,
            z_min=comp.z_min + 0.028,
            z_max=comp.z_max - 0.028,
            border=0.018,
        )

    # Recessed Service Switch Bay Liner on the lower-center instrument pillar
    append_box_to_bmesh(bm_liner, x_front - 0.028, x_front + 0.008, -0.34, -0.22, 0.22, 0.58)

    liner_obj = pipeline.create_mesh_object(
        name="SM_Scoreboard_Hull_BayLiner",
        bm=bm_liner,
        collection_key="HULL",
        material_key="Cavity_MatteBlack",
        origin=(0.0, 0.0, 0.0),
        bevel_width=0.003,
        bevel_segments=1,
    )
    created.append(liner_obj)

    return created


def build_brass_trim(
    pipeline: UE5AssetPipeline,
    cfg: ScoreboardConfig,
    layouts: List[CompartmentLayout],
) -> List[bpy.types.Object]:
    """
    Builds the tarnished brass trim collection (`02_Brass_Trim`) with clear visual hierarchy:
      - `SM_Scoreboard_Trim_OuterFrame`: Heavy perimeter brass frame, stepped mullion divider bars,
        and corner reinforcement gussets.
      - `SM_Scoreboard_Bezel_Bankroll`: HERO double-stepped command bezel with thicker brass molding,
        corner reinforcement plates, and top crown marquee header frame.
      - `SM_Scoreboard_Bezel_Ante`, `_UnbankedPool`, `_Multiplier`: Stepped secondary CRT bezels.
    """
    created: List[bpy.types.Object] = []
    half_w = cfg.total_width * 0.5
    h = cfg.total_height
    x_front = 0.20

    # 1. Outer perimeter frame + structural divider trim bars + corner gusset plates
    bm_outer = bmesh.new()
    build_frame_strip_bmesh(
        bm_outer,
        x_min=x_front - 0.012,
        x_max=x_front + 0.034,
        y_min=-half_w + 0.085,
        y_max=half_w - 0.085,
        z_min=0.085,
        z_max=h - 0.085,
        border=0.042,
    )
    # Center horizontal brass structural rail
    append_box_to_bmesh(
        bm_outer,
        x_front - 0.006,
        x_front + 0.030,
        -half_w + 0.085,
        half_w - 0.085,
        0.662,
        0.706,
    )
    # Top vertical brass mullion (between Bankroll & Ante)
    append_box_to_bmesh(
        bm_outer,
        x_front - 0.006,
        x_front + 0.030,
        -0.005,
        0.040,
        0.706,
        h - 0.085,
    )
    # Bottom vertical brass mullion frame around the recessed switch bay
    append_box_to_bmesh(
        bm_outer,
        x_front - 0.006,
        x_front + 0.026,
        -0.358,
        -0.338,
        0.085,
        0.662,
    )
    append_box_to_bmesh(
        bm_outer,
        x_front - 0.006,
        x_front + 0.026,
        -0.222,
        -0.202,
        0.085,
        0.662,
    )

    outer_trim_obj = pipeline.create_mesh_object(
        name="SM_Scoreboard_Trim_OuterFrame",
        bm=bm_outer,
        collection_key="TRIM",
        material_key="Trim_TarnishedBrass",
        origin=(0.0, 0.0, 0.0),
        bevel_width=cfg.trim_bevel_width,
        bevel_segments=cfg.trim_bevel_segments,
    )
    created.append(outer_trim_obj)

    # 2. Modular per-compartment CRT Brass Bezels (with strong HERO hierarchy on Bankroll)
    x_recess = x_front - cfg.bay_recess_depth
    for comp in layouts:
        bm_bezel = bmesh.new()
        half_by = comp.width * 0.5
        half_bz = comp.height * 0.5

        if comp.is_hero:
            # HERO BANKROLL BEZEL: Double-stepped heavy brass frame + corner gusset lugs + top crown header bar
            depth = cfg.bankroll_bezel_depth
            border = cfg.bankroll_bezel_thickness
            # Outer primary heavy brass flange (projects proud to x = 0.236)
            build_frame_strip_bmesh(
                bm_bezel,
                x_min=-depth * 0.5,
                x_max=depth * 0.5,
                y_min=-half_by,
                y_max=half_by,
                z_min=-half_bz,
                z_max=half_bz,
                border=border,
            )
            # Inner stepped brass retainer ring recessed toward the CRT glass
            build_frame_strip_bmesh(
                bm_bezel,
                x_min=-depth * 0.5 - 0.018,
                x_max=depth * 0.5 - 0.020,
                y_min=-half_by + 0.016,
                y_max=half_by - 0.016,
                z_min=-half_bz + 0.016,
                z_max=half_bz - 0.016,
                border=0.018,
            )
            # 4 Heavy Corner Escutcheon Lugs emphasizing the Bankroll panel
            for sy in (-1.0, 1.0):
                for sz in (-1.0, 1.0):
                    cy = sy * (half_by + border * 0.65)
                    cz = sz * (half_bz + border * 0.65)
                    append_box_to_bmesh(
                        bm_bezel,
                        depth * 0.5 - 0.012,
                        depth * 0.5 + 0.012,
                        cy - 0.045,
                        cy + 0.045,
                        cz - 0.035,
                        cz + 0.035,
                    )
            # Top Crown Header Plinth centered above the Bankroll frame
            append_box_to_bmesh(
                bm_bezel,
                depth * 0.5 - 0.020,
                depth * 0.5 + 0.014,
                -half_by * 0.62,
                half_by * 0.62,
                half_bz + border,
                half_bz + border + 0.042,
            )
            bezel_origin = (x_front - 0.008, comp.center_y, comp.center_z)
        else:
            # Secondary compartments: crisp stepped industrial brass bezel
            depth = cfg.bezel_depth
            border = cfg.bezel_thickness
            build_frame_strip_bmesh(
                bm_bezel,
                x_min=-depth * 0.5,
                x_max=depth * 0.5,
                y_min=-half_by,
                y_max=half_by,
                z_min=-half_bz,
                z_max=half_bz,
                border=border,
            )
            # Inner stepped lip leading into the deep CRT shadow well
            build_frame_strip_bmesh(
                bm_bezel,
                x_min=-depth * 0.5 - 0.016,
                x_max=depth * 0.5 - 0.018,
                y_min=-half_by + 0.014,
                y_max=half_by - 0.014,
                z_min=-half_bz + 0.014,
                z_max=half_bz - 0.014,
                border=0.015,
            )
            bezel_origin = (x_front - 0.016, comp.center_y, comp.center_z)

        bezel_obj = pipeline.create_mesh_object(
            name=comp.bezel_object_name,
            bm=bm_bezel,
            collection_key="TRIM",
            material_key="Trim_TarnishedBrass",
            origin=bezel_origin,
            bevel_width=0.004,
            bevel_segments=2,
        )
        bezel_obj["compartment_id"] = comp.key
        bezel_obj["is_hero_bezel"] = comp.is_hero
        created.append(bezel_obj)

    return created


def build_placeholder_crt_screens(
    pipeline: UE5AssetPipeline,
    cfg: ScoreboardConfig,
    layouts: List[CompartmentLayout],
) -> List[bpy.types.Object]:
    """
    Builds the 4 modular, deeply recessed, watertight 3D CRT glass screen cartridges (`03_Display_Screens`):
      - `SM_Scoreboard_Screen_Bankroll`
      - `SM_Scoreboard_Screen_Ante`
      - `SM_Scoreboard_Screen_UnbankedPool`
      - `SM_Scoreboard_Screen_Multiplier`

    Features:
      - Deeply recessed behind the outer housing & brass bezels (`x_screen = 0.105m`, 9.5cm behind faceplate)
      - Dark smoked teal / near-black CRT phosphor glass with pronounced pillow/barrel curvature
      - Zero modeled text or score digits (ready for UE5 runtime text, numeric displays, or render targets)
      - Closed manifold 3D glass cartridge (`non_manifold_edges == 0`)
      - Clean normalized [0..1] UV map (`UVMap`) filling the entire screen surface
      - Local pivot centered on each screen (`(x_screen, center_y, center_z)`)
    """
    created: List[bpy.types.Object] = []
    x_front = 0.20
    # Seat the CRT glass deep inside the 11.5cm recessed bay
    x_screen = x_front - cfg.bay_recess_depth + 0.020

    nx = cfg.screen_grid_x
    ny = cfg.screen_grid_y

    for comp in layouts:
        bm = bmesh.new()
        uv_layer = bm.loops.layers.uv.verify()

        w = comp.width
        h = comp.height

        front_grid: List[List[bmesh.types.BMVert]] = []
        back_grid: List[List[bmesh.types.BMVert]] = []

        for iy in range(ny + 1):
            v_norm = iy / float(ny)          # 0.0 (bottom) .. 1.0 (top)
            yn = (v_norm - 0.5) * 2.0        # -1.0 .. +1.0
            z_local = (v_norm - 0.5) * h

            f_row: List[bmesh.types.BMVert] = []
            b_row: List[bmesh.types.BMVert] = []
            for ix in range(nx + 1):
                u_norm = ix / float(nx)      # 0.0 (left) .. 1.0 (right)
                xn = (u_norm - 0.5) * 2.0    # -1.0 .. +1.0
                y_local = (u_norm - 0.5) * w

                # Smooth CRT biquadratic dome: max at center (0,0), 0 at edges (±1, ±1)
                dome = max(0.0, (1.0 - xn * xn) * (1.0 - yn * yn))
                x_local = cfg.crt_bulge * dome

                f_Row_vert = bm.verts.new((x_local, y_local, z_local))
                b_Row_vert = bm.verts.new((-cfg.crt_glass_depth, y_local, z_local))
                f_row.append(f_Row_vert)
                b_row.append(b_Row_vert)
            front_grid.append(f_row)
            back_grid.append(b_row)

        bm.verts.ensure_lookup_table()

        def set_quad_uvs(face: bmesh.types.BMFace, uvs: List[Tuple[float, float]]) -> None:
            for loop, uv in zip(face.loops, uvs):
                loop[uv_layer].uv = uv

        # 1. Front CRT curved quads (+X normal) and Back flat quads (-X normal)
        for iy in range(ny):
            for ix in range(nx):
                u0, u1 = ix / float(nx), (ix + 1) / float(nx)
                v0, v1 = iy / float(ny), (iy + 1) / float(ny)

                f_face = bm.faces.new((
                    front_grid[iy][ix],
                    front_grid[iy][ix + 1],
                    front_grid[iy + 1][ix + 1],
                    front_grid[iy + 1][ix],
                ))
                f_face.smooth = True
                set_quad_uvs(f_face, [(u0, v0), (u1, v0), (u1, v1), (u0, v1)])

                b_face = bm.faces.new((
                    back_grid[iy][ix],
                    back_grid[iy + 1][ix],
                    back_grid[iy + 1][ix + 1],
                    back_grid[iy][ix + 1],
                ))
                b_face.smooth = False
                set_quad_uvs(b_face, [(u0, v0), (u0, v1), (u1, v1), (u1, v0)])

        # 2. Stitch perimeter rim faces so the CRT glass cartridge is 100% closed manifold
        for ix in range(nx):
            u0, u1 = ix / float(nx), (ix + 1) / float(nx)
            f_bot = bm.faces.new((
                back_grid[0][ix],
                back_grid[0][ix + 1],
                front_grid[0][ix + 1],
                front_grid[0][ix],
            ))
            set_quad_uvs(f_bot, [(u0, 0.0), (u1, 0.0), (u1, 0.0), (u0, 0.0)])

            f_top = bm.faces.new((
                front_grid[ny][ix],
                front_grid[ny][ix + 1],
                back_grid[ny][ix + 1],
                back_grid[ny][ix],
            ))
            set_quad_uvs(f_top, [(u0, 1.0), (u1, 1.0), (u1, 1.0), (u0, 1.0)])

        for iy in range(ny):
            v0, v1 = iy / float(ny), (iy + 1) / float(ny)
            f_left = bm.faces.new((
                back_grid[iy][0],
                front_grid[iy][0],
                front_grid[iy + 1][0],
                back_grid[iy + 1][0],
            ))
            set_quad_uvs(f_left, [(0.0, v0), (0.0, v0), (0.0, v1), (0.0, v1)])

            f_right = bm.faces.new((
                front_grid[iy][nx],
                back_grid[iy][nx],
                back_grid[iy + 1][nx],
                front_grid[iy + 1][nx],
            ))
            set_quad_uvs(f_right, [(1.0, v0), (1.0, v0), (1.0, v1), (1.0, v1)])

        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

        mesh = bpy.data.meshes.new(comp.screen_object_name)
        bm.to_mesh(mesh)
        bm.free()

        screen_origin = (x_screen, comp.center_y, comp.center_z)
        obj = bpy.data.objects.new(comp.screen_object_name, mesh)
        obj.location = Vector(screen_origin)
        pipeline.link_object_to_collection(obj, "SCREENS")

        if comp.material_key in pipeline.materials:
            mesh.materials.append(pipeline.materials[comp.material_key])

        obj["compartment_id"] = comp.key
        obj["screen_width_cm"] = round(w * 100.0, 2)
        obj["screen_height_cm"] = round(h * 100.0, 2)
        obj["is_placeholder_screen"] = True

        created.append(obj)

    return created


def build_mechanical_details(
    pipeline: UE5AssetPipeline,
    cfg: ScoreboardConfig,
    layouts: List[CompartmentLayout],
) -> List[bpy.types.Object]:
    """
    Builds reusable, low-poly 1980s pinball & analog industrial hardware (`04_Mechanical_Details`):
      - `SM_Scoreboard_Mech_SideMounts`: Articulated cast-steel trunnion yokes, pivot barrels, and top hoist lugs
      - `SM_Scoreboard_Mech_VentLouvers`: Under-brow exhaust slots, side-cheek louvers, and top transformer heat-sink fins
      - `SM_Scoreboard_Mech_ControlSwitches`: Recessed service toggle switches, U-guards, and calibration dials
      - `SM_Scoreboard_Mech_CornerBolts`: Exposed hex fasteners, Hero Bankroll corner bolts, and brass indicator collars
      - `SM_Scoreboard_Mech_StatusLamps_Cyan` & `_Amber`: Domed indicator bulbs + Hero Bankroll meter LEDs + standby CRT traces
    """
    created: List[bpy.types.Object] = []
    half_w = cfg.total_width * 0.5
    h = cfg.total_height
    x_back = -0.28
    x_front = 0.20
    rot_y_90 = Matrix.Rotation(math.radians(90.0), 4, "Y")
    rot_x_90 = Matrix.Rotation(math.radians(90.0), 4, "X")

    # 1. Heavy Trunnion Side Mounting Yokes + Top Suspension Eye-Brackets (`SM_Scoreboard_Mech_SideMounts`)
    bm_mounts = bmesh.new()
    for side in (-1.0, 1.0):
        y_inner = side * (half_w + 0.020)
        y_outer = side * (half_w + 0.135)
        y_min, y_max = min(y_inner, y_outer), max(y_inner, y_outer)
        # Main base mounting plate bolted to side cheek
        append_box_to_bmesh(bm_mounts, -0.18, 0.10, y_min, y_min + 0.035 if side > 0 else y_max - 0.035, h * 0.24, h * 0.84)
        # Upper & lower horizontal trunnion yoke arms
        append_box_to_bmesh(bm_mounts, -0.15, 0.07, y_min, y_max, h * 0.64, h * 0.72)
        append_box_to_bmesh(bm_mounts, -0.15, 0.07, y_min, y_max, h * 0.36, h * 0.44)
        # Outer vertical tie-bar & structural gusset web
        append_box_to_bmesh(bm_mounts, -0.12, 0.04, y_max - 0.032 if side > 0 else y_min, y_max if side > 0 else y_min + 0.032, h * 0.36, h * 0.72)
        # Cylindrical trunnion pivot barrel (8-sided cylinder along Y axis)
        mat_cyl = Matrix.Translation((-0.04, side * (half_w + 0.150), h * 0.54)) @ rot_x_90
        bmesh.ops.create_cone(
            bm_mounts,
            cap_ends=True,
            cap_tris=True,
            segments=8,
            radius1=0.055,
            radius2=0.055,
            depth=0.045,
            matrix=mat_cyl,
        )

    # Top-mounted suspension hoist brackets (left and right of top deck)
    for hoist_y in (-1.25, 1.25):
        append_box_to_bmesh(bm_mounts, -0.16, 0.04, hoist_y - 0.09, hoist_y + 0.09, h + 0.015, h + 0.055)
        append_box_to_bmesh(bm_mounts, -0.12, 0.00, hoist_y - 0.03, hoist_y + 0.03, h + 0.055, h + 0.125)

    mounts_obj = pipeline.create_mesh_object(
        name="SM_Scoreboard_Mech_SideMounts",
        bm=bm_mounts,
        collection_key="MECH",
        material_key="Hull_DarkSteel",
        origin=(0.0, 0.0, 0.0),
        bevel_width=0.004,
        bevel_segments=1,
    )
    created.append(mounts_obj)

    # 2. Ventilation Louver Slots & Top Transformer Cooling Fins (`SM_Scoreboard_Mech_VentLouvers`)
    bm_vents = bmesh.new()
    # Rear spine louvers (2 banks of 7 angled slats)
    for bank_y in (-1.05, 1.05):
        for i in range(7):
            z_center = 0.30 + i * 0.11
            append_box_to_bmesh(
                bm_vents,
                x_back - 0.078,
                x_back - 0.048,
                bank_y - 0.38,
                bank_y + 0.38,
                z_center - 0.016,
                z_center + 0.016,
            )

    # Front under-brow exhaust louver gills on the upper dark steel header plate (z = 1.276 .. 1.298)
    for i in range(9):
        slot_y_left = -1.44 + i * 0.145
        slot_y_right = 0.20 + i * 0.145
        append_box_to_bmesh(bm_vents, x_front - 0.006, x_front + 0.012, slot_y_left, slot_y_left + 0.090, 1.276, 1.298)
        append_box_to_bmesh(bm_vents, x_front - 0.006, x_front + 0.012, slot_y_right, slot_y_right + 0.090, 1.276, 1.298)

    # Side-cheek angled ventilation slots (visible in 3/4 view)
    for side in (-1.0, 1.0):
        y0 = side * (half_w + 0.020)
        y1 = side * (half_w + 0.034)
        ym0, ym1 = min(y0, y1), max(y0, y1)
        for i in range(5):
            zc = 0.80 + i * 0.075
            append_box_to_bmesh(bm_vents, -0.14, 0.12, ym0, ym1, zc - 0.016, zc + 0.016)

    # Top-rear rhythmic transformer heat-sink fins (visible along upper silhouette)
    for i in range(11):
        fin_y = -0.90 + i * 0.18
        append_box_to_bmesh(bm_vents, -0.24, -0.02, fin_y - 0.012, fin_y + 0.012, h + 0.015, h + 0.075)

    vents_obj = pipeline.create_mesh_object(
        name="SM_Scoreboard_Mech_VentLouvers",
        bm=bm_vents,
        collection_key="MECH",
        material_key="Hull_DarkSteel",
        origin=(0.0, 0.0, 0.0),
        bevel_width=0.002,
        bevel_segments=1,
    )
    created.append(vents_obj)

    # 3. Recessed Toggle Switches, Rotary Dials & Service Controls (`SM_Scoreboard_Mech_ControlSwitches`)
    bm_switches = bmesh.new()
    # 3 Recessed toggle switches + U-shaped guard rails in the lower-center service bay (y = -0.28, z = 0.27..0.51)
    for i, sz in enumerate((0.28, 0.39, 0.50)):
        # Switch base nut (hex)
        mat_nut = Matrix.Translation((x_front - 0.012, -0.28, sz)) @ rot_y_90
        bmesh.ops.create_cone(
            bm_switches,
            cap_ends=True,
            cap_tris=True,
            segments=6,
            radius1=0.016,
            radius2=0.016,
            depth=0.014,
            matrix=mat_nut,
        )
        # Toggle bat lever (angled up or down)
        bat_tilt = math.radians(-22.0 if i != 1 else 22.0)
        mat_bat = (
            Matrix.Translation((x_front + 0.012, -0.28, sz))
            @ Matrix.Rotation(bat_tilt, 4, "Y")
            @ rot_y_90
        )
        bmesh.ops.create_cone(
            bm_switches,
            cap_ends=True,
            cap_tris=True,
            segments=6,
            radius1=0.006,
            radius2=0.011,
            depth=0.038,
            matrix=mat_bat,
        )
        # Protective side guard fins flanking each toggle switch
        for gy in (-0.312, -0.248):
            append_box_to_bmesh(bm_switches, x_front - 0.016, x_front + 0.026, gy - 0.004, gy + 0.004, sz - 0.022, sz + 0.022)

    # 2 Knurled Rotary Calibration Dials on the bottom control apron (under Bankroll)
    for dial_y in (-0.44, -0.28):
        mat_dial = Matrix.Translation((x_front + 0.018, dial_y, 0.095)) @ rot_y_90
        bmesh.ops.create_cone(
            bm_switches,
            cap_ends=True,
            cap_tris=True,
            segments=10,
            radius1=0.028,
            radius2=0.024,
            depth=0.024,
            matrix=mat_dial,
        )
        # Dial pointer ridge bar
        append_box_to_bmesh(bm_switches, x_front + 0.026, x_front + 0.036, dial_y - 0.004, dial_y + 0.004, 0.090, 0.120)

    switches_obj = pipeline.create_mesh_object(
        name="SM_Scoreboard_Mech_ControlSwitches",
        bm=bm_switches,
        collection_key="MECH",
        material_key="Trim_TarnishedBrass",
        origin=(0.0, 0.0, 0.0),
        bevel_width=0.0,
    )
    created.append(switches_obj)

    # 4. Exposed Hex Fasteners & Brass Lamp Turret Collars (`SM_Scoreboard_Mech_CornerBolts`)
    bm_bolts = bmesh.new()
    bolt_positions = [
        # Outer frame perimeter bolts
        (-half_w + 0.068, 0.068, x_front + 0.036, 0.020),
        (half_w - 0.068, 0.068, x_front + 0.036, 0.020),
        (-half_w + 0.068, h - 0.068, x_front + 0.036, 0.020),
        (half_w - 0.068, h - 0.068, x_front + 0.036, 0.020),
        (-0.82, 0.068, x_front + 0.036, 0.016),
        (0.018, 0.068, x_front + 0.036, 0.018),
        (0.82, 0.068, x_front + 0.036, 0.016),
        (-0.82, h - 0.068, x_front + 0.036, 0.016),
        (0.018, h - 0.068, x_front + 0.036, 0.018),
        (0.82, h - 0.068, x_front + 0.036, 0.016),
        (-half_w + 0.068, 0.684, x_front + 0.036, 0.018),
        (half_w - 0.068, 0.684, x_front + 0.036, 0.018),
        (0.018, 0.684, x_front + 0.032, 0.018),
        # Hero Bankroll corner escutcheon bolts (sitting proud on the 4 corner lugs)
        (-1.55, 1.25, x_front + 0.048, 0.016),
        (-0.07, 1.25, x_front + 0.048, 0.016),
        (-1.55, 0.73, x_front + 0.048, 0.016),
        (-0.07, 0.73, x_front + 0.048, 0.016),
        # Panel seam bolts along lower steel apron
        (-0.56, 0.10, x_front + 0.008, 0.012),
        (-0.48, 0.10, x_front + 0.014, 0.012),
        (0.14, 0.10, x_front + 0.014, 0.012),
        (0.22, 0.10, x_front + 0.008, 0.012),
    ]
    for by, bz, bx, rad in bolt_positions:
        mat = Matrix.Translation((bx, by, bz)) @ rot_y_90
        bmesh.ops.create_cone(
            bm_bolts,
            cap_ends=True,
            cap_tris=True,
            segments=6,
            radius1=rad,
            radius2=rad * 0.9,
            depth=0.016,
            matrix=mat,
        )

    # Brass turret collars housing the indicator bulbs
    lamp_collar_coords = [
        # Outer stile main status lamps
        (-1.635, 1.08, 0.022),
        (-1.635, 0.99, 0.016),
        (-1.635, 0.90, 0.016),
        (1.635, 0.99, 0.022),
        (-1.635, 0.40, 0.022),
        (1.635, 0.40, 0.022),
    ]
    for ly, lz, lrad in lamp_collar_coords:
        mat_col = Matrix.Translation((x_front + 0.008, ly, lz)) @ rot_y_90
        bmesh.ops.create_cone(
            bm_bolts,
            cap_ends=True,
            cap_tris=True,
            segments=8,
            radius1=lrad * 1.30,
            radius2=lrad * 1.15,
            depth=0.016,
            matrix=mat_col,
        )

    bolts_obj = pipeline.create_mesh_object(
        name="SM_Scoreboard_Mech_CornerBolts",
        bm=bm_bolts,
        collection_key="MECH",
        material_key="Trim_TarnishedBrass",
        origin=(0.0, 0.0, 0.0),
        bevel_width=0.0,
    )
    created.append(bolts_obj)

    # 5. Restrained Status Indicator Bulbs & Recessed CRT Standby Traces (`SM_Scoreboard_Mech_StatusLamps_*`)
    bm_lamps_cyan = bmesh.new()
    bm_lamps_amber = bmesh.new()

    cyan_bulbs = [
        (-1.635, 1.08, 0.018),  # Hero Bankroll top indicator
        (-1.635, 0.99, 0.012),  # Hero Bankroll mid indicator
        (-1.635, 0.90, 0.012),  # Hero Bankroll low indicator
        (-1.635, 0.40, 0.018),  # Unbanked Pool indicator
    ]
    amber_bulbs = [
        (1.635, 0.99, 0.018),   # Ante Quota indicator
        (1.635, 0.40, 0.018),   # Multiplier indicator
    ]

    for ly, lz, lrad in cyan_bulbs:
        mat = Matrix.Translation((x_front + 0.016, ly, lz)) @ rot_y_90
        bmesh.ops.create_cone(
            bm_lamps_cyan,
            cap_ends=True,
            cap_tris=True,
            segments=8,
            radius1=lrad,
            radius2=lrad * 0.72,
            depth=0.016,
            matrix=mat,
        )

    for ly, lz, lrad in amber_bulbs:
        mat = Matrix.Translation((x_front + 0.016, ly, lz)) @ rot_y_90
        bmesh.ops.create_cone(
            bm_lamps_amber,
            cap_ends=True,
            cap_tris=True,
            segments=8,
            radius1=lrad,
            radius2=lrad * 0.72,
            depth=0.016,
            matrix=mat,
        )

    # Tiny restrained CRT hardware standby calibration ticks tucked inside the bottom-left of each screen recess
    x_trace = x_front - cfg.bay_recess_depth + 0.036
    for comp in layouts:
        target_bm = bm_lamps_cyan if comp.indicator_mat_key == "Emissive_Cyan" else bm_lamps_amber
        ty0 = comp.y_min + 0.055
        tz0 = comp.z_min + 0.042
        append_box_to_bmesh(target_bm, x_trace, x_trace + 0.006, ty0, ty0 + 0.065, tz0, tz0 + 0.010)
        append_box_to_bmesh(target_bm, x_trace, x_trace + 0.006, ty0 + 0.080, ty0 + 0.100, tz0, tz0 + 0.010)

    lamps_cyan_obj = pipeline.create_mesh_object(
        name="SM_Scoreboard_Mech_StatusLamps_Cyan",
        bm=bm_lamps_cyan,
        collection_key="MECH",
        material_key="Emissive_Cyan",
        origin=(0.0, 0.0, 0.0),
    )
    lamps_amber_obj = pipeline.create_mesh_object(
        name="SM_Scoreboard_Mech_StatusLamps_Amber",
        bm=bm_lamps_amber,
        collection_key="MECH",
        material_key="Emissive_Amber",
        origin=(0.0, 0.0, 0.0),
    )
    created.extend([lamps_cyan_obj, lamps_amber_obj])

    return created


def build_unreal_collision(pipeline: UE5AssetPipeline, cfg: ScoreboardConfig) -> bpy.types.Object:
    """
    Builds a clean 8-vertex convex collision mesh named `UCX_SM_Scoreboard_Hull_Main_01`
    so Unreal Engine 5 automatically imports it as simple box/convex collision for `SM_Scoreboard_Hull_Main`.
    """
    bm_col = bmesh.new()
    half_w = cfg.total_width * 0.5 + 0.02
    append_box_to_bmesh(bm_col, -0.34, 0.25, -half_w, half_w, 0.0, cfg.total_height + 0.02)

    col_obj = pipeline.create_mesh_object(
        name="UCX_SM_Scoreboard_Hull_Main_01",
        bm=bm_col,
        collection_key="COLLISION",
        material_key=None,
        origin=(0.0, 0.0, 0.0),
        bevel_width=0.0,
        auto_uv=False,
    )
    col_obj.display_type = "WIRE"
    col_obj.hide_render = True
    return col_obj


# ==============================================================================
# 4. PIPELINE EXECUTION & SELF-VERIFICATION MANIFEST
# ==============================================================================

def collect_scene_manifest(cfg: ScoreboardConfig, blend_path: str, fbx_path: str) -> Dict:
    """Inspects all generated meshes, collections, UVs, and polycounts for QA verification."""
    objects_report = []
    total_verts = 0
    total_faces = 0
    total_tris = 0

    for obj in sorted(bpy.data.objects, key=lambda o: o.name):
        if obj.type != "MESH":
            continue
        mesh = obj.data
        mesh.calc_loop_triangles()

        v_count = len(mesh.vertices)
        f_count = len(mesh.polygons)
        t_count = len(mesh.loop_triangles)
        uv_layers = [uv.name for uv in mesh.uv_layers]
        mats = [m.name for m in mesh.materials if m is not None]
        cols = [c.name for c in obj.users_collection]

        # Compute world-space bounding box
        world_corners = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
        min_v = [min(c[i] for c in world_corners) for i in range(3)]
        max_v = [max(c[i] for c in world_corners) for i in range(3)]
        dims_cm = [round((max_v[i] - min_v[i]) * 100.0, 2) for i in range(3)]

        # Check non-manifold edges on render meshes
        bm = bmesh.new()
        bm.from_mesh(mesh)
        non_manifold_edges = sum(1 for e in bm.edges if not e.is_manifold)
        degenerate_faces = sum(1 for f in bm.faces if f.calc_area() <= 1e-8)
        ngon_faces = sum(1 for f in bm.faces if len(f.verts) > 4)
        bm.free()

        if not obj.name.startswith("UCX_"):
            total_verts += v_count
            total_faces += f_count
            total_tris += t_count

        objects_report.append({
            "name": obj.name,
            "collections": cols,
            "origin_m": [round(v, 4) for v in obj.location],
            "scale": [round(v, 4) for v in obj.scale],
            "dimensions_cm": {"x_depth": dims_cm[0], "y_width": dims_cm[1], "z_height": dims_cm[2]},
            "vertices": v_count,
            "polygons": f_count,
            "triangles": t_count,
            "ngon_faces": ngon_faces,
            "materials": mats,
            "uv_layers": uv_layers,
            "non_manifold_edges": non_manifold_edges,
            "degenerate_faces": degenerate_faces,
            "compartment_id": obj.get("compartment_id", None),
        })

    return {
        "asset_name": cfg.asset_name,
        "pipeline_version": "1.0.0-v1-testable",
        "unreal_scale_note": "1.0m in Blender = 100cm in UE5.3 (built for 10x CrapsCrapsCraps table scale)",
        "coordinate_convention": {"forward": "+X", "right": "+Y", "up": "+Z"},
        "files": {
            "blend_file": blend_path,
            "blend_size_bytes": os.path.getsize(blend_path) if os.path.exists(blend_path) else 0,
            "fbx_file": fbx_path,
            "fbx_size_bytes": os.path.getsize(fbx_path) if os.path.exists(fbx_path) else 0,
        },
        "totals_excluding_collision": {
            "mesh_object_count": sum(1 for o in objects_report if not o["name"].startswith("UCX_")),
            "collision_object_count": sum(1 for o in objects_report if o["name"].startswith("UCX_")),
            "total_vertices": total_verts,
            "total_polygons": total_faces,
            "total_triangles": total_tris,
        },
        "collections": {
            col.name: sorted([o.name for o in col.objects])
            for col in bpy.data.collections
        },
        "objects": objects_report,
    }


def setup_preview_camera_and_lighting() -> None:
    """Adds a non-exported close-up preview camera & warm casino lighting rig in a separate collection."""
    preview_col = bpy.data.collections.new("99_Viewport_Preview_Only")
    bpy.context.scene.collection.children.link(preview_col)

    world = bpy.context.scene.world
    if world is None:
        world = bpy.data.worlds.new("SynthNoirWorld")
        bpy.context.scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        # Deep warm-tinted casino void (#090608)
        bg.inputs[0].default_value = (0.014, 0.009, 0.011, 1.0)
        bg.inputs[1].default_value = 0.45

    # 1. Primary Warm Casino Chandelier / Raking Tungsten Key Light (grazing the recessed bays & brass bezels)
    key_data = bpy.data.lights.new(name="LGT_Key_WarmCasinoGold", type="AREA")
    key_data.energy = 460.0
    key_data.color = (1.0, 0.76, 0.44)
    key_data.size = 2.4
    if hasattr(key_data, "specular_factor"):
        key_data.specular_factor = 0.22
    key_obj = bpy.data.objects.new(name="LGT_Key_WarmCasinoGold", object_data=key_data)
    key_obj.location = Vector((2.20, -2.85, 2.65))
    direction = Vector((0.05, -0.35, 0.72)) - key_obj.location
    key_obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    preview_col.objects.link(key_obj)

    # 2. Warm Amber Specular Grazing Light (catches curved smoked CRT glass edges & brushed steel seams)
    spec_data = bpy.data.lights.new(name="LGT_Fill_WarmAmberLounge", type="AREA")
    spec_data.energy = 260.0
    spec_data.color = (1.0, 0.62, 0.28)
    spec_data.size = 2.8
    if hasattr(spec_data, "specular_factor"):
        spec_data.specular_factor = 0.18
    spec_obj = bpy.data.objects.new(name="LGT_Fill_WarmAmberLounge", object_data=spec_data)
    spec_obj.location = Vector((2.40, 2.75, 2.20))
    direction_spec = Vector((0.10, 0.15, 0.68)) - spec_obj.location
    spec_obj.rotation_euler = direction_spec.to_track_quat("-Z", "Y").to_euler()
    preview_col.objects.link(spec_obj)

    # 3. Overhead Pit Boss Rim Light (defines the top heat-sink fins, hoist brackets & burgundy visor hood)
    rim_data = bpy.data.lights.new(name="LGT_Rim_TopWarm", type="AREA")
    rim_data.energy = 320.0
    rim_data.color = (1.0, 0.84, 0.60)
    rim_data.size = 3.6
    rim_obj = bpy.data.objects.new(name="LGT_Rim_TopWarm", object_data=rim_data)
    rim_obj.location = Vector((0.55, -0.30, 2.85))
    direction_rim = Vector((0.0, 0.0, 1.25)) - rim_obj.location
    rim_obj.rotation_euler = direction_rim.to_track_quat("-Z", "Y").to_euler()
    preview_col.objects.link(rim_obj)

    # 4. Subtle Under-Table Cyan/Teal Felt Bounce Fill
    bounce_data = bpy.data.lights.new(name="LGT_Bounce_FeltTeal", type="AREA")
    bounce_data.energy = 85.0
    bounce_data.color = (0.12, 0.82, 0.75)
    bounce_data.size = 3.0
    if hasattr(bounce_data, "specular_factor"):
        bounce_data.specular_factor = 0.15
    bounce_obj = bpy.data.objects.new(name="LGT_Bounce_FeltTeal", object_data=bounce_data)
    bounce_obj.location = Vector((1.85, 0.0, -1.10))
    direction_bounce = Vector((0.10, 0.0, 0.65)) - bounce_obj.location
    bounce_obj.rotation_euler = direction_bounce.to_track_quat("-Z", "Y").to_euler()
    preview_col.objects.link(bounce_obj)

    # Close-up 3/4 Preview Camera showcasing recessed CRT depth, Hero Bankroll bezel, switches & materials
    cam_data = bpy.data.cameras.new(name="CAM_Scoreboard_Preview")
    cam_data.lens = 38.0
    cam_obj = bpy.data.objects.new(name="CAM_Scoreboard_Preview", object_data=cam_data)
    cam_obj.location = Vector((4.15, -1.18, 0.84))
    cam_dir = Vector((0.06, -0.02, 0.72)) - cam_obj.location
    cam_obj.rotation_euler = cam_dir.to_track_quat("-Z", "Y").to_euler()
    preview_col.objects.link(cam_obj)
    bpy.context.scene.camera = cam_obj


def generate_crt_scoreboard(project_root: Optional[str] = None) -> Dict:
    """Main entry point to build, save, export, and document the v1 CRT Scoreboard Enclosure."""
    if project_root is None:
        script_dir = os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else os.getcwd()
        if os.path.basename(script_dir).lower() == "scripts":
            project_root = os.path.abspath(os.path.join(script_dir, "..", ".."))
        else:
            project_root = r"C:\Code\Unreal Projects\Craps\CrapsCrapsCraps"

    cfg = ScoreboardConfig()
    pipeline = UE5AssetPipeline(project_root=project_root, root_collection_name="Asset_CRTScoreboard")
    pipeline.reset_scene()

    # 1. Create named sub-collections
    pipeline.create_subcollection("HULL", "01_Enclosure_Hull")
    pipeline.create_subcollection("TRIM", "02_Brass_Trim")
    pipeline.create_subcollection("SCREENS", "03_Display_Screens")
    pipeline.create_subcollection("MECH", "04_Mechanical_Details")
    pipeline.create_subcollection("COLLISION", "05_Collision")

    # 2. Build CRT Synth-Noir Material Palette
    pipeline.build_materials(PALETTE)

    # 3. Build modular geometry
    layouts = get_compartment_layouts(cfg)
    build_enclosure_hull(pipeline, cfg, layouts)
    build_brass_trim(pipeline, cfg, layouts)
    build_placeholder_crt_screens(pipeline, cfg, layouts)
    build_mechanical_details(pipeline, cfg, layouts)
    build_unreal_collision(pipeline, cfg)

    # 4. Add preview lighting/camera (excluded from FBX export because FBX exports only MESH objects)
    setup_preview_camera_and_lighting()

    # 5. Export FBX first (only MESH objects), then save .blend source file
    fbx_path = pipeline.export_fbx(cfg.fbx_filename)
    blend_path = pipeline.export_blend(cfg.blend_filename)

    # 6. Write verification manifest JSON
    manifest = collect_scene_manifest(cfg, blend_path, fbx_path)
    manifest_path = os.path.join(pipeline.export_dir, cfg.manifest_filename)
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    print(f"[UE5AssetPipeline] Generated .blend: {blend_path}")
    print(f"[UE5AssetPipeline] Exported .fbx:   {fbx_path}")
    print(f"[UE5AssetPipeline] Wrote manifest:  {manifest_path}")
    print(
        f"[UE5AssetPipeline] Summary: {manifest['totals_excluding_collision']['mesh_object_count']} meshes, "
        f"{manifest['totals_excluding_collision']['total_triangles']} triangles."
    )
    return manifest


if __name__ == "__main__":
    generate_crt_scoreboard()
