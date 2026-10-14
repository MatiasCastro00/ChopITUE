"""Import the supplied single strike and set the three double-ring groups."""
import os
import unreal

root = '/Game/ChopIt/Phone'
call = unreal.load_asset(root + '/DA_PhoneCall')
indicator = unreal.load_asset(root + '/DA_PhoneIndicator')
if not isinstance(call, unreal.ChopItPhoneCallConfig):
    raise RuntimeError('Missing DA_PhoneCall')
if not isinstance(indicator, unreal.ChopItPhoneIndicatorConfig):
    raise RuntimeError('Missing DA_PhoneIndicator')

sound_path = root + '/S_PhoneRingStrike'
sound = unreal.load_asset(sound_path)
if not sound:
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', os.path.join(unreal.Paths.project_dir(), 'SourceArt', 'Phone', 'RingStrike.wav'))
    task.set_editor_property('destination_path', root)
    task.set_editor_property('destination_name', 'S_PhoneRingStrike')
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    task.set_editor_property('replace_existing', False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.load_asset(sound_path)
if not isinstance(sound, unreal.SoundWave):
    raise RuntimeError('Could not import S_PhoneRingStrike')
sound.set_editor_property('looping', False)
call.set_editor_property('ring_sound', sound)
call.set_editor_property('ring_pair_spacing_seconds', 1.0)
call.set_editor_property('ring_strike_spacing_seconds', 0.32)
call.set_editor_property('ring_pairs_per_call', 3)
call.set_editor_property('ring_duration_seconds', 3.0)
indicator.set_editor_property('onscreen_offset', unreal.Vector2D(0.0, 0.0))

for asset in (sound, call, indicator):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Could not save ' + asset.get_path_name())
unreal.log('PHONE_RING_STRIKE_READY ' + sound.get_path_name())
