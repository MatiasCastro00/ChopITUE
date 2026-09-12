import unreal

asset = unreal.load_asset("/Game/ChopIt/UI/WBP_PSX_HUD")
if not asset:
    raise RuntimeError("WBP_PSX_HUD is missing")
if asset.get_editor_property("status") != unreal.BlueprintStatus.BS_UP_TO_DATE:
    raise RuntimeError("WBP_PSX_HUD did not compile: %s" % asset.get_editor_property("status"))

world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/ChopIt/World/Maps/L_PSX_test")
if not world:
    raise RuntimeError("L_PSX_test could not be loaded")
settings = world.get_world_settings()
unreal.log("VERIFY_PSX_UI asset=%s status=%s map=%s game_mode=%s" % (
    asset.get_path_name(), asset.get_editor_property("status"), world.get_path_name(),
    settings.get_editor_property("default_game_mode")
))
