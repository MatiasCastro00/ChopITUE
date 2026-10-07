"""Author the boss chest's materials and CPU Niagara bursts.

Run in Unreal with -ExecutePythonScript=Scripts/BuildBossChestAssets.py.
The script only creates missing assets, preserving later designer edits.
"""

import unreal


ROOT = "/Game/ChopIt/Items/Chest"
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
FX = unreal.FXConverterUtilitiesLibrary


def make_material(name, color, roughness, metallic, glow):
    path = ROOT + "/" + name
    existing = unreal.EditorAssetLibrary.load_asset(path)
    if existing:
        return existing
    material = TOOLS.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Could not create " + path)
    base = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -460, -100
    )
    base.set_editor_property("parameter_name", "ChestColor")
    base.set_editor_property("default_value", unreal.LinearColor(*color, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        base, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    rough = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -300, 170
    )
    rough.set_editor_property("r", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    metal = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -300, 330
    )
    metal.set_editor_property("r", metallic)
    unreal.MaterialEditingLibrary.connect_material_property(
        metal, "", unreal.MaterialProperty.MP_METALLIC
    )
    if glow:
        emissive = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionMultiply, -100, -80
        )
        intensity = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant, -300, -300
        )
        intensity.set_editor_property("r", glow)
        unreal.MaterialEditingLibrary.connect_material_expressions(base, "", emissive, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(intensity, "", emissive, "B")
        unreal.MaterialEditingLibrary.connect_material_property(
            emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def script(path, version=None):
    data = FX.create_asset_data(path)
    return unreal.CreateScriptContextArgs(data, version) if version else unreal.CreateScriptContextArgs(data)


def random_float(low, high):
    context = FX.create_script_context(
        script("/Niagara/DynamicInputs/UniformRange/V2/RandomRangeFloat.RandomRangeFloat")
    )
    context.set_parameter("Minimum", FX.create_script_input_float(low))
    context.set_parameter("Maximum", FX.create_script_input_float(high))
    return FX.create_script_input_dynamic(context, unreal.NiagaraScriptInputType.FLOAT)


def color_input(color):
    context = FX.create_script_context(
        script("/Niagara/DynamicInputs/LinearColor/MakeLinearColorFromVectorAndFloat.MakeLinearColorFromVectorAndFloat")
    )
    context.set_parameter(
        "Vector (RGB)", FX.create_script_input_vector(unreal.Vector(*color[:3]))
    )
    context.set_parameter("Float (Alpha)", FX.create_script_input_float(color[3]))
    return FX.create_script_input_dynamic(context, unreal.NiagaraScriptInputType.LINEAR_COLOR)


def velocity_input(low, high):
    context = FX.create_script_context(
        script("/Niagara/DynamicInputs/Multiply/Multiply_VectorByFloat.Multiply_VectorByFloat")
    )
    context.set_parameter(
        "Vector",
        FX.create_script_input_linked_parameter(
            "Particles.ShapeLocation.ShapeNormal", unreal.NiagaraScriptInputType.VEC3
        ),
    )
    context.set_parameter("Float", random_float(low, high))
    return FX.create_script_input_dynamic(context, unreal.NiagaraScriptInputType.VEC3)


def add_emitter(system, name, count, radius, speed, color, width, lifetime):
    emitter = system.add_empty_emitter(name)
    emitter.set_local_space(True)
    emitter.set_sim_target(unreal.NiagaraSimTarget.CPU_SIM)
    state = emitter.find_or_add_module_script(
        "EmitterState", script("/Niagara/Modules/Emitter/EmitterState.EmitterState", [1, 0]),
        unreal.ScriptExecutionCategory.EMITTER_UPDATE,
    )
    state.set_parameter(
        "Life Cycle Mode",
        FX.create_script_input_enum(
            "/Niagara/Enums/ENiagaraEmitterLifeCycleMode.ENiagaraEmitterLifeCycleMode", "Self"
        ),
    )
    state.set_parameter(
        "Loop Behavior",
        FX.create_script_input_enum(
            "/Niagara/Enums/ENiagara_EmitterStateOptions.ENiagara_EmitterStateOptions", "Once"
        ),
    )
    state.set_parameter("Loop Duration", FX.create_script_input_float(0.45))
    burst = emitter.find_or_add_module_script(
        "SpawnBurst", script("/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous", [1, 1]),
        unreal.ScriptExecutionCategory.EMITTER_UPDATE,
    )
    burst.set_parameter("Spawn Count", FX.create_script_input_int(count))
    burst.set_parameter("Spawn Time", FX.create_script_input_float(0.01))
    initialize = emitter.find_or_add_module_script(
        "InitializeParticle",
        script("/Niagara/Modules/Spawn/Initialization/V2/InitializeParticle.InitializeParticle", [1, 0]),
        unreal.ScriptExecutionCategory.PARTICLE_SPAWN,
    )
    initialize.set_parameter("Lifetime", random_float(lifetime * 0.7, lifetime * 1.3))
    initialize.set_parameter(
        "Sprite Size Mode",
        FX.create_script_input_enum("/Niagara/Enums/ENiagara_SizeScaleMode.ENiagara_SizeScaleMode", "Non-Uniform"),
    )
    initialize.set_parameter("Sprite Size", FX.create_script_input_vec2(unreal.Vector2D(width, width * 0.28)), True, True)
    initialize.set_parameter("Color", color_input(color))
    location = emitter.find_or_add_module_script(
        "RingLocation", script("/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation"),
        unreal.ScriptExecutionCategory.PARTICLE_SPAWN,
    )
    location.set_parameter(
        "Shape Primitive", FX.create_script_input_enum(
            "/Niagara/Enums/Location/ENiagara_LocationShapes.ENiagara_LocationShapes", "Ring / Disc"
        ),
    )
    location.set_parameter("Ring Radius", FX.create_script_input_float(radius))
    velocity = emitter.find_or_add_module_script(
        "OutwardVelocity", script("/Niagara/Modules/Spawn/Velocity/AddVelocity.AddVelocity"),
        unreal.ScriptExecutionCategory.PARTICLE_SPAWN,
    )
    velocity.set_parameter(
        "Velocity Mode", FX.create_script_input_enum(
            "/Niagara/Enums/Utility/ENiagara_VelocityMode.ENiagara_VelocityMode", "Linear"
        ),
    )
    velocity.set_parameter("Velocity", velocity_input(speed * 0.6, speed * 1.4))
    emitter.find_or_add_module_script(
        "SolveForcesAndVelocity",
        script("/Niagara/Modules/Solvers/SolveForcesAndVelocity.SolveForcesAndVelocity"),
        unreal.ScriptExecutionCategory.PARTICLE_UPDATE,
    )
    emitter.find_or_add_module_script(
        "ParticleState", script("/Niagara/Modules/Update/Lifetime/ParticleState.ParticleState", [1, 1]),
        unreal.ScriptExecutionCategory.PARTICLE_UPDATE,
    )
    fade = emitter.find_or_add_module_script(
        "Fade", script("/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"),
        unreal.ScriptExecutionCategory.PARTICLE_UPDATE,
    )
    one_minus = FX.create_script_context(
        script("/Niagara/DynamicInputs/Math/OneMinusFloat.OneMinusFloat")
    )
    one_minus.set_parameter(
        "Float", FX.create_script_input_linked_parameter(
            "Particles.NormalizedAge", unreal.NiagaraScriptInputType.FLOAT
        ),
    )
    fade.set_parameter("Scale Alpha", FX.create_script_input_dynamic(
        one_minus, unreal.NiagaraScriptInputType.FLOAT
    ), True, True)
    renderer = unreal.NiagaraSpriteRendererProperties()
    renderer.set_editor_property("material", unreal.load_asset(
        "/Niagara/DefaultAssets/DefaultSpriteMaterial.DefaultSpriteMaterial"
    ))
    emitter.add_renderer("ChestGlints", renderer)
    emitter.finalize()


def make_system(name, gold_count, teal_count, radius, speed):
    path = ROOT + "/" + name
    existing = unreal.EditorAssetLibrary.load_asset(path)
    if existing:
        return existing
    asset = TOOLS.create_asset(name, ROOT, unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if not asset:
        raise RuntimeError("Could not create " + path)
    context = FX.create_system_conversion_context(asset)
    add_emitter(context, "GoldSparks", gold_count, radius, speed,
                (1.0, 0.62, 0.12, 0.95), 17.0, 0.85)
    add_emitter(context, "ForestDust", teal_count, radius * 0.75, speed * 0.65,
                (0.24, 0.85, 0.62, 0.65), 9.0, 1.15)
    try:
        asset.request_compile(False)
    except AttributeError:
        # This Python binding is unavailable in some UE 5.8 builds; saving the
        # staged emitters schedules Niagara compilation on the next load.
        pass
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    return asset


wood = make_material("M_BossChest_Wood", (0.16, 0.075, 0.026), 0.78, 0.0, 0.0)
gold = make_material("M_BossChest_Gold", (0.72, 0.42, 0.09), 0.28, 0.78, 0.15)
appear = make_system("NS_BossChest_Appear", 65, 36, 85.0, 180.0)
opening = make_system("NS_BossChest_Open", 90, 50, 60.0, 250.0)
idle = make_system("NS_BossChest_Idle", 12, 7, 58.0, 38.0)
unreal.log("CHOPIT_CHEST_ASSETS_READY {} {} {} {} {}".format(
    wood.get_path_name(), gold.get_path_name(), appear.get_path_name(), opening.get_path_name(), idle.get_path_name()
))
del TOOLS
