"""Create the editable call asset and a simple original double-bell test sound. Safe to rerun."""
import math
import os
import struct
import wave
import unreal

ROOT = '/Game/NewChopIt/Art/Models/Phone'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
sound_path = ROOT + '/S_PhoneRing'
source_path = os.path.join(unreal.Paths.project_dir(), 'SourceArt', 'Phone', 'PhoneRing.wav')

if not os.path.exists(source_path):
    os.makedirs(os.path.dirname(source_path), exist_ok=True)
    rate = 22050
    duration = 2.4
    samples = []
    for i in range(int(rate * duration)):
        t = i / rate
        pulse_time = t % 1.2
        bell = max(0.0, 1.0 - pulse_time / 0.43) ** 1.4 if pulse_time < 0.43 else 0.0
        attack = min(1.0, pulse_time / 0.008)
        # Two detuned metal partials give a compact antique-phone bell timbre.
        signal = (math.sin(2 * math.pi * 440 * t) +
                  0.60 * math.sin(2 * math.pi * 447 * t) +
                  0.28 * math.sin(2 * math.pi * 880 * t))
        value = max(-1.0, min(1.0, signal * bell * attack * 0.34))
        samples.append(struct.pack('<h', int(value * 32767)))
    with wave.open(source_path, 'wb') as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(rate)
        output.writeframes(b''.join(samples))

sound = unreal.load_asset(sound_path)
if not sound:
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', source_path)
    task.set_editor_property('destination_path', ROOT)
    task.set_editor_property('destination_name', 'S_PhoneRing')
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    task.set_editor_property('replace_existing', False)
    TOOLS.import_asset_tasks([task])
    sound = unreal.load_asset(sound_path)
if not sound:
    raise RuntimeError('Could not import phone ring sound')
sound.set_editor_property('looping', True)
unreal.EditorAssetLibrary.save_loaded_asset(sound, only_if_is_dirty=False)

config_path = ROOT + '/DA_PhoneCall'
config = unreal.load_asset(config_path)
if not config:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.ChopItPhoneCallConfig)
    config = TOOLS.create_asset('DA_PhoneCall', ROOT, unreal.ChopItPhoneCallConfig, factory)
if not isinstance(config, unreal.ChopItPhoneCallConfig):
    raise RuntimeError('Wrong type at ' + config_path)
if not config.get_editor_property('ring_sound'):
    config.set_editor_property('ring_sound', sound)
if not unreal.EditorAssetLibrary.save_loaded_asset(config, only_if_is_dirty=False):
    raise RuntimeError('Could not save ' + config_path)
unreal.log('PHONE_STAGE2_ASSETS_READY ' + config.get_path_name())
