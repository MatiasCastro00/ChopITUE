import os
import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceAssets", "VFX")
MESH_PATH = "/Game/ChopIt/Presentation/VFX/Meshes"
MATERIAL_PATH = "/Game/ChopIt/Presentation/VFX/Materials"
TEXTURE_PATH = "/Game/ChopIt/Presentation/VFX/Textures"
NIAGARA_PATH = "/Game/ChopIt/Presentation/VFX/Niagara"
MASK_ASSET_PATH = TEXTURE_PATH + "/T_AxeSlash_BrushMask"
MASK_SOURCE = "/Engine/EngineMaterials/Good64x64TilingNoiseHighFreq_Low.Good64x64TilingNoiseHighFreq_Low"


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def scalar_parameter(material, name, value, x, y):
    node = expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def constant(material, value, x, y):
    node = expression(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", value)
    return node


def connect(source, source_output, target, target_input):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        source, source_output, target, target_input
    ):
        raise RuntimeError(
            "Could not connect {}:{} to {}:{}".format(
                source.get_name(), source_output, target.get_name(), target_input
            )
        )


def binary(material, cls, left, right, x, y):
    node = expression(material, cls, x, y)
    connect(left, "", node, "A")
    connect(right, "", node, "B")
    return node


def ensure_brush_mask():
    mask = unreal.EditorAssetLibrary.load_asset(MASK_ASSET_PATH)
    if mask is None:
        source_mask = unreal.load_asset(MASK_SOURCE)
        if source_mask is None:
            raise RuntimeError("Could not load source mask {}".format(MASK_SOURCE))
        mask = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
            "T_AxeSlash_BrushMask", TEXTURE_PATH, source_mask
        )
        if mask is None:
            raise RuntimeError("Could not create {}".format(MASK_ASSET_PATH))
    if mask is None:
        raise RuntimeError("Could not load {}".format(MASK_ASSET_PATH))
    try:
        mask.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_MASKS
        )
        mask.set_editor_property("srgb", False)
    except Exception as exc:
        unreal.log_warning("Mask texture settings unchanged: {}".format(exc))
    unreal.EditorAssetLibrary.save_loaded_asset(mask, only_if_is_dirty=False)
    return mask


def weighted_mask(material, sample, weights, x, y):
    terms = []
    for index, channel in enumerate(("R", "G", "B", "A")):
        term = expression(material, unreal.MaterialExpressionMultiply, x, y + index * 120)
        connect(sample, channel, term, "A")
        connect(weights, channel, term, "B")
        terms.append(term)
    sum_rg = binary(material, unreal.MaterialExpressionAdd, terms[0], terms[1], x + 190, y + 50)
    sum_ba = binary(material, unreal.MaterialExpressionAdd, terms[2], terms[3], x + 190, y + 280)
    return binary(material, unreal.MaterialExpressionAdd, sum_rg, sum_ba, x + 380, y + 160)


def create_material(name, blend_mode, mask_texture):
    asset_path = "{}/{}".format(MATERIAL_PATH, name)
    material = unreal.EditorAssetLibrary.load_asset(asset_path)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, MATERIAL_PATH, unreal.Material, unreal.MaterialFactoryNew()
        )
    if material is None:
        raise RuntimeError("Could not create {}".format(asset_path))

    material.set_editor_property("blend_mode", blend_mode)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    uv = expression(material, unreal.MaterialExpressionTextureCoordinate, -1900, 0)
    u = expression(material, unreal.MaterialExpressionComponentMask, -1700, -180)
    u.set_editor_property("r", True)
    u.set_editor_property("g", False)
    v = expression(material, unreal.MaterialExpressionComponentMask, -1700, 260)
    v.set_editor_property("r", False)
    v.set_editor_property("g", True)
    connect(uv, "", u, "")
    connect(uv, "", v, "")

    one_minus_u = expression(material, unreal.MaterialExpressionOneMinus, -1500, -40)
    connect(u, "", one_minus_u, "")
    edge_scale = constant(material, 13.0, -1500, -360)
    left_fade = binary(material, unreal.MaterialExpressionMultiply, u, edge_scale, -1300, -300)
    right_fade = binary(material, unreal.MaterialExpressionMultiply, one_minus_u, edge_scale, -1300, -80)
    end_min = binary(material, unreal.MaterialExpressionMin, left_fade, right_fade, -1100, -190)
    end_fade = expression(material, unreal.MaterialExpressionSaturate, -920, -190)
    connect(end_min, "", end_fade, "")

    two = constant(material, 2.0, -1500, 420)
    one = constant(material, 1.0, -1500, 560)
    v_double = binary(material, unreal.MaterialExpressionMultiply, v, two, -1300, 340)
    v_centered = binary(material, unreal.MaterialExpressionSubtract, v_double, one, -1100, 340)
    v_abs = expression(material, unreal.MaterialExpressionAbs, -920, 340)
    connect(v_centered, "", v_abs, "")
    width_triangle = expression(material, unreal.MaterialExpressionOneMinus, -740, 340)
    connect(v_abs, "", width_triangle, "")
    width = scalar_parameter(material, "Width", 1.0, -740, 500)
    widened = binary(material, unreal.MaterialExpressionMultiply, width_triangle, width, -550, 340)
    widened_sat = expression(material, unreal.MaterialExpressionSaturate, -370, 340)
    connect(widened, "", widened_sat, "")
    edge_power = scalar_parameter(material, "EdgePower", 0.72, -550, 520)
    width_mask = expression(material, unreal.MaterialExpressionPower, -170, 360)
    connect(widened_sat, "", width_mask, "Base")
    connect(edge_power, "", width_mask, "Exp")

    reveal = scalar_parameter(material, "Reveal", 1.0, -1300, 720)
    tail = scalar_parameter(material, "Tail", 0.0, -1300, 900)
    softness = constant(material, 42.0, -1100, 1040)
    half = constant(material, 0.5, -1100, 1160)
    reveal_delta = binary(material, unreal.MaterialExpressionSubtract, reveal, u, -1100, 680)
    reveal_scaled = binary(material, unreal.MaterialExpressionMultiply, reveal_delta, softness, -900, 680)
    reveal_bias = binary(material, unreal.MaterialExpressionAdd, reveal_scaled, half, -700, 680)
    reveal_mask = expression(material, unreal.MaterialExpressionSaturate, -500, 680)
    connect(reveal_bias, "", reveal_mask, "")
    tail_delta = binary(material, unreal.MaterialExpressionSubtract, u, tail, -1100, 880)
    tail_scaled = binary(material, unreal.MaterialExpressionMultiply, tail_delta, softness, -900, 880)
    tail_bias = binary(material, unreal.MaterialExpressionAdd, tail_scaled, half, -700, 880)
    tail_mask = expression(material, unreal.MaterialExpressionSaturate, -500, 880)
    connect(tail_bias, "", tail_mask, "")

    mask_scale = scalar_parameter(material, "MaskScale", 1.45, -1900, 1260)
    mask_offset = scalar_parameter(material, "MaskOffset", 0.0, -1900, 1420)
    scaled_uv = binary(material, unreal.MaterialExpressionMultiply, uv, mask_scale, -1700, 1260)
    offset_uv = binary(material, unreal.MaterialExpressionAdd, scaled_uv, mask_offset, -1500, 1260)
    sample = expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -1300, 1260)
    sample.set_editor_property("parameter_name", "BrushMask")
    sample.set_editor_property("texture", mask_texture)
    connect(offset_uv, "", sample, "UVs")
    weights = expression(material, unreal.MaterialExpressionVectorParameter, -1300, 1540)
    weights.set_editor_property("parameter_name", "MaskWeights")
    weights.set_editor_property(
        "default_value", unreal.LinearColor(0.78, 0.12, 0.04, 0.06)
    )
    selected_noise = weighted_mask(material, sample, weights, -1040, 1260)
    breakup = scalar_parameter(material, "Breakup", 0.30, -650, 1510)
    noise_delta = binary(material, unreal.MaterialExpressionSubtract, selected_noise, breakup, -450, 1300)
    noise_gain = constant(material, 4.0, -450, 1480)
    noise_scaled = binary(material, unreal.MaterialExpressionMultiply, noise_delta, noise_gain, -250, 1300)
    noise_bias = constant(material, 0.82, -250, 1480)
    noise_biased = binary(material, unreal.MaterialExpressionAdd, noise_scaled, noise_bias, -50, 1300)
    brush_mask = expression(material, unreal.MaterialExpressionSaturate, 150, 1300)
    connect(noise_biased, "", brush_mask, "")

    shape = binary(material, unreal.MaterialExpressionMultiply, end_fade, width_mask, 80, 40)
    sweep = binary(material, unreal.MaterialExpressionMultiply, reveal_mask, tail_mask, 80, 720)
    shaped_sweep = binary(material, unreal.MaterialExpressionMultiply, shape, sweep, 300, 260)
    brush_shape = binary(material, unreal.MaterialExpressionMultiply, shaped_sweep, brush_mask, 500, 320)
    opacity = scalar_parameter(material, "Opacity", 1.0, 500, 540)
    final_opacity = binary(material, unreal.MaterialExpressionMultiply, brush_shape, opacity, 700, 340)

    color = expression(material, unreal.MaterialExpressionVectorParameter, 300, -200)
    color.set_editor_property("parameter_name", "SlashColor")
    color.set_editor_property(
        "default_value", unreal.LinearColor(0.94, 0.97, 1.0, 1.0)
    )
    intensity = scalar_parameter(material, "Intensity", 10.5, 300, -20)
    base_emissive = binary(material, unreal.MaterialExpressionMultiply, color, intensity, 520, -100)
    head_scale = constant(material, 0.82, 300, 80)
    head_bias = constant(material, 0.42, 300, 180)
    head_ramp = binary(material, unreal.MaterialExpressionMultiply, u, head_scale, 520, 80)
    head_brightness = binary(material, unreal.MaterialExpressionAdd, head_ramp, head_bias, 700, 80)
    headed_emissive = binary(material, unreal.MaterialExpressionMultiply, base_emissive, head_brightness, 900, -60)
    emissive = binary(material, unreal.MaterialExpressionMultiply, headed_emissive, final_opacity, 1100, 80)

    unreal.MaterialEditingLibrary.connect_material_property(
        emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        final_opacity, "", unreal.MaterialProperty.MP_OPACITY
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def import_mesh(filename, name, material):
    source_mesh = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source_mesh):
        raise RuntimeError("Missing source mesh: {}".format(source_mesh))
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source_mesh)
    task.set_editor_property("destination_path", MESH_PATH)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    options = unreal.FbxImportUI()
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.EditorAssetLibrary.load_asset("{}/{}".format(MESH_PATH, name))
    if mesh is None:
        raise RuntimeError("Could not import {}".format(name))
    mesh.set_material(0, material)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    return mesh


def asset_data(path):
    return unreal.FXConverterUtilitiesLibrary.create_asset_data(path)


def script_args(path, version=None):
    data = asset_data(path)
    return unreal.CreateScriptContextArgs(data, version) if version else unreal.CreateScriptContextArgs(data)


def random_float(minimum, maximum):
    context = unreal.FXConverterUtilitiesLibrary.create_script_context(
        script_args("/Niagara/DynamicInputs/UniformRange/V2/RandomRangeFloat.RandomRangeFloat")
    )
    context.set_parameter(
        "Minimum", unreal.FXConverterUtilitiesLibrary.create_script_input_float(minimum)
    )
    context.set_parameter(
        "Maximum", unreal.FXConverterUtilitiesLibrary.create_script_input_float(maximum)
    )
    return unreal.FXConverterUtilitiesLibrary.create_script_input_dynamic(
        context, unreal.NiagaraScriptInputType.FLOAT
    )


def linear_color_input(color):
    context = unreal.FXConverterUtilitiesLibrary.create_script_context(
        script_args("/Niagara/DynamicInputs/LinearColor/MakeLinearColorFromVectorAndFloat.MakeLinearColorFromVectorAndFloat")
    )
    context.set_parameter(
        "Vector (RGB)",
        unreal.FXConverterUtilitiesLibrary.create_script_input_vector(
            unreal.Vector(color.r, color.g, color.b)
        ),
    )
    context.set_parameter(
        "Float (Alpha)",
        unreal.FXConverterUtilitiesLibrary.create_script_input_float(color.a),
    )
    return unreal.FXConverterUtilitiesLibrary.create_script_input_dynamic(
        context, unreal.NiagaraScriptInputType.LINEAR_COLOR
    )


def scaled_linked_vector(parameter_name, minimum_speed, maximum_speed):
    fx = unreal.FXConverterUtilitiesLibrary
    context = fx.create_script_context(
        script_args(
            "/Niagara/DynamicInputs/Multiply/Multiply_VectorByFloat.Multiply_VectorByFloat"
        )
    )
    context.set_parameter(
        "Vector",
        fx.create_script_input_linked_parameter(
            parameter_name, unreal.NiagaraScriptInputType.VEC3
        ),
    )
    context.set_parameter("Float", random_float(minimum_speed, maximum_speed))
    return fx.create_script_input_dynamic(
        context, unreal.NiagaraScriptInputType.VEC3
    )


def add_detail_emitter(system_context, emitter_name, user_count, glints):
    fx = unreal.FXConverterUtilitiesLibrary
    emitter = system_context.add_empty_emitter(emitter_name)
    emitter.set_local_space(True)
    emitter.set_sim_target(unreal.NiagaraSimTarget.CPU_SIM)

    state = emitter.find_or_add_module_script(
        "EmitterState",
        script_args("/Niagara/Modules/Emitter/EmitterState.EmitterState", [1, 0]),
        unreal.ScriptExecutionCategory.EMITTER_UPDATE,
    )
    state.set_parameter(
        "Life Cycle Mode",
        fx.create_script_input_enum(
            "/Niagara/Enums/ENiagaraEmitterLifeCycleMode.ENiagaraEmitterLifeCycleMode",
            "Self",
        ),
    )
    state.set_parameter(
        "Loop Behavior",
        fx.create_script_input_enum(
            "/Niagara/Enums/ENiagara_EmitterStateOptions.ENiagara_EmitterStateOptions",
            "Once",
        ),
    )
    state.set_parameter("Loop Duration", fx.create_script_input_float(0.36))

    burst = emitter.find_or_add_module_script(
        "SpawnBurst",
        script_args("/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous", [1, 1]),
        unreal.ScriptExecutionCategory.EMITTER_UPDATE,
    )
    burst.set_parameter(
        "Spawn Count",
        fx.create_script_input_linked_parameter(
            user_count, unreal.NiagaraScriptInputType.INT
        ),
    )
    burst.set_parameter("Spawn Time", fx.create_script_input_float(0.0 if glints else 0.025))

    initialize = emitter.find_or_add_module_script(
        "InitializeParticle",
        script_args("/Niagara/Modules/Spawn/Initialization/V2/InitializeParticle.InitializeParticle", [1, 0]),
        unreal.ScriptExecutionCategory.PARTICLE_SPAWN,
    )
    initialize.set_parameter("Lifetime", random_float(0.12, 0.23) if glints else random_float(0.10, 0.18))
    initialize.set_parameter(
        "Sprite Size Mode",
        fx.create_script_input_enum(
            "/Niagara/Enums/ENiagara_SizeScaleMode.ENiagara_SizeScaleMode",
            "Non-Uniform",
        ),
    )
    initialize.set_parameter(
        "Sprite Size",
        fx.create_script_input_vec2(
            unreal.Vector2D(44.0, 3.2) if glints else unreal.Vector2D(12.0, 5.0)
        ),
        True,
        True,
    )
    initialize.set_parameter(
        "Color",
        linear_color_input(
            unreal.LinearColor(1.0, 1.0, 1.0, 0.95)
            if glints
            else unreal.LinearColor(0.70, 0.76, 0.86, 0.62)
        ),
    )

    location = emitter.find_or_add_module_script(
        "ArcLocation",
        script_args("/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation"),
        unreal.ScriptExecutionCategory.PARTICLE_SPAWN,
    )
    location.set_parameter(
        "Shape Primitive",
        fx.create_script_input_enum(
            "/Niagara/Enums/Location/ENiagara_LocationShapes.ENiagara_LocationShapes",
            "Ring / Disc",
        ),
    )
    location.set_parameter(
        "Ring Radius",
        fx.create_script_input_float(88.0 if glints else 96.0),
    )
    location.set_parameter(
        "Disc Coverage",
        fx.create_script_input_float(110.0 / 360.0),
        True,
        True,
    )
    location.set_parameter(
        "Surface Only Band Thickness",
        fx.create_script_input_float(0.08 if glints else 0.16),
        True,
        True,
    )

    velocity = emitter.find_or_add_module_script(
        "TangentVelocity" if glints else "OutwardVelocity",
        script_args("/Niagara/Modules/Spawn/Velocity/AddVelocity.AddVelocity"),
        unreal.ScriptExecutionCategory.PARTICLE_SPAWN,
    )
    velocity.set_parameter(
        "Velocity Mode",
        fx.create_script_input_enum(
            "/Niagara/Enums/Utility/ENiagara_VelocityMode.ENiagara_VelocityMode",
            "Linear",
        ),
    )
    velocity.set_parameter(
        "Velocity",
        scaled_linked_vector(
            "Particles.ShapeLocation.ShapeTangent"
            if glints
            else "Particles.ShapeLocation.ShapeNormal",
            25.0 if glints else 38.0,
            75.0 if glints else 105.0,
        ),
    )

    emitter.find_or_add_module_script(
        "SolveForcesAndVelocity",
        script_args("/Niagara/Modules/Solvers/SolveForcesAndVelocity.SolveForcesAndVelocity"),
        unreal.ScriptExecutionCategory.PARTICLE_UPDATE,
    )
    emitter.find_or_add_module_script(
        "ParticleState",
        script_args("/Niagara/Modules/Update/Lifetime/ParticleState.ParticleState", [1, 1]),
        unreal.ScriptExecutionCategory.PARTICLE_UPDATE,
    )

    fade = emitter.find_or_add_module_script(
        "DirectionalFade",
        script_args("/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"),
        unreal.ScriptExecutionCategory.PARTICLE_UPDATE,
    )
    one_minus = fx.create_script_context(
        script_args("/Niagara/DynamicInputs/Math/OneMinusFloat.OneMinusFloat")
    )
    one_minus.set_parameter(
        "Float",
        fx.create_script_input_linked_parameter(
            "Particles.NormalizedAge", unreal.NiagaraScriptInputType.FLOAT
        ),
    )
    fade.set_parameter(
        "Scale Alpha",
        fx.create_script_input_dynamic(one_minus, unreal.NiagaraScriptInputType.FLOAT),
        True,
        True,
    )

    renderer = unreal.NiagaraSpriteRendererProperties()
    renderer.set_editor_property(
        "material",
        unreal.load_asset(
            "/Niagara/DefaultAssets/DefaultSpriteMaterial.DefaultSpriteMaterial"
        ),
    )
    try:
        if glints:
            renderer.set_editor_property(
                "alignment", unreal.NiagaraSpriteAlignment.VELOCITY_ALIGNED
            )
    except Exception as exc:
        unreal.log_warning("Velocity-aligned glint renderer fallback: {}".format(exc))
    emitter.add_renderer("GlintRenderer" if glints else "FragmentRenderer", renderer)
    return emitter


def create_niagara_system():
    asset_path = "{}/NS_AxeSlash_Details".format(NIAGARA_PATH)
    existing_system = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing_system is not None:
        return existing_system
    system = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "NS_AxeSlash_Details",
        NIAGARA_PATH,
        unreal.NiagaraSystem,
        unreal.NiagaraSystemFactoryNew(),
    )
    if system is None:
        raise RuntimeError("Could not create {}".format(asset_path))
    context = unreal.FXConverterUtilitiesLibrary.create_system_conversion_context(system)
    glints = add_detail_emitter(
        context, "ElongatedGlints", "User.GlintCount", True
    )
    fragments = add_detail_emitter(
        context, "BrushFragments", "User.FragmentCount", False
    )
    # SystemConversionContext.finalize() refreshes Slate and asserts in a
    # commandlet. Finalizing each emitter applies the same staged stack and
    # renderer actions without requiring a desktop UI.
    glints.finalize()
    fragments.finalize()
    try:
        system.request_compile(False)
    except Exception as exc:
        unreal.log_warning("Niagara compile requested on next load: {}".format(exc))
    unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False)
    return system


mask_asset = ensure_brush_mask()
additive_material = create_material("M_AxeSlash", unreal.BlendMode.BLEND_ADDITIVE, mask_asset)
afterimage_material = create_material(
    "M_AxeSlash_Afterimage", unreal.BlendMode.BLEND_TRANSLUCENT, mask_asset
)
meshes = [
    import_mesh("SM_AxeSlash_Main.obj", "SM_AxeSlash_Main", additive_material),
    import_mesh("SM_AxeSlash_Inner.obj", "SM_AxeSlash_Inner", additive_material),
    import_mesh(
        "SM_AxeSlash_Afterimage.obj",
        "SM_AxeSlash_Afterimage",
        afterimage_material,
    ),
]
niagara_system = create_niagara_system()
unreal.log(
    "CHOPIT_SLASH_ASSETS_READY {} {} {} {} {}".format(
        additive_material.get_path_name(),
        afterimage_material.get_path_name(),
        mask_asset.get_path_name(),
        ",".join(mesh.get_path_name() for mesh in meshes),
        niagara_system.get_path_name(),
    )
)
