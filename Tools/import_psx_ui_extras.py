import unreal


ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "SourceArt/UI/")
SOURCES = [
    ("T_QuotaTextContainer.png", "T_QuotaTextContainer"),
    ("T_HealthBackdrop.png", "T_HealthBackdrop"),
]

tasks = []
for filename, asset_name in SOURCES:
    task = unreal.AssetImportTask()
    task.filename = ROOT + filename
    task.destination_path = "/Game/ChopIt/Art/UI"
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = False
    task.save = True
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for _, asset_name in SOURCES:
    asset_path = "/Game/ChopIt/Art/UI/" + asset_name
    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if texture is None:
        raise RuntimeError("No se pudo importar " + asset_path)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)

unreal.log("IMPORT_PSX_UI_EXTRAS_OK")
