import unreal
import os

print("--- STARTING CHIP IMPORT ---")
fbx_path = r"C:/Code/Unreal Projects/Craps/CrapsCrapsCraps/ArtPipeline/exports/SM_CasinoChip_v1.fbx"
dest_path = "/Game/Meshes"
dest_name = "SM_CasinoChip_v1"
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
sm_data.set_editor_property("auto_generate_collision", True)
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

if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    print(f"SUCCESS: Imported {asset_path} into Unreal Engine!")
else:
    print(f"FAILED: Could not find imported asset at {asset_path}")
