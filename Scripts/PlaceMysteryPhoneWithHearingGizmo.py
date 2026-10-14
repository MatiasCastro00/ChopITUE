"""Place the functional phone Blueprint in PSX_test for editor range tuning."""
import unreal

map_path = '/Game/ChopIt/World/Maps/L_PSX_test'
world = unreal.EditorLoadingAndSavingUtils.load_map(map_path)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
phone_class = unreal.EditorAssetLibrary.load_blueprint_class('/Game/ChopIt/Phone/BP_MysteryPhone')
if not phone_class:
    raise RuntimeError('Missing BP_MysteryPhone')

placed = [actor for actor in actors.get_all_level_actors()
          if isinstance(actor, unreal.ChopItMysteryPhone)]
if placed:
    phone = placed[0]
else:
    # Place it beside the cabin on the original Ground mesh.
    location = unreal.Vector(-250.0, 250.0, 0.0)
    phone = actors.spawn_actor_from_class(phone_class, location)
    if not phone:
        raise RuntimeError('Could not place BP_MysteryPhone')
    phone.set_actor_label('BP_MysteryPhone_Placed')
phone.set_editor_property('hearing_range', 850.0)
if not levels.save_current_level():
    raise RuntimeError('Could not save L_PSX_test')
unreal.log('PHONE_RANGE_GIZMO_PLACED ' + phone.get_actor_label() +
           ' at ' + str(phone.get_actor_location()))
