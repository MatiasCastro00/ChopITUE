import unreal


ROOT = "/Game/ChopIt/Art/Character"
UPDATED = ROOT + "/Updated"
MESH_PATH = UPDATED + "/c85471db_c635_45fc_bae4_6171d60be24f"
ANIMATION_PATHS = [
    UPDATED + "/LumberJackCTRL_COG_Idle_Respiracion_96",
    UPDATED + "/LumberJackCTRL_COG_Run_Heavy_650_14",
    UPDATED + "/LumberJackCTRL_COG_Jump_Quick_24",
]

mesh = unreal.load_asset(MESH_PATH)
material = unreal.load_asset(ROOT + "/M_Lumberjack")
if not isinstance(mesh, unreal.SkeletalMesh):
    raise RuntimeError("Updated Lumberjack skeletal mesh is missing")
if not isinstance(material, unreal.MaterialInterface):
    raise RuntimeError("Lumberjack material is missing")

slots = mesh.get_editor_property("materials")
if len(slots) != 1:
    raise RuntimeError(f"Expected one Lumberjack material slot, found {len(slots)}")

replacement = unreal.SkeletalMaterial()
replacement.set_editor_property("material_interface", material)
replacement.set_editor_property("material_slot_name", slots[0].get_editor_property("material_slot_name"))
mesh.modify()
mesh.set_editor_property("materials", [replacement])

mesh_skeleton = mesh.get_editor_property("skeleton")
if not mesh_skeleton:
    raise RuntimeError("Updated Lumberjack mesh has no skeleton")

for path in ANIMATION_PATHS:
    animation = unreal.load_asset(path)
    if not isinstance(animation, unreal.AnimSequence):
        raise RuntimeError("Missing required animation: " + path)
    if animation.get_editor_property("skeleton") != mesh_skeleton:
        raise RuntimeError("Animation skeleton mismatch: " + path)
    unreal.log_warning(
        f"LUMBERJACK_ANIMATION_OK: {animation.get_name()} "
        f"duration={animation.get_play_length():.3f}s"
    )

if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
    raise RuntimeError("Failed to save updated Lumberjack mesh")

unreal.log_warning("LUMBERJACK_FINALIZE_OK")
