"""Verify persisted Nanite flags and run Unreal's map check on L_PSX_test."""

import unreal


MAP_PATH = "/Game/ChopIt/World/Maps/L_PSX_test"
MATERIAL_PATHS = (
    "/Game/ChopIt/Art/Models/Trees/New/Tree1/M_Tree_Far_01",
    "/Game/ChopIt/Art/Models/Trees/New/Tree2/M_Tree_Cabin_02",
    "/Game/ChopIt/Art/Models/Trees/New/Tree3/M_Tree_Cabin_03",
    "/Game/ChopIt/Art/Models/SetPieces/House/M_House_PSX_Jittering",
    "/Game/ChopIt/Art/Models/SetPieces/Machine/M_Machine_PSX_Jittering",
    "/Game/ChopIt/Art/Materials/Ground/M_Grass_Ground_01_PSX_Tiled",
    "/Game/ChopIt/Art/Materials/Ground/M_Grass_Ground_02_PSX_Tiled",
    "/Game/ChopIt/Art/Materials/Ground/M_Grass_Ground_03_PSX_Tiled",
)


for path in MATERIAL_PATHS:
    material = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Missing material " + path)
    assert material.get_editor_property("used_with_nanite"), "Nanite flag was not saved: " + path

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
if not world:
    raise RuntimeError("Could not load PSX map")
unreal.SystemLibrary.execute_console_command(world, "MAP CHECKDEP NOCLEARLOG")
unreal.log("PSX_MATERIAL_MAPCHECK_TRIGGERED nanite_materials={}".format(len(MATERIAL_PATHS)))
