import unreal

path = "/Game/ChopIt/UI/WBP_PSX_HUD"
if unreal.EditorAssetLibrary.does_asset_exist(path):
    if not unreal.EditorAssetLibrary.delete_asset(path):
        raise RuntimeError("Could not reset " + path)
unreal.log("CHOPIT_PSX_UI_RESET")
