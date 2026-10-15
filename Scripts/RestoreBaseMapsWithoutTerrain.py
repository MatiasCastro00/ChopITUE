"""Remove authored terrain actors and keep the functional phone on the PSX base map."""
import unreal

map_paths = ['/Game/NewChopIt/Terrain/L_Terrain_Phase2'] + [
    '/Game/NewChopIt/Maps/' + name for name in (
        'L_Dev_Sandbox', 'L_PSX_test', 'L_Startup',
        'L_Test_AllFeaturesTerrain', 'L_Test_ChainLab', 'L_Test_Combat',
        'L_Test_Cycle', 'L_Test_Dialogue', 'L_Test_Economy', 'L_Test_Elite',
        'L_Test_Enemies', 'L_Test_Harvest', 'L_Test_Infinite',
        'L_Test_Progression', 'L_Test_Shop', 'L_Test_TerrainDefault')
]
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
phone_class = unreal.EditorAssetLibrary.load_blueprint_class('/Game/NewChopIt/Art/Models/Phone/BP_MysteryPhone')
if not phone_class:
    raise RuntimeError('Missing BP_MysteryPhone')

for path in map_paths:
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        continue
    unreal.EditorLoadingAndSavingUtils.load_map(path)
    changed = False
    removed = []
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.ChopItTerrainActor):
            removed.append(actor.get_actor_label())
            if not actors.destroy_actor(actor):
                raise RuntimeError('Could not remove terrain from ' + path)
            changed = True
    if path.endswith('/L_PSX_test'):
        placed = [actor for actor in actors.get_all_level_actors()
                  if isinstance(actor, unreal.ChopItMysteryPhone)]
        if not placed:
            # The original Ground mesh has its top at Z=0, next to the cabin.
            phone = actors.spawn_actor_from_class(phone_class, unreal.Vector(-250, 250, 0))
            if not phone:
                raise RuntimeError('Could not place phone on restored PSX map')
            phone.set_actor_label('BP_MysteryPhone_Placed')
            phone.set_editor_property('hearing_range', 850.0)
            changed = True
    if changed:
        if not levels.save_current_level():
            raise RuntimeError('Could not save ' + path)
    unreal.log('TERRAIN_REMOVAL ' + path + ' removed=' + repr(removed) +
               ' changed=' + str(changed))
