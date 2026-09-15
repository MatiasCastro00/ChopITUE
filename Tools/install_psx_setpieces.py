"""Import the supplied house and machine and replace the gameplay blockout visuals."""

import unreal


MAP_PATH = "/Game/ChopIt/World/Maps/L_PSX_test"
ROOT = "/Game/ChopIt/Art/Models/SetPieces"
OUTLINE_PATH = "/Game/ChopIt/Art/Materials/PSX_Materials/MI_PSX_Outline_Jittering"
SOURCES = (
    {
        "label": "CabinHub",
        "component": "CabinVisual",
        "source": r"C:\Users\Elmr\Downloads\HouseText.fbx",
        "texture_source": r"C:\Users\Elmr\Documents\Unreal Projects\ChopIt\SourceArt\SetPieces\House_BaseColor.jpg",
        "folder": ROOT + "/House",
        "mesh": "SM_House_PSX",
        "texture": "T_House_BaseColor",
        "material": "M_House_PSX_Jittering",
        "height": 560.0,
        "roughness": 0.88,
        "metallic": 0.0,
    },
    {
        "label": "QuotaMachine",
        "component": "MachineVisual",
        "source": r"C:\Users\Elmr\Downloads\Machine.fbx",
        "texture_source": r"C:\Users\Elmr\Documents\Unreal Projects\ChopIt\SourceArt\SetPieces\Machine_BaseColor.jpg",
        "folder": ROOT + "/Machine",
        "mesh": "SM_Machine_PSX",
        "texture": "T_Machine_BaseColor",
        "material": "M_Machine_PSX_Jittering",
        "height": 360.0,
        "roughness": 0.62,
        "metallic": 0.38,
    },
)


def asset_path(spec, key):
    return spec["folder"] + "/" + spec[key]


def import_fbx(spec):
    task = unreal.AssetImportTask()
    task.filename = spec["source"]
    task.destination_path = spec["folder"]
    task.destination_name = spec["mesh"]
    task.automated = True
    task.replace_existing = True
    task.save = True
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.EditorAssetLibrary.load_asset(asset_path(spec, "mesh"))
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Could not import static mesh " + spec["label"])
    return mesh


def import_texture(spec):
    task = unreal.AssetImportTask()
    task.filename = spec["texture_source"]
    task.destination_path = spec["folder"]
    task.destination_name = spec["texture"]
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset(asset_path(spec, "texture"))
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Could not import texture " + spec["label"])
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    return texture


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)


def build_material(spec, texture):
    path = asset_path(spec, "material")
    folder, name = path.rsplit("/", 1)
    material = unreal.EditorAssetLibrary.load_asset(path)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Could not create material " + spec["label"])
    material.set_editor_property("two_sided", True)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("used_with_nanite", True)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    sample = expression(material, unreal.MaterialExpressionTextureSample, -500, -80)
    sample.set_editor_property("texture", texture)
    unreal.MaterialEditingLibrary.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = expression(material, unreal.MaterialExpressionConstant, -260, 120)
    roughness.set_editor_property("r", spec["roughness"])
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    metallic = expression(material, unreal.MaterialExpressionConstant, -260, 220)
    metallic.set_editor_property("r", spec["metallic"])
    unreal.MaterialEditingLibrary.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    world_position = expression(material, unreal.MaterialExpressionWorldPosition, -680, 380)
    grid_size = expression(material, unreal.MaterialExpressionScalarParameter, -660, 500)
    grid_size.set_editor_property("parameter_name", "PSXVertexGridCm")
    grid_size.set_editor_property("default_value", 3.0)
    divide = expression(material, unreal.MaterialExpressionDivide, -460, 380)
    floor = expression(material, unreal.MaterialExpressionFloor, -300, 380)
    multiply = expression(material, unreal.MaterialExpressionMultiply, -140, 380)
    jitter = expression(material, unreal.MaterialExpressionSubtract, 20, 380)
    unreal.MaterialEditingLibrary.connect_material_expressions(world_position, "", divide, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(grid_size, "", divide, "B")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(divide, "", floor, ""):
        raise RuntimeError("Could not connect PSX Floor input for " + spec["label"])
    unreal.MaterialEditingLibrary.connect_material_expressions(floor, "", multiply, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(grid_size, "", multiply, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(multiply, "", jitter, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(world_position, "", jitter, "B")
    if not unreal.MaterialEditingLibrary.connect_material_property(jitter, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET):
        raise RuntimeError("Could not apply PSX jitter to " + spec["label"])
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def find_component(actor, component_name):
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        if component.get_name() == component_name:
            return component
    raise RuntimeError("{} has no {} component".format(actor.get_actor_label(), component_name))


def replace_visual(spec, mesh, material, actor):
    component = find_component(actor, spec["component"])
    bounds = mesh.get_bounds()
    mesh_height = bounds.box_extent.z * 2.0
    if mesh_height <= 0.0:
        raise RuntimeError("Invalid mesh bounds " + spec["label"])
    scale = spec["height"] / mesh_height
    component.set_relative_scale3d(unreal.Vector(scale, scale, scale))
    component.set_relative_location(
        unreal.Vector(0.0, 0.0, -(bounds.origin.z - bounds.box_extent.z) * scale), False, False
    )
    component.set_relative_rotation(unreal.Rotator(), False, False)
    component.set_static_mesh(mesh)
    for material_index in range(max(1, component.get_num_materials())):
        component.set_material(material_index, material)
    outline = unreal.EditorAssetLibrary.load_asset(OUTLINE_PATH)
    if not isinstance(outline, unreal.MaterialInterface):
        raise RuntimeError("PSX outline material is unavailable")
    component.set_overlay_material(outline)
    component.set_collision_profile_name("BlockAll")


assets = []
for source in SOURCES:
    mesh = import_fbx(source)
    texture = import_texture(source)
    material = build_material(source, texture)
    static_materials = mesh.get_editor_property("static_materials")
    for static_material in static_materials:
        static_material.set_editor_property("material_interface", material)
    mesh.set_editor_property("static_materials", static_materials)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    assets.append((source, mesh, material))

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
if not world:
    raise RuntimeError("Could not load PSX map")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
actors_by_label = {actor.get_actor_label(): actor for actor in actors}
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
removed_duplicates = 0
for duplicate_label in ("PSX_House", "PSX_Machine"):
    duplicate = actors_by_label.get(duplicate_label)
    if duplicate:
        if not actor_subsystem.destroy_actor(duplicate):
            raise RuntimeError("Could not remove duplicate actor " + duplicate_label)
        removed_duplicates += 1

for source, mesh, material in assets:
    actor = actors_by_label.get(source["label"])
    if not actor:
        raise RuntimeError("Gameplay actor not found: " + source["label"])
    replace_visual(source, mesh, material, actor)

if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
    raise RuntimeError("Could not save PSX map")
unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
unreal.log("PSX_SETPIECES_INSTALL_OK visuals=replaced duplicates_removed={} shaders=jitter_outline_pixelation_cellshading".format(removed_duplicates))
