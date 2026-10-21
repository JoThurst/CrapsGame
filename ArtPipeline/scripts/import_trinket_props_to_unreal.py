import unreal
import os

print("--- STARTING TRINKET PROPS IMPORT ---")
dest_path = "/Game/Meshes"

props = [
    ("SM_RailProp_WhiskeyTumbler", r"C:/Code/Unreal Projects/Craps/CrapsCrapsCraps/ArtPipeline/exports/SM_RailProp_WhiskeyTumbler.fbx"),
    ("SM_TrinketRing_Signet", r"C:/Code/Unreal Projects/Craps/CrapsCrapsCraps/ArtPipeline/exports/SM_TrinketRing_Signet.fbx")
]

for dest_name, fbx_path in props:
    asset_path = f"{dest_path}/{dest_name}"
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
        bounds = mesh.get_bounds()
        print(f"SUCCESS: {dest_name} imported! Origin={bounds.origin}, Extent={bounds.box_extent}, Radius={bounds.sphere_radius}")
    else:
        print(f"FAILED to import {dest_name}")
