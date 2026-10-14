"""Make the test phone's ring and comic indicator perceivable from its spawn range."""
import unreal

root = '/Game/ChopIt/Phone/'
call = unreal.load_asset(root + 'DA_PhoneCall')
indicator = unreal.load_asset(root + 'DA_PhoneIndicator')
if not isinstance(call, unreal.ChopItPhoneCallConfig):
    raise RuntimeError('Missing DA_PhoneCall')
if not isinstance(indicator, unreal.ChopItPhoneIndicatorConfig):
    raise RuntimeError('Missing DA_PhoneIndicator')

call.set_editor_property('first_call_delay_seconds', 0.1)
call.set_editor_property('min_interval_seconds', 10.0)
call.set_editor_property('max_interval_seconds', 10.0)
call.set_editor_property('ring_duration_seconds', 3.0)
call.set_editor_property('full_volume_radius', 350.0)
call.set_editor_property('audible_distance', 7500.0)
call.set_editor_property('volume', 1.0)
indicator.set_editor_property('max_visible_distance', 7500.0)

for asset in (call, indicator):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Could not save ' + asset.get_path_name())
    unreal.log('PHONE_PRESENTATION_TUNED ' + asset.get_path_name())
