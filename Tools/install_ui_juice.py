"""Run with the editor closed after compiling ChopItEditor. Preserves the authored tree."""
import math
import random
import struct
import wave
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source = root / 'SourceArt' / 'UI' / 'Audio'
source.mkdir(parents=True, exist_ok=True)
specs = {'Pickup':(740,.09), 'Reject':(120,.14), 'Delivery':(330,.12),
         'Damage':(85,.18), 'Heal':(880,.24), 'Money':(1100,.16),
         'Spend':(310,.13), 'Tick':(1500,.035), 'Expire':(70,.35),
         'Mission':(440,.28), 'Complete':(660,.42), 'Level':(990,.45)}
tasks=[]
for index,(name,(frequency,duration)) in enumerate(specs.items()):
    filename=source / ('UI_'+name+'.wav')
    if not filename.exists():
        rng=random.Random(531+index)
        samples=[]
        for n in range(int(44100*duration)):
            t=n/44100
            envelope=min(1,t/.004)*max(0,1-t/duration)**2
            pitch=frequency*(1+(.6 if name in ('Heal','Level','Complete') else -.35)*t/duration)
            value=(.7*math.sin(2*math.pi*pitch*t)+.15*rng.uniform(-1,1))*envelope
            samples.append(struct.pack('<h',int(14000*value)))
        with wave.open(str(filename),'wb') as out:
            out.setnchannels(1);out.setsampwidth(2);out.setframerate(44100);out.writeframes(b''.join(samples))
    if not unreal.EditorAssetLibrary.does_asset_exist('/Game/ChopIt/UI/Audio/UI_'+name):
        task=unreal.AssetImportTask();task.filename=str(filename)
        task.destination_path='/Game/ChopIt/UI/Audio';task.automated=True;task.save=True;tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
profile=unreal.load_asset('/Game/ChopIt/UI/DA_UIJuice')
if not profile:
    factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.ChopItUIJuiceProfile)
    profile=unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_UIJuice','/Game/ChopIt/UI',unreal.ChopItUIJuiceProfile,factory)
    profile.set_editor_property('sounds',{name:unreal.load_asset('/Game/ChopIt/UI/Audio/UI_'+name) for name in specs})
    unreal.EditorAssetLibrary.save_loaded_asset(profile,only_if_is_dirty=False)
asset=unreal.load_asset('/Game/ChopIt/UI/WBP_PSX_HUD')
assert asset
unreal.BlueprintEditorLibrary.reparent_blueprint(asset,unreal.ChopItJuiceWidget)
unreal.BlueprintEditorLibrary.compile_blueprint(asset)
unreal.get_default_object(asset.generated_class()).set_editor_property('juice_profile',profile)
assert asset.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
assert unreal.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)
unreal.log('UI_JUICE_INSTALLED')
