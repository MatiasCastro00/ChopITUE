"""Replace L_PSX_test's blockout floor with a DA_Terrain editor preview.

The map game mode rebuilds this same actor with a new seed on every Play.
"""
import unreal

map_path = '/Game/ChopIt/World/Maps/L_PSX_test'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level(map_path):
    raise RuntimeError('Could not load ' + map_path)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing = actors.get_all_level_actors()
if any(isinstance(actor, unreal.ChopItTerrainActor) for actor in existing):
    raise RuntimeError('L_PSX_test already has a terrain actor; inspect it before rerunning this script')

recipe = unreal.load_asset('/Game/ChopIt/World/DA_Terrain')
material = unreal.load_asset('/Game/ChopIt/Art/Materials/Ground/M_Grass_Ground_03_PSX_Tiled')
if not recipe or not material:
    raise RuntimeError('The default terrain recipe or PSX grass material is missing')

removed = []
for actor in existing:
    if isinstance(actor, unreal.StaticMeshActor) and actor.get_actor_label() in {
        'Ground', 'Hub_Path', 'Hub_Marker',
        'Boundary_North', 'Boundary_South', 'Boundary_East', 'Boundary_West',
    }:
        removed.append(actor.get_actor_label())
        if not actors.destroy_actor(actor):
            raise RuntimeError('Could not remove blockout actor ' + removed[-1])
    elif isinstance(actor, unreal.RecastNavMesh):
        actor.set_editor_property('runtime_generation', unreal.RuntimeGenerationType.DYNAMIC)
    elif actor.get_actor_label() == 'BP_MandrakeFXPreview':
        actor.set_editor_property('tags', list(actor.get_editor_property('tags')) + ['AlignToGeneratedTerrain'])

preview = actors.spawn_actor_from_class(unreal.ChopItTerrainActor, unreal.Vector(0, 0, 0))
if not preview:
    raise RuntimeError('Could not create the terrain preview')
preview.set_actor_label('Terrain_PSX_DA_Preview')
settings = recipe.make_settings(0)
settings.spawn_flat_center = unreal.Vector2D(750, 0)
settings.spawn_flat_radius = max(settings.spawn_flat_radius, 2500)
settings.spawn_flat_blend_width = max(settings.spawn_flat_blend_width, 500)
preview.set_editor_property('terrain_settings', settings)
preview.set_editor_property('mesh_settings', recipe.mesh)
preview.set_editor_property('terrain_material', material)
preview.regenerate_terrain()
world.get_world_settings().set_editor_property('default_game_mode', unreal.ChopItAllFeaturesGameMode)
if not levels.save_current_level():
    raise RuntimeError('Could not save L_PSX_test')
unreal.log('PSX_TERRAIN_READY map=' + map_path + ' removed=' + repr(removed)
           + ' preview=' + preview.get_actor_label() + ' collision=' + str(recipe.mesh.enable_collision))
