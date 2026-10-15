"""Create the weighted devil/angel variants and selection pool. Safe to rerun."""
import unreal

ROOT = '/Game/NewChopIt/Art/Models/Phone'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def data_asset(name, cls):
    path = ROOT + '/' + name
    asset = unreal.load_asset(path)
    if asset:
        if not isinstance(asset, cls):
            raise RuntimeError('Wrong type at ' + path)
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = TOOLS.create_asset(name, ROOT, cls, factory)
    if not asset:
        raise RuntimeError('Could not create ' + path)
    return asset, True


devil, fresh_devil = data_asset('DA_PhoneDevil', unreal.ChopItPhoneVariantData)
if fresh_devil:
    devil.set_editor_property('variant_id', 'Devil')
    devil.set_editor_property('display_name', 'Teléfono diabólico')
    devil.set_editor_property('weight', 80.0)
    devil.set_editor_property('accent_color', unreal.LinearColor(1.0, 0.035, 0.025, 1.0))
    devil.set_editor_property('peak_light_intensity', 18000.0)
    devil.set_editor_property('idle_light_intensity', 900.0)
    devil.set_editor_property('reveal_duration', 1.15)
    devil.set_editor_property('light_radius', 560.0)
unreal.EditorAssetLibrary.save_loaded_asset(devil, only_if_is_dirty=False)

angel, fresh_angel = data_asset('DA_PhoneAngel', unreal.ChopItPhoneVariantData)
if fresh_angel:
    angel.set_editor_property('variant_id', 'Angel')
    angel.set_editor_property('display_name', 'Teléfono angelical')
    angel.set_editor_property('weight', 20.0)
    angel.set_editor_property('accent_color', unreal.LinearColor(0.85, 0.78, 1.0, 1.0))
    angel.set_editor_property('peak_light_intensity', 14000.0)
    angel.set_editor_property('idle_light_intensity', 700.0)
    angel.set_editor_property('reveal_duration', 1.35)
    angel.set_editor_property('light_radius', 620.0)
unreal.EditorAssetLibrary.save_loaded_asset(angel, only_if_is_dirty=False)

pool, fresh_pool = data_asset('DA_PhoneVariants', unreal.ChopItPhoneVariantPool)
if fresh_pool or not pool.get_editor_property('variants'):
    pool.set_editor_property('variants', [devil, angel])
unreal.EditorAssetLibrary.save_loaded_asset(pool, only_if_is_dirty=False)
unreal.log('PHONE_STAGE5_ASSETS_READY ' + pool.get_path_name())
