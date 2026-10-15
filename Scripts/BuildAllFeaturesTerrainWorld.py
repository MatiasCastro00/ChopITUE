"""Build a playable integration scene from the progression layout."""
import unreal

source = '/Game/NewChopIt/Maps/L_Test_Progression'
target = '/Game/NewChopIt/Maps/L_Test_AllFeaturesTerrain'
assets = unreal.EditorAssetLibrary
if assets.does_asset_exist(target):
    raise RuntimeError('Combined test map already exists; refusing to overwrite: ' + target)
if not assets.duplicate_asset(source, target):
    raise RuntimeError('Could not duplicate progression map')

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level(target):
    raise RuntimeError('Could not open combined map')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode', unreal.ChopItAllFeaturesGameMode)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# The new game mode creates exactly one terrain from the selected DA at Play.
removed = []
for actor in actors.get_all_level_actors():
    if isinstance(actor, (unreal.ChopItTerrainActor, unreal.StaticMeshActor)):
        removed.append(actor.get_actor_label())
        actors.destroy_actor(actor)

def spawn(cls, label, xyz, rot=None):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*xyz), rot or unreal.Rotator())
    if not actor:
        raise RuntimeError('Could not spawn ' + label)
    actor.set_actor_label(label)
    return actor

dialogue = spawn(unreal.ChopItDialogueTrigger, 'Dialogue Demo - interact', (1040, 330, 60))
sequence = unreal.load_asset('/Game/NewChopIt/Data/Dialogue/Sequences/DA_Dialogue_Demo')
if not sequence:
    raise RuntimeError('Dialogue sequence is missing')
dialogue.set_editor_property('sequence', sequence)
anchor = spawn(unreal.ChopItCameraAnchor, 'Dialogue Camera', (560, 520, 240), unreal.Rotator(-10, -28, 0))
anchor.set_editor_property('default_subject', dialogue)
dialogue.set_editor_property('camera_anchor', anchor)
spawn(unreal.ChopItCameraDemoTrigger, 'Camera Demo - interact', (930, 180, 60))

for i, pos in enumerate(((1300, -200, 75), (1550, -450, 75), (1850, 200, 75), (1050, 850, 75))):
    spawn(unreal.ChopItCombatDummy, 'Combat Dummy %02d' % (i + 1), pos)

# The authored quota machine remains near the cabin; this second station
# exposes the chain settings and tether interactions from the chain lab.
chain_class = assets.load_blueprint_class('/Game/NewChopIt/ChainLab/BP_ChainLabMachine')
if not chain_class:
    raise RuntimeError('Chain machine Blueprint is missing')
spawn(chain_class, 'Chain Lab Machine - tether', (-1250, -800, 0))

for actor in actors.get_all_level_actors():
    if isinstance(actor, unreal.NavMeshBoundsVolume):
        actor.set_actor_location(unreal.Vector(0, 0, -200), False, False)
        actor.set_actor_scale3d(unreal.Vector(24, 24, 30))
    elif isinstance(actor, unreal.RecastNavMesh):
        actor.set_editor_property('runtime_generation', unreal.RuntimeGenerationType.DYNAMIC)

if not levels.save_current_level():
    raise RuntimeError('Could not save combined map')
unreal.log('ALL_FEATURES_TERRAIN_READY ' + target + ' removed=' + repr(removed))
