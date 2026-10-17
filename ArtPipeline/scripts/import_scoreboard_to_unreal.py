#!/usr/bin/env python3
"""
Imports SM_CRT_Scoreboard_v1.fbx into Unreal Engine 5.3 (/Game/Meshes/SM_CRT_Scoreboard_v1)
via the live Unreal Editor Python API.
"""
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "Tools", "ue5_mcp"))
import server

UE_SCRIPT = r"""
import traceback
try:
    fbx_path = r"C:/Code/Unreal Projects/Craps/CrapsCrapsCraps/ArtPipeline/exports/SM_CRT_Scoreboard_v1.fbx"
    dest_path = "/Game/Meshes"
    dest_name = "SM_CRT_Scoreboard_v1"

    asset_path = f"{dest_path}/{dest_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.delete_asset(asset_path)

    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)

    sm_data = options.get_editor_property("static_mesh_import_data")
    sm_data.set_editor_property("combine_meshes", True)
    sm_data.set_editor_property("transform_vertex_to_absolute", True)
    sm_data.set_editor_property("bake_pivot_in_vertex", False)
    sm_data.set_editor_property("auto_generate_collision", False)
    sm_data.set_editor_property("generate_lightmap_u_vs", True)
    sm_data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fbx_path)
    task.set_editor_property("destination_path", dest_path)
    task.set_editor_property("destination_name", dest_name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
    if mesh:
        mat_folder = "/Game/Materials/Scoreboard"
        if not unreal.EditorAssetLibrary.does_directory_exist(mat_folder):
            unreal.EditorAssetLibrary.make_directory(mat_folder)

        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        mel = unreal.MaterialEditingLibrary

        # 1. Create or load M_Scoreboard_Base (PBR master material with BaseColor, Metallic, Roughness, EmissiveColor)
        master_path = f"{mat_folder}/M_Scoreboard_Master"
        if unreal.EditorAssetLibrary.does_asset_exist(master_path):
            master_mat = unreal.EditorAssetLibrary.load_asset(master_path)
            mel.delete_all_material_expressions(master_mat)
        else:
            mat_factory = unreal.MaterialFactoryNew()
            master_mat = asset_tools.create_asset("M_Scoreboard_Master", mat_folder, unreal.Material, mat_factory)

        base_color_param = mel.create_material_expression(master_mat, unreal.MaterialExpressionVectorParameter, -380, -200)
        base_color_param.set_editor_property("parameter_name", "BaseColor")
        base_color_param.set_editor_property("default_value", unreal.LinearColor(0.05, 0.05, 0.06, 1.0))
        mel.connect_material_property(base_color_param, "", unreal.MaterialProperty.MP_BASE_COLOR)

        metallic_param = mel.create_material_expression(master_mat, unreal.MaterialExpressionScalarParameter, -380, 0)
        metallic_param.set_editor_property("parameter_name", "Metallic")
        metallic_param.set_editor_property("default_value", 0.8)
        mel.connect_material_property(metallic_param, "", unreal.MaterialProperty.MP_METALLIC)

        roughness_param = mel.create_material_expression(master_mat, unreal.MaterialExpressionScalarParameter, -380, 120)
        roughness_param.set_editor_property("parameter_name", "Roughness")
        roughness_param.set_editor_property("default_value", 0.35)
        mel.connect_material_property(roughness_param, "", unreal.MaterialProperty.MP_ROUGHNESS)

        emissive_param = mel.create_material_expression(master_mat, unreal.MaterialExpressionVectorParameter, -380, 260)
        emissive_param.set_editor_property("parameter_name", "EmissiveColor")
        emissive_param.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
        mel.connect_material_property(emissive_param, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

        mel.recompile_material(master_mat)
        unreal.EditorAssetLibrary.save_asset(master_path, only_if_is_dirty=False)

        # 2. Define the 10 CRT Synth-Noir Material Instance presets (BaseColor, Metallic, Roughness, EmissiveColor)
        # Colors are linear RGB matching PALETTE in generate_crt_scoreboard.py
        presets = {
            "MI_Hull_DarkSteel": (
                unreal.LinearColor(0.022, 0.025, 0.032, 1.0),
                0.88,
                0.42,
                unreal.LinearColor(0.0, 0.0, 0.0, 1.0),
            ),
            "MI_Trim_TarnishedBrass": (
                unreal.LinearColor(0.42, 0.27, 0.09, 1.0),
                0.94,
                0.28,
                unreal.LinearColor(0.0, 0.0, 0.0, 1.0),
            ),
            "MI_Accent_Oxblood": (
                unreal.LinearColor(0.14, 0.015, 0.022, 1.0),
                0.25,
                0.34,
                unreal.LinearColor(0.0, 0.0, 0.0, 1.0),
            ),
            "MI_Cavity_MatteBlack": (
                unreal.LinearColor(0.006, 0.007, 0.009, 1.0),
                0.15,
                0.82,
                unreal.LinearColor(0.0, 0.0, 0.0, 1.0),
            ),
            "MI_Screen_Bankroll_Cyan": (
                unreal.LinearColor(0.004, 0.024, 0.028, 1.0),
                0.08,
                0.06,
                unreal.LinearColor(0.0, 0.09, 0.085, 1.0),
            ),
            "MI_Screen_UnbankedPool_Cyan": (
                unreal.LinearColor(0.004, 0.020, 0.024, 1.0),
                0.08,
                0.07,
                unreal.LinearColor(0.0, 0.065, 0.060, 1.0),
            ),
            "MI_Screen_Ante_Amber": (
                unreal.LinearColor(0.026, 0.012, 0.003, 1.0),
                0.08,
                0.07,
                unreal.LinearColor(0.09, 0.042, 0.004, 1.0),
            ),
            "MI_Screen_Multiplier_Amber": (
                unreal.LinearColor(0.026, 0.008, 0.016, 1.0),
                0.08,
                0.06,
                unreal.LinearColor(0.11, 0.032, 0.055, 1.0),
            ),
            "MI_Emissive_PhosphorCyan": (
                unreal.LinearColor(0.0, 0.96, 0.83, 1.0),
                0.0,
                0.15,
                unreal.LinearColor(0.0, 7.5, 6.5, 1.0),
            ),
            "MI_Emissive_HighVoltageAmber": (
                unreal.LinearColor(1.0, 0.68, 0.02, 1.0),
                0.0,
                0.15,
                unreal.LinearColor(8.5, 5.2, 0.15, 1.0),
            ),
        }

        created_mis = {}
        mic_factory = unreal.MaterialInstanceConstantFactoryNew()
        for mi_name, (bcol, met, rough, ecol) in presets.items():
            mi_path = f"{mat_folder}/{mi_name}"
            if unreal.EditorAssetLibrary.does_asset_exist(mi_path):
                mi = unreal.EditorAssetLibrary.load_asset(mi_path)
            else:
                mi = asset_tools.create_asset(mi_name, mat_folder, unreal.MaterialInstanceConstant, mic_factory)
            mi.set_editor_property("parent", master_mat)
            mel.set_material_instance_vector_parameter_value(mi, "BaseColor", bcol)
            mel.set_material_instance_scalar_parameter_value(mi, "Metallic", met)
            mel.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
            mel.set_material_instance_vector_parameter_value(mi, "EmissiveColor", ecol)
            mel.update_material_instance(mi)
            unreal.EditorAssetLibrary.save_asset(mi_path, only_if_is_dirty=False)
            created_mis[mi_name] = mi

        static_mats = mesh.get_editor_property("static_materials")
        slot_names = []
        for idx, sm in enumerate(static_mats):
            sname = str(sm.get_editor_property("material_slot_name"))
            slot_names.append(sname)
            if sname in created_mis:
                mesh.set_material(idx, created_mis[sname])

        unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)

        # 3. Also place/update the CRT Scoreboard in the open level (/Game/Maps/L_TestingPit) right now
        # so the user can see it immediately in the Editor viewport without waiting for a C++ restart!
        actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        all_actors = actor_sub.get_all_level_actors()

        # Remove any previous standalone preview scoreboard actors to avoid duplicates
        for a in all_actors:
            lbl = a.get_actor_label()
            if lbl.startswith("CRT_Scoreboard_"):
                actor_sub.destroy_actor(a)

        # Find table origin (defaults to 0,0,0)
        table_actors = [a for a in actor_sub.get_all_level_actors() if a.get_class().get_name() == "CrapsTableGrayboxActor"]
        table_origin = table_actors[0].get_actor_location() if table_actors else unreal.Vector(0.0, 0.0, 0.0)

        sb_loc = unreal.Vector(table_origin.x + 1565.0, table_origin.y + 0.0, table_origin.z + 152.0)
        sb_rot = unreal.Rotator(roll=0.0, pitch=-8.0, yaw=180.0)

        sb_actor = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor, sb_loc, sb_rot)
        sb_actor.set_actor_label("CRT_Scoreboard_Enclosure")
        sb_actor.set_editor_property("tags", [unreal.Name("CRT_Scoreboard_Enclosure")])
        sm_comp = sb_actor.get_editor_property("static_mesh_component")
        sm_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        sm_comp.set_static_mesh(mesh)
        for idx, sm in enumerate(static_mats):
            sname = str(sm.get_editor_property("material_slot_name"))
            if sname in created_mis:
                sm_comp.set_material(idx, created_mis[sname])

        # Helper to spawn a TextRenderActor attached to sb_actor in local space
        def spawn_screen_text(label_name, local_xyz, text_str, world_size, color_rgba):
            ta = actor_sub.spawn_actor_from_class(unreal.TextRenderActor, sb_loc, sb_rot)
            ta.set_actor_label(label_name)
            ta.set_editor_property("tags", [unreal.Name(label_name)])
            tr = ta.get_editor_property("text_render")
            tr.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            tr.set_text(unreal.Text(text_str))
            tr.set_world_size(world_size)
            tr.set_text_render_color(unreal.Color(r=color_rgba[0], g=color_rgba[1], b=color_rgba[2], a=255))
            tr.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
            tr.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
            ta.attach_to_actor(sb_actor, "", unreal.AttachmentRule.KEEP_RELATIVE, unreal.AttachmentRule.KEEP_RELATIVE, unreal.AttachmentRule.KEEP_RELATIVE, False)
            ta.set_actor_relative_location(unreal.Vector(local_xyz[0], local_xyz[1], local_xyz[2]), False, False)
            ta.set_actor_relative_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0), False, False)
            return ta

        cyan = (0, 245, 212)
        amber = (255, 183, 3)
        magenta = (255, 42, 133)
        warm = (210, 175, 110)

        # Compartment 1: BANKROLL (Top-Left to Viewer -> Local Y = +81.0, Z = 99.0)
        spawn_screen_text("CRT_Scoreboard_Hdr_Bankroll", (15.2, 81.0, 113.5), "BANKROLL // SAFE CREDITS", 7.5, cyan)
        spawn_screen_text("CRT_Scoreboard_Val_Bankroll", (15.5, 81.0, 94.0), "$ 500", 24.0, cyan)

        # Compartment 2: ANTE QUOTA (Top-Right to Viewer -> Local Y = -83.0, Z = 98.5)
        spawn_screen_text("CRT_Scoreboard_Hdr_Ante", (15.2, -83.0, 112.5), "PIT BOSS ANTE // PUCK OFF", 7.2, amber)
        spawn_screen_text("CRT_Scoreboard_Val_Ante", (15.5, -83.0, 93.5), "$ 2,500", 22.0, amber)

        # Compartment 3: UNBANKED POOL (Bottom-Left to Viewer -> Local Y = +96.0, Z = 40.0)
        spawn_screen_text("CRT_Scoreboard_Hdr_Unbanked", (15.2, 96.0, 53.0), "UNBANKED POOL [AT RISK]", 6.5, cyan)
        spawn_screen_text("CRT_Scoreboard_Val_Unbanked", (15.5, 96.0, 35.5), "+ 0", 20.5, cyan)

        # Compartment 4: MULTIPLIER CHAIN (Bottom-Right to Viewer -> Local Y = -68.0, Z = 40.0)
        spawn_screen_text("CRT_Scoreboard_Hdr_Mult", (15.2, -68.0, 53.0), "CHIPS 0  x  THROW 1.0x  x  BET 1.0x", 6.5, warm)
        spawn_screen_text("CRT_Scoreboard_Val_Mult", (15.5, -68.0, 35.5), "1.0x MULT", 21.0, magenta)

        # Spawn a warm casino key light right in front of the scoreboard so the tarnished brass & oxblood pop in Lumen!
        light_loc = unreal.Vector(table_origin.x + 1410.0, table_origin.y + 0.0, table_origin.z + 265.0)
        light_actor = actor_sub.spawn_actor_from_class(unreal.PointLight, light_loc, unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
        light_actor.set_actor_label("CRT_Scoreboard_KeyLight")
        light_actor.set_editor_property("tags", [unreal.Name("CRT_Scoreboard_KeyLight")])
        plc = light_actor.get_editor_property("point_light_component")
        plc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        plc.set_intensity(4500.0)
        plc.set_attenuation_radius(650.0)
        plc.set_light_color(unreal.LinearColor(1.0, 0.78, 0.48, 1.0))

        level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        level_sub.save_current_level()

        bounds = mesh.get_bounds()
        sm_sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        result = {
            "imported": True,
            "asset_path": asset_path,
            "num_vertices": sm_sub.get_number_verts(mesh, 0) if sm_sub else -1,
            "num_triangles": mesh.get_num_triangles(0),
            "num_sections": mesh.get_num_sections(0),
            "material_slots": slot_names,
            "assigned_materials": list(created_mis.keys()),
            "spawned_in_level": sb_actor.get_path_name(),
            "scoreboard_world_location": [sb_loc.x, sb_loc.y, sb_loc.z],
            "box_extent": [round(bounds.box_extent.x, 2), round(bounds.box_extent.y, 2), round(bounds.box_extent.z, 2)],
            "origin": [round(bounds.origin.x, 2), round(bounds.origin.y, 2), round(bounds.origin.z, 2)],
        }
    else:
        result = {"imported": False, "error": "load_asset returned None"}
except Exception as e:
    result = {"imported": False, "error": str(e), "traceback": traceback.format_exc()}
"""

if __name__ == "__main__":
    resp = server.ue_execute_python(UE_SCRIPT, "Import & Spawn SM_CRT_Scoreboard_v1", use_scoped_transaction=False)
    print(json.dumps(resp, indent=2))
