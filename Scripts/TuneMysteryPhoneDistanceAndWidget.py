"""Make the phone comic perspective stronger and author its editable UMG layout."""
import unreal

root = '/Game/NewChopIt/Art/Models/Phone'
indicator = unreal.load_asset(root + '/DA_PhoneIndicator')
call = unreal.load_asset(root + '/DA_PhoneCall')
widget = unreal.load_asset(root + '/WBP_PhoneRingIndicator')
if not isinstance(indicator, unreal.ChopItPhoneIndicatorConfig):
    raise RuntimeError('Missing indicator data asset')
if not isinstance(call, unreal.ChopItPhoneCallConfig):
    raise RuntimeError('Missing call data asset')
if not isinstance(widget, unreal.WidgetBlueprint):
    raise RuntimeError('Missing Widget Blueprint')

indicator.set_editor_property('min_scale', 0.38)
indicator.set_editor_property('max_scale', 1.15)
indicator.set_editor_property('distance_scale_exponent', 1.35)
indicator.set_editor_property('onscreen_scale_multiplier', 0.95)
indicator.set_editor_property('distance_scale_curve', None)
call.set_editor_property('full_volume_radius', 110.0)
unreal.log('PHONE_AUDIO_ATTENUATION_ASSET ' + str(call.get_editor_property('attenuation_asset')))

if not unreal.ChopItPhoneWidgetEditorLibrary.build_editable_phone_indicator():
    raise RuntimeError('Could not build editable UMG widget tree')
for asset in (indicator, call, widget):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Could not save ' + asset.get_path_name())
unreal.log('PHONE_DISTANCE_WIDGET_READY min=0.38 max=1.15 exponent=1.35 widget=' + widget.get_path_name())
