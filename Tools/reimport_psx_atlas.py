import unreal


SOURCE = unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_dir() + "SourceArt/UI/UI_PSX_Atlas_Transparente.png"
)
ASSET = "/Game/ChopIt/Art/UI/UI_PSX_Atlas_Transparente"

task = unreal.AssetImportTask()
task.filename = SOURCE
task.destination_path = "/Game/ChopIt/Art/UI"
task.destination_name = "UI_PSX_Atlas_Transparente"
task.automated = True
task.replace_existing = True
task.replace_existing_settings = False
task.save = True

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.EditorAssetLibrary.load_asset(ASSET)
if texture is None:
    raise RuntimeError("No se pudo cargar el atlas reimportado: " + ASSET)

# Mantiene el atlas apto para UI pixel-art y preserva el canal alfa nuevo.
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property("srgb", True)
unreal.EditorAssetLibrary.save_loaded_asset(texture)

unreal.log("REIMPORT_PSX_ATLAS_OK source={} asset={}".format(SOURCE, ASSET))
