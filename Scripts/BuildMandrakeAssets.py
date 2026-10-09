"""Create Mandrake Stick art/audio and activate its existing Data Asset.

Run with UnrealEditor-Cmd -run=pythonscript -script=<absolute path>.
Existing visual assets and item icons are preserved.
"""
import math
import os
import random
import struct
import wave
import unreal

ROOT = '/Game/ChopIt/Items/Mandrake'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

def material(name, rgb, emissive=False):
    path = ROOT + '/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return
    asset = TOOLS.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    color = unreal.MaterialEditingLibrary.create_material_expression(asset, unreal.MaterialExpressionVectorParameter)
    color.set_editor_property('parameter_name', 'Color')
    color.set_editor_property('default_value', unreal.LinearColor(*rgb, 1.0))
    if emissive:
        asset.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
        asset.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
        unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(asset)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

material('M_Mandrake_Root', (.24, .095, .045))
material('M_Mandrake_Leaf', (.09, .65, .12))
material('M_Mandrake_Face', (.008, .014, .006))
material('M_Mandrake_Wave', (.3, 3.0, .22), True)

sound_path = ROOT + '/S_Mandrake_Scream'
if not unreal.EditorAssetLibrary.does_asset_exist(sound_path):
    source_dir = os.path.join(unreal.Paths.project_content_dir(), 'ChopIt', 'Items', 'Mandrake')
    os.makedirs(source_dir, exist_ok=True)
    wav_path = os.path.join(source_dir, 'S_Mandrake_Scream.wav')
    random.seed(924)
    sample_rate = 44100
    duration = 1.25
    samples = bytearray()
    for i in range(int(sample_rate * duration)):
        t = i / sample_rate
        x = t / duration
        envelope = min(1.0, x * 16) * min(1.0, (1.0 - x) * 13)
        pitch = 360 + 160 * math.sin(2 * math.pi * (x * 1.2 + .12)) + 25 * math.sin(2 * math.pi * 11 * t)
        phase = 2 * math.pi * pitch * t
        vowel = math.sin(phase) + .38 * math.sin(2 * phase) + .17 * math.sin(3 * phase)
        rasp = (random.random() * 2 - 1) * .24
        sample = max(-1, min(1, envelope * (vowel + rasp) * .50))
        samples.extend(struct.pack('<h', int(sample * 32767)))
    with wave.open(wav_path, 'wb') as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(sample_rate)
        output.writeframes(samples)
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', wav_path)
    task.set_editor_property('destination_path', ROOT)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    TOOLS.import_asset_tasks([task])
sound = unreal.EditorAssetLibrary.load_asset(sound_path)
if sound:
    sound.set_editor_property('looping', True)
    unreal.EditorAssetLibrary.save_loaded_asset(sound)

item_path = '/Game/ChopIt/Items/DA_Item_MandrakeStick'
item = unreal.EditorAssetLibrary.load_asset(item_path)
if not item:
    raise RuntimeError('Missing ' + item_path)
item.set_editor_property('display_name', 'Palo Mandrágora')
item.set_editor_property('description', 'Al matar a un enemigo, 10% de probabilidad de invocar una mandrágora durante 6 s. Su grito causa 8 de daño por segundo y ralentiza un 35% a los enemigos cercanos. La confusión dura hasta 1 s después del último impacto. Los stacks aumentan la probabilidad con rendimiento decreciente.')
item.set_editor_property('rarity', unreal.ChopItItemRarity.UNCOMMON)
item.set_editor_property('spawn_weight', 1.0)
if not item.get_editor_property('effects'):
    effect = unreal.new_object(unreal.ChopItMandrakeEffect, outer=item, name='MandrakeEffect')
    item.set_editor_property('effects', [effect])
if not unreal.EditorAssetLibrary.save_loaded_asset(item, only_if_is_dirty=False):
    raise RuntimeError('Could not save ' + item_path)
print('Mandrake item and assets ready')
