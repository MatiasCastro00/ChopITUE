"""Keep the comic proportional to distance and above the visible phone."""
import unreal

indicator = unreal.load_asset('/Game/ChopIt/Phone/DA_PhoneIndicator')
if not isinstance(indicator, unreal.ChopItPhoneIndicatorConfig):
    raise RuntimeError('Missing DA_PhoneIndicator')

indicator.set_editor_property('onscreen_offset', unreal.Vector2D(0.0, 0.0))
indicator.set_editor_property('min_scale', 0.5)
indicator.set_editor_property('max_scale', 1.0)
indicator.set_editor_property('onscreen_scale_multiplier', 0.85)
indicator.set_editor_property('offscreen_scale_multiplier', 1.05)
if not unreal.EditorAssetLibrary.save_loaded_asset(indicator, only_if_is_dirty=False):
    raise RuntimeError('Could not save DA_PhoneIndicator')
unreal.log('PHONE_INDICATOR_PERSPECTIVE_READY')
