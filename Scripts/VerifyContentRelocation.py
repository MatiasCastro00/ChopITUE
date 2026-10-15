"""Unreal Editor Python smoke check for relocated gameplay content."""

import unreal
from pathlib import Path

report_path = Path(unreal.Paths.project_saved_dir()) / "RelocationReport.txt"
report_path.unlink(missing_ok=True)

ASSETS = [
    "/Game/NewChopIt/Art/UI/WBP_PSX_HUD",
    "/Game/NewChopIt/Characters/Blueprints/BP_ChopItCharacter",
    "/Game/NewChopIt/Input/IA_Move",
    "/Game/NewChopIt/Input/IA_CameraLook",
    "/Game/NewChopIt/Input/IA_CameraZoom",
    "/Game/NewChopIt/Input/IA_CameraReset",
    "/Game/NewChopIt/Input/IMC_Gameplay",
    "/Game/NewChopIt/Camera/CA_PlayerCameras",
    "/Game/NewChopIt/Camera/ST_CameraDirector",
]

report = []
for path in ASSETS:
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError("Missing relocated asset: " + path)
    report.append("OK " + asset.get_path_name())

world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/NewChopIt/Maps/L_Startup")
if not world:
    raise RuntimeError("Startup map did not load")
report.append("OK startup map " + world.get_path_name())

for old_path, expected_path in (
    ("/Game/ChopIt/UI/WBP_PSX_HUD", "/Game/NewChopIt/Art/UI/WBP_PSX_HUD"),
    ("/Game/ChopIt/Input/IA_Move", "/Game/NewChopIt/Input/IA_Move"),
    ("/Game/ChopIt/Presentation/Camera/CA_PlayerCameras", "/Game/NewChopIt/Camera/CA_PlayerCameras"),
):
    redirected = unreal.load_asset(old_path)
    if not redirected or not redirected.get_path_name().startswith(expected_path + "."):
        raise RuntimeError("Broken package redirect: " + old_path)
    report.append("OK redirect " + old_path + " -> " + redirected.get_path_name())

widget_class = unreal.load_class(None, "/Game/NewChopIt/Art/UI/WBP_PSX_HUD.WBP_PSX_HUD_C")
if not widget_class:
    raise RuntimeError("New HUD widget class did not load")
report.append("OK HUD class " + widget_class.get_path_name())
for old_class_path in (
    "/Game/ChopIt/UI/WBP_PSX_HUD.WBP_PSX_HUD_C",
    "/Game/ChopIt/Characters/Blueprints/BP_ChopItCharacter.BP_ChopItCharacter_C",
):
    old_class = unreal.load_class(None, old_class_path)
    if not old_class:
        raise RuntimeError("Broken Blueprint class redirect: " + old_class_path)
    report.append("OK Blueprint class redirect " + old_class.get_path_name())

context = unreal.load_asset("/Game/NewChopIt/Input/IMC_Gameplay")
raw_mappings = context.get_editor_property("default_key_mappings").get_editor_property("mappings")
mapped_actions = set()
for mapping in raw_mappings:
    action = mapping.get_editor_property("action")
    report.append("MAPPING " + (action.get_path_name() if action else "<none>"))
    if action:
        mapped_actions.add(action.get_name())
report_path.write_text("\n".join(report) + "\n", encoding="utf-8")
for required_action in ("IA_Move", "IA_CameraLook", "IA_CameraZoom", "IA_CameraReset"):
    if required_action not in mapped_actions:
        raise RuntimeError("Gameplay mapping missing " + required_action)
report.append("OK gameplay mappings " + ", ".join(sorted(mapped_actions)))
report_path.write_text("\n".join(report) + "\n", encoding="utf-8")
unreal.log("RELOCATION REPORT " + str(report_path))
