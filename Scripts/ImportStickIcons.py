"""Import English-named stick PNGs as UI textures and connect existing items.

Run from the Unreal Editor or with -ExecutePythonScript. Safe to run again.
"""

import os
import unreal


SOURCE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "Content", "ChopIt", "Art", "Sticks"))
DESTINATION = "/Game/ChopIt/Art/Sticks"
ITEMS = {
    "Heartwood": "Heartwood",
    "VampireStick": "VampireStick",
    "Wormwood": "Wormwood",
    # The available art names this log HollowLog; it is the only log illustration.
    "GenerousLog": "HollowLog",
}

if not os.path.isdir(SOURCE_DIR):
    raise RuntimeError("Missing stick icon directory: " + SOURCE_DIR)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
for filename in sorted(os.listdir(SOURCE_DIR)):
    if not filename.lower().endswith(".png"):
        continue
    stem = os.path.splitext(filename)[0]
    name = "T_Stick_" + stem
    path = DESTINATION + "/" + name
    texture = unreal.EditorAssetLibrary.load_asset(path)
    if texture is None:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(SOURCE_DIR, filename))
        task.set_editor_property("destination_path", DESTINATION)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("replace_existing", False)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        asset_tools.import_asset_tasks([task])
        texture = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Icon import failed: " + path)
    if texture.get_editor_property("filter") != unreal.TextureFilter.TF_NEAREST:
        texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
            raise RuntimeError("Could not save icon: " + path)
    unreal.log("CHOPIT_STICK_ICON " + texture.get_path_name())

for item_name, icon_name in ITEMS.items():
    item = unreal.EditorAssetLibrary.load_asset("/Game/ChopIt/Items/DA_Item_" + item_name)
    texture = unreal.EditorAssetLibrary.load_asset(DESTINATION + "/T_Stick_" + icon_name)
    if item is None or texture is None:
        raise RuntimeError("Missing item or icon for " + item_name)
    item.set_editor_property("icon", texture)
    if not unreal.EditorAssetLibrary.save_loaded_asset(item):
        raise RuntimeError("Could not save item icon reference: " + item.get_path_name())
    unreal.log("CHOPIT_ITEM_ICON {} -> {}".format(item.get_path_name(), texture.get_path_name()))

del asset_tools
unreal.log("CHOPIT_STICK_ICONS_READY")
