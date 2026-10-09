"""Create a placeable harmless Blueprint for viewing Mandrake VFX in PIE."""
import unreal

folder = '/Game/ChopIt/Items/Mandrake'
name = folder + '/BP_MandrakeFXPreview'
asset = unreal.EditorAssetLibrary.load_asset(name)
if not asset:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.ChopItMandrakePreview)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'BP_MandrakeFXPreview', folder, unreal.Blueprint, factory)
if not asset:
    raise RuntimeError('Could not create BP_MandrakeFXPreview')
if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
    raise RuntimeError('Could not save BP_MandrakeFXPreview')
unreal.log('MANDRAKE_PREVIEW_READY ' + asset.get_path_name())
