"""Read-only verification for the PSX gameplay visual replacements."""

import unreal


MAP_PATH = "/Game/ChopIt/World/Maps/L_PSX_test"
OUTLINE_PATH = "/Game/ChopIt/Art/Materials/PSX_Materials/MI_PSX_Outline_Jittering"
SET_PIECES = (
    ("CabinHub", "CabinVisual", "/Game/ChopIt/Art/Models/SetPieces/House/SM_House_PSX", "/Game/ChopIt/Art/Models/SetPieces/House/T_House_BaseColor", "/Game/ChopIt/Art/Models/SetPieces/House/M_House_PSX_Jittering"),
    ("QuotaMachine", "MachineVisual", "/Game/ChopIt/Art/Models/SetPieces/Machine/SM_Machine_PSX", "/Game/ChopIt/Art/Models/SetPieces/Machine/T_Machine_BaseColor", "/Game/ChopIt/Art/Models/SetPieces/Machine/M_Machine_PSX_Jittering"),
)


for label, _, mesh_path, texture_path, material_path in SET_PIECES:
    for path in (mesh_path, texture_path, material_path):
        assert unreal.EditorAssetLibrary.does_asset_exist(path), "Missing {} asset: {}".format(label, path)
    material = unreal.EditorAssetLibrary.load_asset(material_path)
    base = unreal.MaterialEditingLibrary.get_material_property_input_node(material, unreal.MaterialProperty.MP_BASE_COLOR)
    wpo = unreal.MaterialEditingLibrary.get_material_property_input_node(material, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    assert isinstance(base, unreal.MaterialExpressionTextureSample), "{} has no base-color texture".format(label)
    assert base.get_editor_property("texture").get_path_name().split(".")[0] == texture_path
    assert isinstance(wpo, unreal.MaterialExpressionSubtract), "{} has no PSX vertex jitter".format(label)

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
assert world, "Could not load map"
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
by_label = {actor.get_actor_label(): actor for actor in actors}
assert "PSX_House" not in by_label, "Duplicate PSX_House actor still exists"
assert "PSX_Machine" not in by_label, "Duplicate PSX_Machine actor still exists"
for label, component_name, mesh_path, _, material_path in SET_PIECES:
    actor = by_label.get(label)
    assert actor, "Missing gameplay actor " + label
    component = next(
        (item for item in actor.get_components_by_class(unreal.StaticMeshComponent) if item.get_name() == component_name),
        None,
    )
    assert component
    assert component.get_editor_property("static_mesh").get_path_name().split(".")[0] == mesh_path
    assert component.get_num_materials() > 0
    for material_index in range(component.get_num_materials()):
        assert component.get_material(material_index).get_path_name().split(".")[0] == material_path
    assert component.get_overlay_material().get_path_name().split(".")[0] == OUTLINE_PATH
    assert component.get_collision_profile_name() == "BlockAll"
unreal.log("PSX_SETPIECES_VERIFY_OK gameplay_visuals=replaced duplicates=false materials=textured_jittered_outlined map=L_PSX_test")
