"""Create the test map; terrain is spawned from the project default DA at runtime."""
import unreal

map_path = '/Game/ChopIt/World/Maps/L_Test_TerrainDefault'
asset_path = '/Game/ChopIt/World/DA_Terrain'
asset = unreal.load_asset(asset_path)
if not isinstance(asset, unreal.ChopItTerrainDataAsset):
    raise RuntimeError('Missing terrain recipe: ' + asset_path)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(map_path):
    raise RuntimeError('Test map already exists; refusing to overwrite it: ' + map_path)
if not levels.new_level(map_path):
    raise RuntimeError('Could not create ' + map_path)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode', unreal.ChopItTerrainTestGameMode)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 3000), unreal.Rotator(-50, -35, 0))
sun.set_actor_label('Terrain Test Sun')
sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sun.light_component.set_editor_property('atmosphere_sun_light', True)
sun.light_component.set_intensity(5.0)

sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000))
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property('real_time_capture', True)
sky.light_component.set_intensity(1.2)
actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 2500), unreal.Rotator(-30, 30, 0))

if not levels.save_current_level():
    raise RuntimeError('Could not save ' + map_path)
unreal.log('TERRAIN_DEFAULT_TEST_WORLD_READY ' + map_path + ' uses project default ' + asset.get_path_name())
