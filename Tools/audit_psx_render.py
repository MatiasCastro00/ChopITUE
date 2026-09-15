"""Read-only audit of every render layer that can remove texture color in L_PSX_test."""

import unreal

MAP_PATH = "/Game/ChopIt/World/Maps/L_PSX_test"
CHECK_LABELS = {"CabinHub", "QuotaMachine"}


def path(obj):
    return obj.get_path_name().split(".")[0] if obj else "None"


def describe_material(prefix, material_interface):
    if not material_interface:
        unreal.log("PSX_AUDIT {} material=None".format(prefix))
        return
    base = material_interface.get_base_material()
    base_node = unreal.MaterialEditingLibrary.get_material_property_input_node(
        base, unreal.MaterialProperty.MP_BASE_COLOR
    )
    emissive_node = unreal.MaterialEditingLibrary.get_material_property_input_node(
        base, unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    textures = [path(texture) for texture in unreal.MaterialEditingLibrary.get_material_used_textures(material_interface)]
    unreal.log(
        "PSX_AUDIT {} material={} base={} domain={} blend={} base_node={} emissive_node={} textures={}".format(
            prefix,
            path(material_interface),
            path(base),
            base.get_editor_property("material_domain"),
            base.get_editor_property("blend_mode"),
            base_node.get_class().get_name() if base_node else "None",
            emissive_node.get_class().get_name() if emissive_node else "None",
            textures,
        )
    )
    if isinstance(material_interface, unreal.MaterialInstanceConstant):
        for name in unreal.MaterialEditingLibrary.get_scalar_parameter_names(base):
            value = unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(material_interface, name)
            unreal.log("PSX_AUDIT {} scalar {}={}".format(prefix, name, value))
        for name in unreal.MaterialEditingLibrary.get_vector_parameter_names(base):
            value = unreal.MaterialEditingLibrary.get_material_instance_vector_parameter_value(material_interface, name)
            unreal.log("PSX_AUDIT {} vector {}={}".format(prefix, name, value))


world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
assert world
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
for actor in actors:
    label = actor.get_actor_label()
    if label not in CHECK_LABELS and not label.startswith("Tree_") and label != "Ground":
        continue
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh = component.get_editor_property("static_mesh")
        if not mesh:
            continue
        unreal.log(
            "PSX_AUDIT actor={} component={} mesh={} slots={} overlay={}".format(
                label, component.get_name(), path(mesh), component.get_num_materials(), path(component.get_overlay_material())
            )
        )
        for index in range(component.get_num_materials()):
            describe_material("{}.slot{}".format(label, index), component.get_material(index))
        describe_material("{}.overlay".format(label), component.get_overlay_material())

for actor in actors:
    if actor.get_class().get_name() != "PostProcessVolume":
        continue
    settings = actor.get_editor_property("settings")
    unreal.log(
        "PSX_AUDIT pp saturation_override={} saturation={} scene_tint={} grading_intensity={}".format(
            settings.get_editor_property("override_color_saturation"),
            settings.get_editor_property("color_saturation"),
            settings.get_editor_property("scene_color_tint"),
            settings.get_editor_property("color_grading_intensity"),
        )
    )
    entries = settings.get_editor_property("weighted_blendables").get_editor_property("array")
    for index, entry in enumerate(entries):
        describe_material("postprocess.{} weight={}".format(index, entry.get_editor_property("weight")), entry.get_editor_property("object"))

unreal.log("PSX_AUDIT_COMPLETE")
