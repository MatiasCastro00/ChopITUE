"""Import the supplied prompt sprites and author the editable UMG phone prompt."""
import os
import unreal

root = '/Game/NewChopIt/Art/Models/Phone'
source_dir = os.path.join(unreal.Paths.project_dir(), 'SourceArt', 'Phone', 'Prompt')
assets = {
    'T_PhonePromptKeyPlate': 'KeyPlate.png',
    'T_PhonePromptKeyE': 'KeyE.png',
    'T_PhonePromptRayLeft': 'RayLeft.png',
    'T_PhonePromptRayCenter': 'RayCenter.png',
    'T_PhonePromptRayRight': 'RayRight.png',
}
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
for name, filename in assets.items():
    path = root + '/' + name
    texture = unreal.load_asset(path)
    if not texture:
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', os.path.join(source_dir, filename))
        task.set_editor_property('destination_path', root)
        task.set_editor_property('destination_name', name)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        asset_tools.import_asset_tasks([task])
        texture = unreal.load_asset(path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError('Could not import ' + filename)
    texture.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)

if not unreal.ChopItPhoneWidgetEditorLibrary.build_editable_phone_prompt():
    raise RuntimeError('Could not build WBP_PhoneInteractPrompt')
widget = unreal.load_asset(root + '/WBP_PhoneInteractPrompt')
if not unreal.EditorAssetLibrary.save_loaded_asset(widget, only_if_is_dirty=False):
    raise RuntimeError('Could not save the editable prompt widget')
widget_class = unreal.EditorAssetLibrary.load_blueprint_class(root + '/WBP_PhoneInteractPrompt')
if not widget_class:
    raise RuntimeError('Widget class did not compile')
config = unreal.load_asset(root + '/DA_PhonePrompt')
config.set_editor_property('widget_class', widget_class)
config.set_editor_property('draw_size', unreal.Vector2D(112.0, 120.0))
if not unreal.EditorAssetLibrary.save_loaded_asset(config, only_if_is_dirty=False):
    raise RuntimeError('Could not save DA_PhonePrompt')
phone_blueprint = unreal.load_asset(root + '/BP_MysteryPhone')
phone_class = unreal.EditorAssetLibrary.load_blueprint_class(root + '/BP_MysteryPhone')
phone_default = unreal.get_default_object(phone_class)
phone_widget = phone_default.get_editor_property('prompt_widget')
if not phone_widget:
    raise RuntimeError('BP_MysteryPhone does not expose its authored PromptWidget component')
phone_widget.set_editor_property('widget_class', widget_class)
if not unreal.EditorAssetLibrary.save_loaded_asset(phone_blueprint, only_if_is_dirty=False):
    raise RuntimeError('Could not save BP_MysteryPhone')
unreal.log('PHONE_PROMPT_AUTHORED_READY widget=' + widget.get_path_name())
