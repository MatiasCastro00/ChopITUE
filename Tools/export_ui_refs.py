import unreal


def export_texture(asset_path, filename):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError("Missing texture: " + asset_path)
    task = unreal.AssetExportTask()
    task.object = asset
    task.filename = unreal.Paths.convert_relative_path_to_full(
        unreal.Paths.project_saved_dir() + "UIReferences/" + filename
    )
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError("Could not export: " + asset_path)


export_texture("/Game/ChopIt/Art/MockUp/Mockup", "Mockup.png")
export_texture("/Game/ChopIt/Art/UI/UI_PSX_Atlas_Transparente", "UI_PSX_Atlas_Transparente.png")
unreal.log("CHOPIT_UI_REFERENCES_EXPORTED")
