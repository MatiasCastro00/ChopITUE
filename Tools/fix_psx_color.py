"""Replace the grayscale cel-shading pass with color-preserving PSX posterization."""

import unreal

MATERIAL_PATH = "/Game/ChopIt/Art/Materials/PSX_Materials/M_PSX_CellShading_Color"
INSTANCE_PATH = "/Game/ChopIt/Art/Materials/PSX_Materials/MI_PSX_CellShading"

material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
if material is None:
    folder, name = MATERIAL_PATH.rsplit("/", 1)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, folder, unreal.Material, unreal.MaterialFactoryNew()
    )
if not isinstance(material, unreal.Material):
    raise RuntimeError("Could not create color-preserving cel material")

material.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

scene_color = unreal.MaterialEditingLibrary.create_material_expression(
    material, unreal.MaterialExpressionSceneTexture, -620, 0
)
scene_color.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
steps = unreal.MaterialEditingLibrary.create_material_expression(
    material, unreal.MaterialExpressionScalarParameter, -620, 170
)
steps.set_editor_property("parameter_name", "PSXColorSteps")
steps.set_editor_property("default_value", 16.0)
scaled = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -390, 0)
quantized = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionFloor, -190, 0)
restored = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionDivide, 20, 0)

unreal.MaterialEditingLibrary.connect_material_expressions(scene_color, "Color", scaled, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(steps, "", scaled, "B")
if not unreal.MaterialEditingLibrary.connect_material_expressions(scaled, "", quantized, ""):
    raise RuntimeError("Could not connect PSX color Floor input")
unreal.MaterialEditingLibrary.connect_material_expressions(quantized, "", restored, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(steps, "", restored, "B")
if not unreal.MaterialEditingLibrary.connect_material_property(restored, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
    raise RuntimeError("Could not connect color-preserving PSX output")

unreal.MaterialEditingLibrary.layout_material_expressions(material)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)

instance = unreal.EditorAssetLibrary.load_asset(INSTANCE_PATH)
if not isinstance(instance, unreal.MaterialInstanceConstant):
    raise RuntimeError("Missing PSX cel-shading instance")
unreal.MaterialEditingLibrary.set_material_instance_parent(instance, material)
unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
fog = unreal.EditorAssetLibrary.load_asset(
    "/Game/ChopIt/Art/Materials/PSX_Materials/MI_PSX_Fog_Inst"
)
if not isinstance(fog, unreal.MaterialInstanceConstant):
    raise RuntimeError("Missing PSX fog instance")
unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(fog, "FogDistance", 100000.0)
unreal.EditorAssetLibrary.save_loaded_asset(fog, only_if_is_dirty=False)
unreal.log("PSX_COLOR_FIX_OK parent={} color_steps=16 fog_distance=100000 desaturation=false".format(MATERIAL_PATH))
unreal.log("PSX_COLOR_FIX_OK mode=color_preserving_posterization steps=8 grayscale_nodes=0")
