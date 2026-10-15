"""Read-only verification that the CRT effect is persisted in L_PSX_test."""

import unreal

ROOT = "/Game/NewChopIt/Art/Materials/PSX_Materials"
material = unreal.EditorAssetLibrary.load_asset(ROOT + "/M_PSX_CRT_Post")
instance = unreal.EditorAssetLibrary.load_asset(ROOT + "/MI_PSX_CRT")
assert isinstance(material, unreal.Material)
assert isinstance(instance, unreal.MaterialInstanceConstant)
assert material.get_editor_property("material_domain") == unreal.MaterialDomain.MD_POST_PROCESS
assert material.get_editor_property("blendable_priority") == 10
assert instance.get_editor_property("parent") == material
assert isinstance(unreal.MaterialEditingLibrary.get_material_property_input_node(
    material, unreal.MaterialProperty.MP_EMISSIVE_COLOR), unreal.MaterialExpressionCustom)
unreal.MaterialEditingLibrary.recompile_material(material)

expected = {"EffectStrength", "ScanlineStrength", "ScanlineCount",
            "VignetteStrength", "ChromaticStrength", "ColorOffsetPixels",
            "ScanlineSpeed", "RefreshStrength", "RefreshSpeed",
            "ScreenCurvature", "BorderWidth", "CornerRadius", "HorizontalWobblePixels"}
assert expected == set(map(str, unreal.MaterialEditingLibrary.get_scalar_parameter_names(material)))
assert len([node for node in unreal.MaterialEditingLibrary.get_material_expressions(material)
            if isinstance(node, unreal.MaterialExpressionCustom)]) == 2
nodes = unreal.MaterialEditingLibrary.get_material_expressions(material)
warp = next(node for node in nodes if isinstance(node, unreal.MaterialExpressionCustom)
            and "HorizontalWobblePixels" in node.get_editor_property("code"))
assert "1.0 + 2.0 * curvature" in warp.get_editor_property("code")
scalar_nodes = {str(node.get_editor_property("parameter_name")): node for node in nodes
                if isinstance(node, unreal.MaterialExpressionScalarParameter)}
assert abs(scalar_nodes["ScreenCurvature"].get_editor_property("default_value") - 0.02) < 0.0001
assert abs(scalar_nodes["BorderWidth"].get_editor_property("default_value") - 0.025) < 0.0001
overrides = [(str(item.get_editor_property("parameter_info").get_editor_property("name")),
              item.get_editor_property("parameter_value"))
             for item in instance.get_editor_property("scalar_parameter_values")]
unreal.log("PSX_CRT_OVERRIDES {}".format(overrides))

world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/NewChopIt/Maps/L_PSX_test")
assert world
volumes = [actor for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
           if isinstance(actor, unreal.PostProcessVolume)]
assert len(volumes) == 1
volume = volumes[0]
entries = volume.get_editor_property("settings").get_editor_property(
    "weighted_blendables").get_editor_property("array")
crt_entries = [entry for entry in entries if entry.get_editor_property("object") == instance]
assert len(crt_entries) == 1
assert crt_entries[0].get_editor_property("weight") == 1.0
assert volume.get_editor_property("unbound")
unreal.log("PSX_CRT_VERIFY_OK volume={} blendables={} parameters={}".format(
    volume.get_actor_label(), len(entries), sorted(expected)))
