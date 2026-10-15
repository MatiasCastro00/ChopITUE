"""Create the editable Stage 1 phone Blueprint and spawn Data Asset. Safe to rerun."""
import unreal

ROOT = '/Game/NewChopIt/Art/Models/Phone'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

blueprint_path = ROOT + '/BP_MysteryPhone'
if not unreal.EditorAssetLibrary.does_asset_exist(blueprint_path):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.ChopItMysteryPhone)
    blueprint = TOOLS.create_asset('BP_MysteryPhone', ROOT, unreal.Blueprint, factory)
    if not blueprint:
        raise RuntimeError('Could not create phone Blueprint')
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)

phone_class = unreal.EditorAssetLibrary.load_blueprint_class(blueprint_path)
if not phone_class:
    raise RuntimeError('Could not load phone Blueprint class')

config_path = ROOT + '/DA_PhoneSpawn'
config = unreal.load_asset(config_path)
if not config:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.ChopItPhoneSpawnConfig)
    config = TOOLS.create_asset('DA_PhoneSpawn', ROOT, unreal.ChopItPhoneSpawnConfig, factory)
if not isinstance(config, unreal.ChopItPhoneSpawnConfig):
    raise RuntimeError('Wrong type at ' + config_path)
config.set_editor_property('phone_class', phone_class)
if not unreal.EditorAssetLibrary.save_loaded_asset(config, only_if_is_dirty=False):
    raise RuntimeError('Could not save ' + config_path)
unreal.log('PHONE_STAGE1_ASSETS_READY ' + config.get_path_name())
