"""Create an editable CRT post-process pass and attach it only to L_PSX_test.

Run in Unreal Editor with Python enabled, or with UnrealEditor-Cmd.exe
ChopIt.uproject -run=pythonscript -script=Tools/install_psx_crt.py.
Running it again keeps existing material-instance parameter edits.
"""

import unreal

MAP_PATH = "/Game/ChopIt/World/Maps/L_PSX_test"
ROOT = "/Game/ChopIt/Art/Materials/PSX_Materials"
MATERIAL_PATH = ROOT + "/M_PSX_CRT_Post"
INSTANCE_PATH = ROOT + "/MI_PSX_CRT"
MEL = unreal.MaterialEditingLibrary
SAFE_WARP_CODE = """
float2 p = UV * 2.0 - 1.0;
float curvature = clamp(ScreenCurvature, 0.0, 0.12);
float radial = dot(p, p);
// Normalize at the screen corners so sampling stays inside the viewport.
float2 curved = 0.5 + (UV - 0.5) * ((1.0 + curvature * radial) / (1.0 + 2.0 * curvature));
float wobble = sin(UV.y * 22.0 + Time * 0.8) * sin(UV.y * 7.0 - Time * 0.31);
curved.x += wobble * HorizontalWobblePixels * InvSize.x;
return lerp(UV, curved, saturate(EffectStrength));
"""


def expression(material, kind, x, y, **properties):
    result = MEL.create_material_expression(material, getattr(unreal, "MaterialExpression" + kind), x, y)
    for name, value in properties.items():
        result.set_editor_property(name, value)
    return result


def link(source, output, target, input_name):
    if not MEL.connect_material_expressions(source, output, target, input_name):
        raise RuntimeError("Could not connect {} to {}.{}".format(source.get_name(), target.get_name(), input_name))


def parameter(material, name, value, y):
    return expression(material, "ScalarParameter", -600, y,
                      parameter_name=name, default_value=value, group="CRT")


def add_custom_inputs(custom, sources):
    pins = list(custom.get_editor_property("inputs"))
    existing = {str(pin.get_editor_property("input_name")) for pin in pins}
    for name in sources:
        if name not in existing:
            pin = unreal.CustomInput()
            pin.set_editor_property("input_name", name)
            pins.append(pin)
    custom.set_editor_property("inputs", pins)
    for name, (source, output) in sources.items():
        link(source, output, custom, name)


def add_motion_and_frame(material):
    nodes = MEL.get_material_expressions(material)
    scalars = {str(node.get_editor_property("parameter_name")): node for node in nodes
               if isinstance(node, unreal.MaterialExpressionScalarParameter)}
    if "ScreenCurvature" in scalars:
        warped = [node for node in nodes if isinstance(node, unreal.MaterialExpressionCustom)
                  and "HorizontalWobblePixels" in node.get_editor_property("code")]
        if len(warped) != 1:
            raise RuntimeError("Could not identify the CRT curvature node")
        old_code = warped[0].get_editor_property("code")
        if "max(ScreenCurvature, 0.0) * dot(p, p)" in old_code:
            warped[0].set_editor_property("code", SAFE_WARP_CODE)
            scalars["ScreenCurvature"].set_editor_property("default_value", 0.02)
            scalars["BorderWidth"].set_editor_property("default_value", 0.025)
            MEL.recompile_material(material)
            if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
                raise RuntimeError("Could not save corrected CRT material")
        return

    uv_nodes = [node for node in nodes if isinstance(node, unreal.MaterialExpressionScreenPosition)]
    scene_nodes = [node for node in nodes if isinstance(node, unreal.MaterialExpressionSceneTexture)]
    add_nodes = [node for node in nodes if isinstance(node, unreal.MaterialExpressionAdd)]
    subtract_nodes = [node for node in nodes if isinstance(node, unreal.MaterialExpressionSubtract)]
    if len(uv_nodes) != 1 or len(scene_nodes) != 3 or len(add_nodes) != 1 or len(subtract_nodes) != 1:
        raise RuntimeError("Unexpected CRT material graph; inspect it before upgrading")
    uv = uv_nodes[0]
    center, red, blue = sorted(
        scene_nodes, key=lambda node: node.get_editor_property("material_expression_editor_y"))
    color = MEL.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if not isinstance(color, unreal.MaterialExpressionCustom):
        raise RuntimeError("Unexpected CRT output node")

    clock = expression(material, "Time", -1150, 650)
    scene_size = expression(material, "SceneTexture", -1100, -850,
                            scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    new_params = {
        "ScanlineSpeed": parameter(material, "ScanlineSpeed", 0.4, 1850),
        "RefreshStrength": parameter(material, "RefreshStrength", 0.05, 2000),
        "RefreshSpeed": parameter(material, "RefreshSpeed", 0.18, 2150),
        "ScreenCurvature": parameter(material, "ScreenCurvature", 0.02, 2300),
        "BorderWidth": parameter(material, "BorderWidth", 0.025, 2450),
        "CornerRadius": parameter(material, "CornerRadius", 0.11, 2600),
        "HorizontalWobblePixels": parameter(material, "HorizontalWobblePixels", 0.35, 2750),
    }
    warped_uv = expression(material, "Custom", -240, -850,
                           output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT2,
                           code=SAFE_WARP_CODE)
    add_custom_inputs(warped_uv, {
        "UV": (uv, "ViewportUV"),
        "InvSize": (scene_size, "InvSize"),
        "Time": (clock, ""),
        "EffectStrength": (scalars["EffectStrength"], ""),
        "ScreenCurvature": (new_params["ScreenCurvature"], ""),
        "HorizontalWobblePixels": (new_params["HorizontalWobblePixels"], ""),
    })
    link(warped_uv, "", center, "UVs")
    link(warped_uv, "", add_nodes[0], "A")
    link(warped_uv, "", subtract_nodes[0], "A")

    color.set_editor_property("code", """
float phase = UV.y * max(ScanlineCount, 1.0) * 6.2831853 + Time * ScanlineSpeed * 6.2831853;
float scan = 0.5 + 0.5 * cos(phase);
float scanGain = 1.0 - saturate(ScanlineStrength) * scan;
float beamY = frac(Time * max(RefreshSpeed, 0.0));
float beamDistance = abs(UV.y - beamY);
beamDistance = min(beamDistance, 1.0 - beamDistance);
float beam = exp(-beamDistance * beamDistance * 900.0);
float refreshGain = 1.0 + max(RefreshStrength, 0.0) * beam;
float2 centered = (UV - 0.5) * 2.0;
float edge = saturate(dot(centered, centered) * 0.5);
float vignetteGain = 1.0 - saturate(VignetteStrength) * edge;
float3 separated = float3(RedSample.r, CenterSample.g, BlueSample.b);
float3 chromatic = lerp(CenterSample.rgb, separated, saturate(ChromaticStrength));
float3 image = chromatic * scanGain * refreshGain * vignetteGain;
float radius = clamp(CornerRadius, 0.001, 0.45);
float2 bounds = float2(1.0 - 2.0 * saturate(BorderWidth), 1.0 - 2.0 * saturate(BorderWidth));
float2 d = abs(centered) - (bounds - radius);
float sdf = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
float screenMask = 1.0 - smoothstep(-0.008, 0.008, sdf);
float3 frame = float3(0.012, 0.014, 0.018);
return lerp(CenterSample.rgb, lerp(frame, image, screenMask), saturate(EffectStrength));
""")
    add_custom_inputs(color, {
        "Time": (clock, ""),
        "ScanlineSpeed": (new_params["ScanlineSpeed"], ""),
        "RefreshStrength": (new_params["RefreshStrength"], ""),
        "RefreshSpeed": (new_params["RefreshSpeed"], ""),
        "BorderWidth": (new_params["BorderWidth"], ""),
        "CornerRadius": (new_params["CornerRadius"], ""),
    })
    MEL.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError("Could not save updated CRT material")


material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_PSX_CRT_Post", ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Could not create CRT material")
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    material.set_editor_property("blendable_priority", 10)

    uv = expression(material, "ScreenPosition", -1100, -500)
    center = expression(material, "SceneTexture", -850, -500,
                        scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    red = expression(material, "SceneTexture", -850, -180,
                     scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    blue = expression(material, "SceneTexture", -850, 140,
                      scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    offset = parameter(material, "ColorOffsetPixels", 0.6, 850)
    horizontal = expression(material, "Constant2Vector", -850, 850,
                            r=1.0, g=0.0)
    pixels_to_uv = expression(material, "Multiply", -620, 700)
    horizontal_offset = expression(material, "Multiply", -430, 700)
    red_uv = expression(material, "Add", -240, -130)
    blue_uv = expression(material, "Subtract", -240, 160)
    link(center, "InvSize", pixels_to_uv, "A")
    link(offset, "", pixels_to_uv, "B")
    link(pixels_to_uv, "", horizontal_offset, "A")
    link(horizontal, "", horizontal_offset, "B")
    link(uv, "ViewportUV", red_uv, "A")
    link(horizontal_offset, "", red_uv, "B")
    link(uv, "ViewportUV", blue_uv, "A")
    link(horizontal_offset, "", blue_uv, "B")
    link(red_uv, "", red, "UVs")
    link(blue_uv, "", blue, "UVs")

    params = {
        "EffectStrength": parameter(material, "EffectStrength", 1.0, 1050),
        "ScanlineStrength": parameter(material, "ScanlineStrength", 0.12, 1200),
        "ScanlineCount": parameter(material, "ScanlineCount", 360.0, 1350),
        "VignetteStrength": parameter(material, "VignetteStrength", 0.16, 1500),
        "ChromaticStrength": parameter(material, "ChromaticStrength", 0.35, 1650),
    }
    custom = expression(material, "Custom", 100, -380,
                        output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                        code="""
float scan = 0.5 + 0.5 * cos(UV.y * max(ScanlineCount, 1.0) * 6.2831853);
float scanGain = 1.0 - saturate(ScanlineStrength) * scan;
float2 centered = (UV - 0.5) * 2.0;
float edge = saturate(dot(centered, centered) * 0.5);
float vignetteGain = 1.0 - saturate(VignetteStrength) * edge;
float3 separated = float3(RedSample.r, CenterSample.g, BlueSample.b);
float3 chromatic = lerp(CenterSample.rgb, separated, saturate(ChromaticStrength));
return lerp(CenterSample.rgb, chromatic * scanGain * vignetteGain, saturate(EffectStrength));
""")
    sources = {"UV": (uv, "ViewportUV"), "CenterSample": (center, "Color"),
               "RedSample": (red, "Color"), "BlueSample": (blue, "Color")}
    sources.update({name: (node, "") for name, node in params.items()})
    pins = []
    for name in sources:
        pin = unreal.CustomInput()
        pin.set_editor_property("input_name", name)
        pins.append(pin)
    custom.set_editor_property("inputs", pins)
    for name, (source, output) in sources.items():
        link(source, output, custom, name)
    if not MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Could not connect CRT output")
    MEL.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError("Could not save CRT material")
elif not isinstance(material, unreal.Material):
    raise RuntimeError("CRT material path is occupied by a different asset")

add_motion_and_frame(material)

instance = unreal.EditorAssetLibrary.load_asset(INSTANCE_PATH)
if instance is None:
    instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "MI_PSX_CRT", ROOT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if not isinstance(instance, unreal.MaterialInstanceConstant):
        raise RuntimeError("Could not create CRT instance")
    MEL.set_material_instance_parent(instance, material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False):
        raise RuntimeError("Could not save CRT instance")
elif not isinstance(instance, unreal.MaterialInstanceConstant):
    raise RuntimeError("CRT instance path is occupied by a different asset")

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
if not world:
    raise RuntimeError("Could not load L_PSX_test")
volumes = [actor for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
           if isinstance(actor, unreal.PostProcessVolume)]
if len(volumes) != 1:
    raise RuntimeError("Expected one post-process volume in L_PSX_test, found {}".format(len(volumes)))
volume = volumes[0]
settings = volume.get_editor_property("settings")
blendables = settings.get_editor_property("weighted_blendables")
entries = list(blendables.get_editor_property("array"))
if not any(entry.get_editor_property("object") and
           entry.get_editor_property("object").get_path_name().split(".")[0] == INSTANCE_PATH
           for entry in entries):
    entry = unreal.WeightedBlendable()
    entry.set_editor_property("weight", 1.0)
    entry.set_editor_property("object", instance)
    entries.append(entry)
    blendables.set_editor_property("array", entries)
    settings.set_editor_property("weighted_blendables", blendables)
    volume.set_editor_property("settings", settings)
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
        raise RuntimeError("Could not save L_PSX_test")

unreal.log("PSX_CRT_OK map={} instance={} parameters={}".format(
    MAP_PATH, INSTANCE_PATH, MEL.get_scalar_parameter_names(material)))
