"""Import the supplied comic artwork and assign it to the phone indicator."""
import os
import unreal

root = '/Game/ChopIt/Phone'
source_dir = os.path.join(unreal.Paths.project_dir(), 'SourceArt', 'Phone')
tools = unreal.AssetToolsHelpers.get_asset_tools()


def ensure_texture(source_name, asset_name):
    source = os.path.join(source_dir, source_name)
    if not os.path.isfile(source):
        raise RuntimeError('Missing phone artwork: ' + source)
    asset_path = root + '/' + asset_name
    texture = unreal.load_asset(asset_path)
    if not texture:
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', source)
        task.set_editor_property('destination_path', root)
        task.set_editor_property('destination_name', asset_name)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        task.set_editor_property('replace_existing', False)
        tools.import_asset_tasks([task])
        texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError('Could not import ' + asset_path)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError('Could not save ' + asset_path)
    return texture


bubble = ensure_texture('Globo.png', 'T_PhoneComicBubble')
ring = ensure_texture('Ring.png', 'T_PhoneComicRing')
indicator = unreal.load_asset(root + '/DA_PhoneIndicator')
if not isinstance(indicator, unreal.ChopItPhoneIndicatorConfig):
    raise RuntimeError('Missing DA_PhoneIndicator')
indicator.set_editor_property('bubble_texture', bubble)
indicator.set_editor_property('ring_texture', ring)
if not unreal.EditorAssetLibrary.save_loaded_asset(indicator, only_if_is_dirty=False):
    raise RuntimeError('Could not save DA_PhoneIndicator')
unreal.log('PHONE_COMIC_READY bubble=' + bubble.get_path_name() + ' ring=' + ring.get_path_name())
