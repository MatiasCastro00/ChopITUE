"""Install PSX trees, tiled grass materials, and the expanded L_PSX_test layout.

Run with Unreal Editor closed. The script is intentionally idempotent: it can be
executed again after the assets have already been renamed. It also configures
the rounded tree collision, playable bounds, navigation, and starting chain.
"""

import math
import unreal


MAP_PATH = "/Game/ChopIt/World/Maps/L_PSX_test"
TREE_ROOT = "/Game/ChopIt/Art/Models/Trees/New"
MATERIAL_ROOT = "/Game/ChopIt/Art/Materials"
GROUND_MATERIAL_ROOT = MATERIAL_ROOT + "/Ground"
PSX_OUTLINE_PATH = MATERIAL_ROOT + "/PSX_Materials/MI_PSX_Outline_Jittering"
NEAR_TREE_COUNT = 8
PLAYABLE_HALF_EXTENT = 3800.0
NAV_HALF_EXTENT = 4000.0
MAX_CHAIN_LENGTH = 3600.0
MAX_CHAIN_LINKS = 64

RENAMES = (
    ("{}/Tree1/low_poly_tree_3d_model_basecolor".format(TREE_ROOT), "{}/Tree1/T_Tree_Far_01_BaseColor".format(TREE_ROOT)),
    ("{}/Tree1/low_poly_tree_3d_model_metallic".format(TREE_ROOT), "{}/Tree1/T_Tree_Far_01_Metallic".format(TREE_ROOT)),
    ("{}/Tree1/low_poly_tree_3d_model_normal".format(TREE_ROOT), "{}/Tree1/T_Tree_Far_01_Normal".format(TREE_ROOT)),
    ("{}/Tree1/low_poly_tree_3d_model_rm".format(TREE_ROOT), "{}/Tree1/T_Tree_Far_01_RM".format(TREE_ROOT)),
    ("{}/Tree1/low_poly_tree_3d_model_roughness".format(TREE_ROOT), "{}/Tree1/T_Tree_Far_01_Roughness".format(TREE_ROOT)),
    ("{}/Tree1/tripo_mat_719371cb".format(TREE_ROOT), "{}/Tree1/MI_Tree_Far_01".format(TREE_ROOT)),
    ("{}/Tree1/tripo_convert_719371cb-6689-428f-b7d6-c7470da06dae".format(TREE_ROOT), "{}/Tree1/SM_Tree_Far_01".format(TREE_ROOT)),
    ("{}/Tree2/stylized_tree_3d_model_basecolor".format(TREE_ROOT), "{}/Tree2/T_Tree_Cabin_02_BaseColor".format(TREE_ROOT)),
    ("{}/Tree2/stylized_tree_3d_model_normal".format(TREE_ROOT), "{}/Tree2/T_Tree_Cabin_02_Normal".format(TREE_ROOT)),
    ("{}/Tree2/tripo_mat_dc04f681".format(TREE_ROOT), "{}/Tree2/MI_Tree_Cabin_02".format(TREE_ROOT)),
    ("{}/Tree2/tripo_convert_dc04f681-73f7-44f3-a035-2b51092bdc67".format(TREE_ROOT), "{}/Tree2/SM_Tree_Cabin_02".format(TREE_ROOT)),
    ("{}/Tree3/tripo_image_dfb0bfc1-9fe2-422d-8263-5d37a34f6efe_0_0".format(TREE_ROOT), "{}/Tree3/T_Tree_Cabin_03_BaseColor".format(TREE_ROOT)),
    ("{}/Tree3/tripo_mat_dfb0bfc1".format(TREE_ROOT), "{}/Tree3/MI_Tree_Cabin_03".format(TREE_ROOT)),
    ("{}/Tree3/tripo_convert_dfb0bfc1-9fe2-422d-8263-5d37a34f6efe".format(TREE_ROOT), "{}/Tree3/SM_Tree_Cabin_03".format(TREE_ROOT)),
    (MATERIAL_ROOT + "/Grass", GROUND_MATERIAL_ROOT + "/T_Grass_Ground_01"),
    (MATERIAL_ROOT + "/Grass3", GROUND_MATERIAL_ROOT + "/T_Grass_Ground_02"),
    (MATERIAL_ROOT + "/Grass4", GROUND_MATERIAL_ROOT + "/T_Grass_Ground_03"),
)

TREE_VARIANTS = {
    1: ("{}/Tree1/SM_Tree_Far_01".format(TREE_ROOT), "{}/Tree1/M_Tree_Far_01".format(TREE_ROOT), 620.0),
    2: ("{}/Tree2/SM_Tree_Cabin_02".format(TREE_ROOT), "{}/Tree2/M_Tree_Cabin_02".format(TREE_ROOT), 560.0),
    3: ("{}/Tree3/SM_Tree_Cabin_03".format(TREE_ROOT), "{}/Tree3/M_Tree_Cabin_03".format(TREE_ROOT), 520.0),
}

TEXTURES = {
    1: {
        "base_color": "{}/Tree1/T_Tree_Far_01_BaseColor".format(TREE_ROOT),
        "normal": "{}/Tree1/T_Tree_Far_01_Normal".format(TREE_ROOT),
        "metallic": "{}/Tree1/T_Tree_Far_01_Metallic".format(TREE_ROOT),
        "roughness": "{}/Tree1/T_Tree_Far_01_Roughness".format(TREE_ROOT),
        "packed": "{}/Tree1/T_Tree_Far_01_RM".format(TREE_ROOT),
    },
    2: {
        "base_color": "{}/Tree2/T_Tree_Cabin_02_BaseColor".format(TREE_ROOT),
        "normal": "{}/Tree2/T_Tree_Cabin_02_Normal".format(TREE_ROOT),
    },
    3: {
        "base_color": "{}/Tree3/T_Tree_Cabin_03_BaseColor".format(TREE_ROOT),
    },
}

GRASS_VARIANTS = {
    1: (GROUND_MATERIAL_ROOT + "/T_Grass_Ground_01", GROUND_MATERIAL_ROOT + "/M_Grass_Ground_01_PSX_Tiled"),
    2: (GROUND_MATERIAL_ROOT + "/T_Grass_Ground_02", GROUND_MATERIAL_ROOT + "/M_Grass_Ground_02_PSX_Tiled"),
    3: (GROUND_MATERIAL_ROOT + "/T_Grass_Ground_03", GROUND_MATERIAL_ROOT + "/M_Grass_Ground_03_PSX_Tiled"),
}


def asset_exists(path):
    return unreal.EditorAssetLibrary.does_asset_exist(path)


def rename_assets():
    for old_path, new_path in RENAMES:
        if asset_exists(new_path):
            if asset_exists(old_path):
                raise RuntimeError("Both old and new asset names exist: {} / {}".format(old_path, new_path))
            continue
        if not asset_exists(old_path):
            raise RuntimeError("Missing source asset: {}".format(old_path))
        if not unreal.EditorAssetLibrary.rename_asset(old_path, new_path):
            raise RuntimeError("Could not rename {} to {}".format(old_path, new_path))
        unreal.log("PSX_TREE_RENAMED {} -> {}".format(old_path, new_path))


def material_expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)


def load_texture(path):
    texture = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Expected Texture2D at {}".format(path))
    return texture


def add_psx_jitter(material, x=-250, y=520):
    # Vertex snapping creates the characteristic low-resolution PSX movement
    # without relying on the project's incomplete MF_PSX_Jittering asset.
    world_position = material_expression(material, unreal.MaterialExpressionWorldPosition, x - 640, y)
    grid_size = material_expression(material, unreal.MaterialExpressionScalarParameter, x - 620, y + 130)
    grid_size.set_editor_property("parameter_name", "PSXVertexGridCm")
    grid_size.set_editor_property("default_value", 3.0)
    divide = material_expression(material, unreal.MaterialExpressionDivide, x - 420, y)
    floor = material_expression(material, unreal.MaterialExpressionFloor, x - 260, y)
    multiply = material_expression(material, unreal.MaterialExpressionMultiply, x - 100, y)
    snapped_offset = material_expression(material, unreal.MaterialExpressionSubtract, x + 60, y)
    unreal.MaterialEditingLibrary.connect_material_expressions(world_position, "", divide, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(grid_size, "", divide, "B")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(divide, "", floor, ""):
        raise RuntimeError("Could not connect PSX Floor input for {}".format(material.get_path_name()))
    unreal.MaterialEditingLibrary.connect_material_expressions(floor, "", multiply, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(grid_size, "", multiply, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(multiply, "", snapped_offset, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(world_position, "", snapped_offset, "B")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        snapped_offset, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET
    ):
        raise RuntimeError("Could not connect PSX jitter to {}".format(material.get_path_name()))
    return snapped_offset


def create_tree_material(variant_number, material_path):
    folder, name = material_path.rsplit("/", 1)
    material = unreal.EditorAssetLibrary.load_asset(material_path)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Expected Material at {}".format(material_path))

    texture_paths = TEXTURES[variant_number]
    base_color = load_texture(texture_paths["base_color"])
    base_color.set_editor_property("srgb", True)

    normal = load_texture(texture_paths["normal"]) if "normal" in texture_paths else None
    if normal:
        normal.set_editor_property("srgb", False)
        normal.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)

    metallic = load_texture(texture_paths["metallic"]) if "metallic" in texture_paths else None
    roughness = load_texture(texture_paths["roughness"]) if "roughness" in texture_paths else None
    packed = load_texture(texture_paths["packed"]) if "packed" in texture_paths else None
    for mask_texture in (metallic, roughness, packed):
        if mask_texture:
            mask_texture.set_editor_property("srgb", False)
            mask_texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)

    material.set_editor_property("two_sided", True)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("used_with_nanite", True)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    base_sample = material_expression(material, unreal.MaterialExpressionTextureSample, -600, -120)
    base_sample.set_editor_property("texture", base_color)
    unreal.MaterialEditingLibrary.connect_material_property(
        base_sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    )

    if normal:
        normal_sample = material_expression(material, unreal.MaterialExpressionTextureSample, -600, 40)
        normal_sample.set_editor_property("texture", normal)
        normal_sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        unreal.MaterialEditingLibrary.connect_material_property(
            normal_sample, "RGB", unreal.MaterialProperty.MP_NORMAL
        )

    if metallic:
        metallic_sample = material_expression(material, unreal.MaterialExpressionTextureSample, -600, 200)
        metallic_sample.set_editor_property("texture", metallic)
        metallic_sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        unreal.MaterialEditingLibrary.connect_material_property(
            metallic_sample, "R", unreal.MaterialProperty.MP_METALLIC
        )

    if roughness:
        roughness_sample = material_expression(material, unreal.MaterialExpressionTextureSample, -600, 360)
        roughness_sample.set_editor_property("texture", roughness)
        roughness_sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        unreal.MaterialEditingLibrary.connect_material_property(
            roughness_sample, "R", unreal.MaterialProperty.MP_ROUGHNESS
        )
    else:
        roughness_value = material_expression(material, unreal.MaterialExpressionConstant, -420, 360)
        roughness_value.set_editor_property("r", 0.82)
        unreal.MaterialEditingLibrary.connect_material_property(
            roughness_value, "", unreal.MaterialProperty.MP_ROUGHNESS
        )

    add_psx_jitter(material)

    unreal.MaterialEditingLibrary.recompile_material(material)
    for texture in (base_color, normal, metallic, roughness, packed):
        if texture:
            unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    unreal.log("PSX_TREE_MATERIAL_BUILT variant={} material={}".format(variant_number, material_path))
    return material


def create_grass_material(variant_number, texture_path, material_path):
    folder, name = material_path.rsplit("/", 1)
    material = unreal.EditorAssetLibrary.load_asset(material_path)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Expected Material at {}".format(material_path))

    texture = load_texture(texture_path)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)

    material.set_editor_property("two_sided", False)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("used_with_nanite", True)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    world_position = material_expression(material, unreal.MaterialExpressionWorldPosition, -900, -120)
    world_xy = material_expression(material, unreal.MaterialExpressionComponentMask, -700, -120)
    world_xy.set_editor_property("r", True)
    world_xy.set_editor_property("g", True)
    world_xy.set_editor_property("b", False)
    world_xy.set_editor_property("a", False)
    tile_size = material_expression(material, unreal.MaterialExpressionScalarParameter, -700, 40)
    tile_size.set_editor_property("parameter_name", "TileSizeCm")
    tile_size.set_editor_property("default_value", 600.0)
    world_uv = material_expression(material, unreal.MaterialExpressionDivide, -480, -100)
    sample = material_expression(material, unreal.MaterialExpressionTextureSample, -220, -120)
    sample.set_editor_property("texture", texture)

    unreal.MaterialEditingLibrary.connect_material_expressions(world_position, "", world_xy, "Input")
    unreal.MaterialEditingLibrary.connect_material_expressions(world_xy, "", world_uv, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(tile_size, "", world_uv, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(world_uv, "", sample, "Coordinates")
    unreal.MaterialEditingLibrary.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness = material_expression(material, unreal.MaterialExpressionConstant, -80, 120)
    roughness.set_editor_property("r", 0.9)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    add_psx_jitter(material, -180, 300)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    unreal.log(
        "PSX_GRASS_MATERIAL_BUILT variant={} material={} tile_size_cm=600 world_aligned=true".format(
            variant_number, material_path
        )
    )
    return material


def assign_mesh_material(mesh, material):
    slots = mesh.get_editor_property("static_materials")
    if not slots:
        raise RuntimeError("{} has no material slots".format(mesh.get_path_name()))
    slots[0].set_editor_property("material_interface", material)
    mesh.set_editor_property("static_materials", slots)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)


def distance_2d(a, b):
    return math.hypot(a.x - b.x, a.y - b.y)


def components_by_name(actor):
    return {component.get_name(): component for component in actor.get_components_by_class(unreal.ActorComponent)}


def configure_tree(actor, mesh, material, target_height, variation, variant_number, distance, yaw):
    components = components_by_name(actor)
    trunk = components.get("TrunkMesh")
    crown = components.get("CrownMesh")
    root = components.get("PhysicsRoot")
    crown_collision = components.get("CrownCollision")
    health_label = components.get("HealthLabel")
    if not all((trunk, crown, root, crown_collision, health_label)):
        raise RuntimeError("{} is missing required ChopItTree components".format(actor.get_actor_label()))

    bounds = mesh.get_bounds()
    mesh_height = bounds.box_extent.z * 2.0
    if mesh_height <= 0.0:
        raise RuntimeError("{} has invalid bounds".format(mesh.get_path_name()))

    height = target_height * variation
    uniform_scale = height / mesh_height
    half_height = height * 0.5

    actor_location = actor.get_actor_location()
    actor.set_actor_location(unreal.Vector(actor_location.x, actor_location.y, half_height), False, False)
    actor.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), False)

    trunk.set_static_mesh(mesh)
    trunk.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, -half_height))
    trunk.set_editor_property("relative_scale3d", unreal.Vector(uniform_scale, uniform_scale, uniform_scale))
    trunk.set_editor_property("visible", True)
    trunk.set_editor_property("hidden_in_game", False)
    trunk.set_material(0, material)
    outline_material = unreal.EditorAssetLibrary.load_asset(PSX_OUTLINE_PATH)
    if not isinstance(outline_material, unreal.MaterialInterface):
        raise RuntimeError("Missing PSX outline material at {}".format(PSX_OUTLINE_PATH))
    trunk.set_overlay_material(outline_material)

    # Keep the gameplay tree class and its fall/reward behavior, replacing only
    # the two primitive blockout visuals with the authored whole-tree mesh.
    crown.set_editor_property("visible", False)
    crown.set_editor_property("hidden_in_game", True)
    crown.set_overlay_material(outline_material)

    root.set_capsule_size(55.0, half_height, False)
    crown_collision.set_sphere_radius(max(110.0, min(185.0, height * 0.28)), False)
    crown_collision.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, height * 0.18))
    health_label.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, half_height + 60.0))

    unreal.log(
        "PSX_TREE_ASSIGNED label={} variant={} distance_to_cabin={:.1f} height={:.1f} yaw={:.1f} mesh={}".format(
            actor.get_actor_label(), variant_number, distance, height, yaw, mesh.get_name()
        )
    )


def configure_expanded_map(actors_by_label):
    ground = actors_by_label.get("Ground")
    if not ground:
        raise RuntimeError("Ground was not found in {}".format(MAP_PATH))

    _, ground_extent = ground.get_actor_bounds(False)
    if ground_extent.x < PLAYABLE_HALF_EXTENT or ground_extent.y < PLAYABLE_HALF_EXTENT:
        required_scale = PLAYABLE_HALF_EXTENT * 2.0 / 100.0
        ground.set_actor_scale3d(unreal.Vector(required_scale, required_scale, 1.0))

    boundary_length_scale = PLAYABLE_HALF_EXTENT * 2.0 / 100.0
    boundary_settings = {
        "Boundary_North": (
            unreal.Vector(0.0, PLAYABLE_HALF_EXTENT, 200.0),
            unreal.Vector(boundary_length_scale, 0.25, 5.0),
        ),
        "Boundary_South": (
            unreal.Vector(0.0, -PLAYABLE_HALF_EXTENT, 200.0),
            unreal.Vector(boundary_length_scale, 0.25, 5.0),
        ),
        "Boundary_East": (
            unreal.Vector(PLAYABLE_HALF_EXTENT, 0.0, 200.0),
            unreal.Vector(0.25, boundary_length_scale, 5.0),
        ),
        "Boundary_West": (
            unreal.Vector(-PLAYABLE_HALF_EXTENT, 0.0, 200.0),
            unreal.Vector(0.25, boundary_length_scale, 5.0),
        ),
    }
    for label, (location, scale) in boundary_settings.items():
        actor = actors_by_label.get(label)
        if not actor:
            raise RuntimeError("{} was not found in {}".format(label, MAP_PATH))
        actor.set_actor_location(location, False, False)
        actor.set_actor_scale3d(scale)

    nav_bounds = actors_by_label.get("NavMeshBounds")
    if not nav_bounds:
        raise RuntimeError("NavMeshBounds was not found in {}".format(MAP_PATH))
    nav_bounds.set_actor_location(unreal.Vector(0.0, 0.0, 200.0), False, False)
    nav_scale = NAV_HALF_EXTENT / 100.0
    nav_bounds.set_actor_scale3d(unreal.Vector(nav_scale, nav_scale, 5.0))
    unreal.log(
        "PSX_MAP_EXPANDED playable_half_extent={} nav_half_extent={}".format(
            PLAYABLE_HALF_EXTENT, NAV_HALF_EXTENT
        )
    )


rename_assets()

meshes = {}
for variant_number, (mesh_path, material_path, target_height) in TREE_VARIANTS.items():
    material = create_tree_material(variant_number, material_path)
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Expected StaticMesh at {}".format(mesh_path))
    assign_mesh_material(mesh, material)
    meshes[variant_number] = (mesh, material, target_height)

grass_materials = {}
for variant_number, (texture_path, material_path) in GRASS_VARIANTS.items():
    grass_materials[variant_number] = create_grass_material(variant_number, texture_path, material_path)

unreal.EditorAssetLibrary.save_directory(TREE_ROOT, only_if_is_dirty=False, recursive=True)
unreal.EditorAssetLibrary.save_directory(GROUND_MATERIAL_ROOT, only_if_is_dirty=False, recursive=True)

chain_definition = unreal.EditorAssetLibrary.load_asset("/Game/ChopIt/World/ChainLab/DA_Chain_Default")
if not isinstance(chain_definition, unreal.ChopItChainDefinition):
    raise RuntimeError("DA_Chain_Default could not be loaded")
chain_definition.set_editor_property("chain_link_count", MAX_CHAIN_LINKS)
chain_definition.set_editor_property("max_chain_length", MAX_CHAIN_LENGTH)
chain_definition.set_editor_property("minimum_deployed_links", 16)
chain_definition.set_editor_property("chain_slack", 200.0)
chain_definition.set_editor_property("chain_feed_speed", 1000.0)
unreal.EditorAssetLibrary.save_loaded_asset(chain_definition, only_if_is_dirty=False)
unreal.log(
    "PSX_CHAIN_EXPANDED links={} max_length={} min_links={} slack={}".format(
        MAX_CHAIN_LINKS, MAX_CHAIN_LENGTH, 16, 200.0
    )
)

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
if not world:
    raise RuntimeError("Could not load {}".format(MAP_PATH))

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_subsystem.get_all_level_actors()
actors_by_label = {actor.get_actor_label(): actor for actor in actors}
cabin = next((actor for actor in actors if actor.get_actor_label() == "CabinHub"), None)
trees = [actor for actor in actors if actor.get_actor_label().startswith("Tree_") and actor.get_class().get_name() == "ChopItTree"]
if not cabin:
    raise RuntimeError("CabinHub was not found in {}".format(MAP_PATH))
if len(trees) < NEAR_TREE_COUNT:
    raise RuntimeError("Expected at least {} ChopIt trees; found {}".format(NEAR_TREE_COUNT, len(trees)))

cabin_location = cabin.get_actor_location()
trees.sort(key=lambda actor: (distance_2d(actor.get_actor_location(), cabin_location), actor.get_actor_label()))
variation_cycle = (0.94, 1.00, 1.06, 0.98)
far_radii = (2500.0, 2700.0, 2900.0, 3100.0)

configure_expanded_map(actors_by_label)

ground_meshes = actors_by_label["Ground"].get_components_by_class(unreal.StaticMeshComponent)
if not ground_meshes:
    raise RuntimeError("Ground has no StaticMeshComponent")
ground_meshes[0].set_material(0, grass_materials[1])
unreal.log("PSX_GROUND_ASSIGNED material={}".format(grass_materials[1].get_path_name()))

for index, actor in enumerate(trees):
    variant_number = 2 + (index % 2) if index < NEAR_TREE_COUNT else 1
    if variant_number == 1:
        location = actor.get_actor_location()
        offset_x = location.x - cabin_location.x
        offset_y = location.y - cabin_location.y
        angle = math.atan2(offset_y, offset_x)
        radius = far_radii[(index - NEAR_TREE_COUNT) % len(far_radii)]
        actor.set_actor_location(
            unreal.Vector(
                cabin_location.x + math.cos(angle) * radius,
                cabin_location.y + math.sin(angle) * radius,
                location.z,
            ),
            False,
            False,
        )
    distance = distance_2d(actor.get_actor_location(), cabin_location)
    mesh, material, target_height = meshes[variant_number]
    configure_tree(
        actor,
        mesh,
        material,
        target_height,
        variation_cycle[index % len(variation_cycle)],
        variant_number,
        distance,
        (index * 137.5) % 360.0,
    )

if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
    raise RuntimeError("Could not save {}".format(MAP_PATH))

unreal.log(
    "PSX_TREE_INSTALL_COMPLETE map={} near_tree_count={} far_tree_count={}".format(
        MAP_PATH, NEAR_TREE_COUNT, len(trees) - NEAR_TREE_COUNT
    )
)
