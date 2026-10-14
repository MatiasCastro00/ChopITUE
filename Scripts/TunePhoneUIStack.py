"""Put the comic closer to the phone and persist prompt UI layer settings."""
import unreal

root = '/Game/ChopIt/Phone'
indicator = unreal.load_asset(root + '/DA_PhoneIndicator')
if not isinstance(indicator, unreal.ChopItPhoneIndicatorConfig):
    raise RuntimeError('Missing DA_PhoneIndicator')
indicator.set_editor_property('world_height', 95.0)
if not unreal.EditorAssetLibrary.save_loaded_asset(indicator, only_if_is_dirty=False):
    raise RuntimeError('Cannot save DA_PhoneIndicator')

phone_asset = unreal.load_asset(root + '/BP_MysteryPhone')
phone_class = unreal.EditorAssetLibrary.load_blueprint_class(root + '/BP_MysteryPhone')
if not phone_asset or not phone_class:
    raise RuntimeError('Missing BP_MysteryPhone')
phone_default = unreal.get_default_object(phone_class)
prompt = phone_default.get_editor_property('prompt_widget')
if not prompt:
    raise RuntimeError('Missing authored PromptWidget component')
prompt.set_editor_property('shared_layer_name', unreal.Name('PhonePromptLayer'))
prompt.set_editor_property('layer_z_order', -200)
if not unreal.EditorAssetLibrary.save_loaded_asset(phone_asset, only_if_is_dirty=False):
    raise RuntimeError('Cannot save BP_MysteryPhone')
unreal.log('PHONE_UI_STACK_READY height=' + str(indicator.get_editor_property('world_height')) +
           ' prompt_layer=' + str(prompt.get_editor_property('layer_z_order')))
