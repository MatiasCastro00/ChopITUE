"""Create editable nearby phone prompt assets. Safe to rerun."""
import unreal

ROOT = '/Game/NewChopIt/Art/Models/Phone'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

widget_path = ROOT + '/WBP_PhoneInteractPrompt'
if not unreal.EditorAssetLibrary.does_asset_exist(widget_path):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.ChopItPhonePromptWidget)
    widget = TOOLS.create_asset('WBP_PhoneInteractPrompt', ROOT, unreal.Blueprint, factory)
    if not widget:
        raise RuntimeError('Could not create phone prompt widget Blueprint')
    unreal.EditorAssetLibrary.save_loaded_asset(widget, only_if_is_dirty=False)
widget_class = unreal.EditorAssetLibrary.load_blueprint_class(widget_path)
if not widget_class:
    raise RuntimeError('Could not load phone prompt widget class')

config_path = ROOT + '/DA_PhonePrompt'
config = unreal.load_asset(config_path)
if not config:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.ChopItPhonePromptConfig)
    config = TOOLS.create_asset('DA_PhonePrompt', ROOT, unreal.ChopItPhonePromptConfig, factory)
if not isinstance(config, unreal.ChopItPhonePromptConfig):
    raise RuntimeError('Wrong type at ' + config_path)
if not config.get_editor_property('widget_class'):
    config.set_editor_property('widget_class', widget_class)
if not unreal.EditorAssetLibrary.save_loaded_asset(config, only_if_is_dirty=False):
    raise RuntimeError('Could not save ' + config_path)
unreal.log('PHONE_STAGE4_ASSETS_READY ' + config.get_path_name())
