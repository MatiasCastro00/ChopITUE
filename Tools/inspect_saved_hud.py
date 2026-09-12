import unreal
asset = unreal.load_asset('/Game/ChopIt/UI/WBP_PSX_HUD')
task = unreal.AssetExportTask()
task.object = asset
task.filename = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir() + 'HUD_inspect.t3d')
task.automated = True
task.prompt = False
task.replace_identical = True
assert unreal.Exporter.run_asset_export_task(task)
