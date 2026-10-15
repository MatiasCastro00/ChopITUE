"""Create the editable comic indicator and its Data Asset. Safe to rerun."""
import unreal

ROOT = '/Game/NewChopIt/Art/Models/Phone'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

widget_path = ROOT + '/WBP_PhoneRingIndicator'
if not unreal.EditorAssetLibrary.does_asset_exist(widget_path):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.ChopItPhoneIndicatorWidget)
    widget = TOOLS.create_asset('WBP_PhoneRingIndicator', ROOT, unreal.Blueprint, factory)
    if not widget:
        raise RuntimeError('Could not create ring indicator widget Blueprint')
    unreal.EditorAssetLibrary.save_loaded_asset(widget, only_if_is_dirty=False)
widget_class = unreal.EditorAssetLibrary.load_blueprint_class(widget_path)
if not widget_class:
    raise RuntimeError('Could not load ring indicator widget class')

config_path = ROOT + '/DA_PhoneIndicator'
config = unreal.load_asset(config_path)
if not config:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.ChopItPhoneIndicatorConfig)
    config = TOOLS.create_asset('DA_PhoneIndicator', ROOT, unreal.ChopItPhoneIndicatorConfig, factory)
if not isinstance(config, unreal.ChopItPhoneIndicatorConfig):
    raise RuntimeError('Wrong type at ' + config_path)
if not config.get_editor_property('widget_class'):
    config.set_editor_property('widget_class', widget_class)
if not unreal.EditorAssetLibrary.save_loaded_asset(config, only_if_is_dirty=False):
    raise RuntimeError('Could not save ' + config_path)
unreal.log('PHONE_STAGE3_ASSETS_READY ' + config.get_path_name())
