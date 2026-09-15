"""Read-only verification for the expanded forest in L_PSX_test."""

import math
import unreal

MAP_PATH = "/Game/ChopIt/World/Maps/L_PSX_test"
TREE_ROOT = "/Game/ChopIt/Art/Models/Trees/New"
NEAR_COUNT = 8
MESH_PATHS = {
    1: "{}/Tree1/SM_Tree_Far_01".format(TREE_ROOT),
    2: "{}/Tree2/SM_Tree_Cabin_02".format(TREE_ROOT),
    3: "{}/Tree3/SM_Tree_Cabin_03".format(TREE_ROOT),
}
MATERIAL_PATHS = {
    1: "{}/Tree1/M_Tree_Far_01".format(TREE_ROOT),
    2: "{}/Tree2/M_Tree_Cabin_02".format(TREE_ROOT),
    3: "{}/Tree3/M_Tree_Cabin_03".format(TREE_ROOT),
}
TEXTURE_PATHS = (
    "{}/Tree1/T_Tree_Far_01_BaseColor".format(TREE_ROOT),
    "{}/Tree1/T_Tree_Far_01_Metallic".format(TREE_ROOT),
    "{}/Tree1/T_Tree_Far_01_Normal".format(TREE_ROOT),
    "{}/Tree1/T_Tree_Far_01_RM".format(TREE_ROOT),
    "{}/Tree1/T_Tree_Far_01_Roughness".format(TREE_ROOT),
    "{}/Tree2/T_Tree_Cabin_02_BaseColor".format(TREE_ROOT),
    "{}/Tree2/T_Tree_Cabin_02_Normal".format(TREE_ROOT),
    "{}/Tree3/T_Tree_Cabin_03_BaseColor".format(TREE_ROOT),
)
GROUND_ROOT = "/Game/ChopIt/Art/Materials/Ground"
GRASS_TEXTURE_PATHS = tuple(
    "{}/T_Grass_Ground_{:02d}".format(GROUND_ROOT, index) for index in range(1, 4)
)
GRASS_MATERIAL_PATHS = tuple(
    "{}/M_Grass_Ground_{:02d}_PSX_Tiled".format(GROUND_ROOT, index) for index in range(1, 4)
)
OUTLINE_PATH = "/Game/ChopIt/Art/Materials/PSX_Materials/MI_PSX_Outline_Jittering"


def distance_2d(a, b):
    return math.hypot(a.x - b.x, a.y - b.y)


for path in tuple(MESH_PATHS.values()) + tuple(MATERIAL_PATHS.values()) + TEXTURE_PATHS + GRASS_TEXTURE_PATHS + GRASS_MATERIAL_PATHS:
    assert unreal.EditorAssetLibrary.does_asset_exist(path), "Missing forest asset: {}".format(path)

for path in tuple(MATERIAL_PATHS.values()) + GRASS_MATERIAL_PATHS:
    material = unreal.EditorAssetLibrary.load_asset(path)
    base_color = unreal.MaterialEditingLibrary.get_material_property_input_node(
        material, unreal.MaterialProperty.MP_BASE_COLOR
    )
    assert isinstance(base_color, unreal.MaterialExpressionTextureSample), "{} lacks its color texture".format(path)
    wpo = unreal.MaterialEditingLibrary.get_material_property_input_node(
        material, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET
    )
    assert isinstance(wpo, unreal.MaterialExpressionSubtract), "{} lacks PSX vertex jitter".format(path)
    scalar_parameters = {str(name) for name in unreal.MaterialEditingLibrary.get_scalar_parameter_names(material)}
    assert "PSXVertexGridCm" in scalar_parameters, "{} lacks the PSX jitter parameter".format(path)
    if path in GRASS_MATERIAL_PATHS:
        assert "TileSizeCm" in scalar_parameters, "{} is not world tiled".format(path)

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
assert world, "Could not load {}".format(MAP_PATH)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
actors_by_label = {actor.get_actor_label(): actor for actor in actors}
cabin = actors_by_label["CabinHub"]
post_process = next(actor for actor in actors if actor.get_class().get_name() == "PostProcessVolume")
settings = post_process.get_editor_property("settings")
weighted_blendables = settings.get_editor_property("weighted_blendables").get_editor_property("array")
post_process_materials = {
    blendable.get_editor_property("object").get_path_name().split(".")[0]
    for blendable in weighted_blendables
    if blendable.get_editor_property("object")
}
assert "/Game/ChopIt/Art/Materials/PSX_Materials/MI_PSX_CellShading" in post_process_materials
assert "/Game/ChopIt/Art/Materials/PSX_Materials/MI_PSX_Pixelation_TextSafe" in post_process_materials
cell_instance = unreal.EditorAssetLibrary.load_asset(
    "/Game/ChopIt/Art/Materials/PSX_Materials/MI_PSX_CellShading"
)
cell_parent = unreal.EditorAssetLibrary.load_asset(
    "/Game/ChopIt/Art/Materials/PSX_Materials/M_PSX_CellShading_Color"
)
assert cell_instance.get_editor_property("parent").get_path_name().split(".")[0] == cell_parent.get_path_name().split(".")[0]
cell_output = unreal.MaterialEditingLibrary.get_material_property_input_node(
    cell_parent, unreal.MaterialProperty.MP_EMISSIVE_COLOR
)
assert isinstance(cell_output, unreal.MaterialExpressionDivide), "Cel shading must preserve RGB color"
assert not any(
    isinstance(item, unreal.MaterialExpressionDesaturation)
    for item in unreal.MaterialEditingLibrary.get_material_expressions(cell_parent)
), "Cel shading still contains grayscale conversion"
trees = [
    actor
    for actor in actors
    if actor.get_actor_label().startswith("Tree_") and actor.get_class().get_name() == "ChopItTree"
]
assert len(trees) >= 20, "Expected at least 20 gameplay trees, found {}".format(len(trees))

cabin_location = cabin.get_actor_location()
trees.sort(key=lambda actor: (distance_2d(actor.get_actor_location(), cabin_location), actor.get_actor_label()))
counts = {1: 0, 2: 0, 3: 0}

for index, actor in enumerate(trees):
    components = {
        component.get_name(): component
        for component in actor.get_components_by_class(unreal.ActorComponent)
    }
    trunk = components["TrunkMesh"]
    crown = components["CrownMesh"]
    mesh = trunk.get_editor_property("static_mesh")
    mesh_path = mesh.get_path_name().split(".")[0]
    variant = next(number for number, path in MESH_PATHS.items() if path == mesh_path)
    expected_variants = (2, 3) if index < NEAR_COUNT else (1,)
    assert variant in expected_variants, "{} has variant {} at sorted index {}".format(
        actor.get_actor_label(), variant, index
    )
    distance = distance_2d(actor.get_actor_location(), cabin_location)
    if index < NEAR_COUNT:
        assert distance < 1700.0, "Near tree {} is too far away: {}".format(actor.get_actor_label(), distance)
    else:
        assert distance >= 2490.0, "Far tree {} is still too close: {}".format(actor.get_actor_label(), distance)
    material_path = trunk.get_material(0).get_path_name().split(".")[0]
    assert material_path == MATERIAL_PATHS[variant]
    assert isinstance(unreal.EditorAssetLibrary.load_asset(material_path), unreal.Material)
    assert components["PhysicsRoot"].get_class().get_name() == "CapsuleComponent"
    assert trunk.get_overlay_material().get_path_name().split(".")[0] == OUTLINE_PATH
    assert not crown.get_editor_property("visible")
    assert crown.get_editor_property("hidden_in_game")
    counts[variant] += 1

assert counts == {1: len(trees) - NEAR_COUNT, 2: 4, 3: 4}, "Unexpected distribution: {}".format(counts)

ground = actors_by_label["Ground"]
_, ground_extent = ground.get_actor_bounds(False)
assert ground_extent.x >= 3800.0 and ground_extent.y >= 3800.0
ground_mesh = ground.get_components_by_class(unreal.StaticMeshComponent)[0]
assert ground_mesh.get_material(0).get_path_name().split(".")[0] == GRASS_MATERIAL_PATHS[0]
for label in ("Boundary_North", "Boundary_South", "Boundary_East", "Boundary_West"):
    location = actors_by_label[label].get_actor_location()
    assert max(abs(location.x), abs(location.y)) >= 3790.0, "{} was not expanded".format(label)
_, nav_extent = actors_by_label["NavMeshBounds"].get_actor_bounds(False)
assert nav_extent.x >= 3990.0 and nav_extent.y >= 3990.0

chain = unreal.EditorAssetLibrary.load_asset("/Game/ChopIt/World/ChainLab/DA_Chain_Default")
assert chain.get_editor_property("chain_link_count") == 64
assert abs(chain.get_editor_property("max_chain_length") - 3600.0) < 0.1
assert chain.get_editor_property("minimum_deployed_links") == 16
assert abs(chain.get_editor_property("chain_slack") - 200.0) < 0.1

near_max = max(distance_2d(actor.get_actor_location(), cabin_location) for actor in trees[:NEAR_COUNT])
far_min = min(distance_2d(actor.get_actor_location(), cabin_location) for actor in trees[NEAR_COUNT:])
unreal.log(
    "PSX_TREE_VERIFY_OK map={} distribution={} near_max={:.1f} far_min={:.1f} nav_extent=({:.1f},{:.1f}) chain_length={:.1f} grass_materials=3 capsule_roots=true psx_jitter=true outline=true".format(
        MAP_PATH,
        counts,
        near_max,
        far_min,
        nav_extent.x,
        nav_extent.y,
        chain.get_editor_property("max_chain_length"),
    )
)
