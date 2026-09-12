import unreal


SOURCE_FBX = unreal.Paths.project_content_dir() + "ChopIt/Art/Character/LumberJack.fbx"
DESTINATION = "/Game/ChopIt/Art/Character/Updated"

options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_animations", True)
options.set_editor_property("import_materials", False)
options.set_editor_property("import_textures", False)
options.set_editor_property("import_as_skeletal", True)
options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
options.skeletal_mesh_import_data.set_editor_property("import_morph_targets", True)
options.anim_sequence_import_data.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)

task = unreal.AssetImportTask()
task.set_editor_property("filename", SOURCE_FBX)
task.set_editor_property("destination_path", DESTINATION)
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("replace_existing_settings", True)
task.set_editor_property("save", True)
task.set_editor_property("options", options)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

assets = unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=True, include_folder=False)
if not assets:
    raise RuntimeError("The Lumberjack FBX import produced no assets")

for asset_path in sorted(assets):
    unreal.log_warning("LUMBERJACK_UPDATED_ASSET: " + asset_path)

unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
unreal.log_warning("LUMBERJACK_UPDATE_IMPORT_OK")
