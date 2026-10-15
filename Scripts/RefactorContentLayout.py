"""Move ChopIt assets by Unreal class while retaining their thematic subfolders.

Run as an Unreal Python commandlet. Set CHOPIT_APPLY_CONTENT_MOVES=1 to apply;
otherwise this writes only the move plan to Saved/ContentMovePlan.json.
"""

import json
import os
from pathlib import Path

import unreal


ROOT = "/Game/NewChopIt/"
MODEL_CLASSES = {"StaticMesh", "SkeletalMesh", "Skeleton", "PhysicsAsset"}
MATERIAL_CLASSES = {"Material", "MaterialInstanceConstant", "MaterialFunction"}
TEXTURE_CLASSES = {"Texture2D", "TextureCube"}


def destination(relative_path, asset_class):
    name = relative_path.rsplit("/", 1)[-1]

    if relative_path.startswith("Art/Models/"):
        subject = relative_path.removeprefix("Art/Models/")
        if asset_class in MODEL_CLASSES:
            return None
        if subject.startswith("Character/Textures/"):
            subject = "Character/" + subject.removeprefix("Character/Textures/")
        if asset_class in MATERIAL_CLASSES:
            return "Art/Materials/" + subject
        if asset_class in TEXTURE_CLASSES:
            return "Art/Textures/" + subject
        if asset_class == "NiagaraSystem":
            return "Art/FX/" + subject
        if asset_class in {"SoundWave", "SoundCue", "MetaSoundSource"}:
            return "Audio/" + subject
        if asset_class == "WidgetBlueprint":
            return "Art/UI/" + subject
        if asset_class in {"Blueprint", "AnimBlueprint"}:
            return "BP/" + subject
        if asset_class == "AnimSequence":
            return "Art/Animations/" + subject
        if name.startswith("DA_"):
            return "Data/" + subject
        raise RuntimeError(f"Unclassified model-folder asset: {relative_path} ({asset_class})")

    if relative_path.startswith("Art/Materials/") and asset_class in TEXTURE_CLASSES:
        return "Art/Textures/" + relative_path.removeprefix("Art/Materials/")
    if relative_path.startswith("Art/Blockout/Materials/"):
        return "Art/Materials/Blockout/" + name
    if relative_path.startswith("Camera/Materials/"):
        return "Art/Materials/Camera/" + name
    if relative_path == "Art/MockUp/Mockup" and asset_class in TEXTURE_CLASSES:
        return "Art/Textures/Mockups/Mockup"
    if relative_path == "Characters/Blueprints/BP_ChopItCharacter":
        return "BP/Character/BP_ChopItCharacter"
    if relative_path.startswith("ChainLab/"):
        if asset_class == "Blueprint":
            return "BP/ChainLab/" + name
        if name.startswith("DA_"):
            return "Data/ChainLab/" + name
    if relative_path == "Terrain/L_Terrain_Phase2":
        return "Maps/Terrain/L_Terrain_Phase2"
    if relative_path == "Data/DA_Terrain":
        return "Data/Terrain/DA_Terrain"
    return None


registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path(ROOT.rstrip("/"), recursive=True)
plan = []
for data in assets:
    old_path = str(data.package_name)
    asset_class = str(data.asset_class_path.asset_name)
    relative_path = old_path.removeprefix(ROOT)
    new_relative_path = destination(relative_path, asset_class)
    if new_relative_path and new_relative_path != relative_path:
        plan.append({"old": old_path, "new": ROOT + new_relative_path, "class": asset_class})

plan.sort(key=lambda move: (move["class"] == "World", move["old"]))
old_names = {move["old"] for move in plan}
new_names = [move["new"] for move in plan]
if len(new_names) != len(set(new_names)):
    raise RuntimeError("Two assets would collide at the same destination")
for new_name in new_names:
    if new_name not in old_names and unreal.EditorAssetLibrary.does_asset_exist(new_name):
        raise RuntimeError("Destination already exists: " + new_name)

saved = Path(unreal.Paths.project_saved_dir())
plan_path = saved / "ContentMovePlan.json"
plan_path.write_text(json.dumps(plan, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
unreal.log(f"CONTENT REFACTOR planned {len(plan)} asset moves: {plan_path}")

if os.environ.get("CHOPIT_APPLY_CONTENT_MOVES") == "1":
    results = []
    for move in plan:
        new_directory = move["new"].rsplit("/", 1)[0]
        unreal.EditorAssetLibrary.make_directory(new_directory)
        success = unreal.EditorAssetLibrary.rename_asset(move["old"], move["new"])
        results.append({**move, "success": bool(success)})
        if not success:
            break
    result_path = saved / "ContentMoveResults.json"
    result_path.write_text(json.dumps(results, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    if len(results) != len(plan) or not all(result["success"] for result in results):
        raise RuntimeError(f"Only {sum(r['success'] for r in results)}/{len(plan)} moves succeeded; see {result_path}")
    unreal.log(f"CONTENT REFACTOR moved {len(results)} assets: {result_path}")
