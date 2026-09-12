import unreal

root = '/Game/ChopIt/Art/Character'
task = unreal.AssetImportTask()
task.filename = unreal.Paths.project_content_dir() + 'ChopIt/Art/Character/Textures/Lumberjack_BaseColor.png'
task.destination_path = root + '/Textures'
task.destination_name = 'Lumberjack_BaseColor'
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset(root + '/Textures/Lumberjack_BaseColor')
assert isinstance(texture, unreal.Texture2D)
texture.set_editor_property('srgb', True)
material = unreal.load_asset(root + '/M_Lumberjack')
if not material:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_Lumberjack', root, unreal.Material, unreal.MaterialFactoryNew())
    sample = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample, -400, 0)
    sample.texture = texture
    unreal.MaterialEditingLibrary.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 200)
    roughness.set_editor_property('r', 0.8)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
material.set_editor_property('used_with_skeletal_mesh', True)
unreal.MaterialEditingLibrary.recompile_material(material)
mesh = unreal.load_asset(root + '/Imported/c85471db_c635_45fc_bae4_6171d60be24f1')
assert isinstance(mesh, unreal.SkeletalMesh)
slots = mesh.get_editor_property('materials')
assert len(slots) == 1, 'Unexpected material slots; inspect before assigning'
old_slot = slots[0]
new_slot = unreal.SkeletalMaterial()
new_slot.set_editor_property('material_interface', material)
new_slot.set_editor_property('material_slot_name', old_slot.get_editor_property('material_slot_name'))
mesh.modify()
mesh.set_editor_property('materials', [new_slot])
for asset in (texture, material, mesh):
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
unreal.SystemLibrary.collect_garbage()
mesh = unreal.load_asset(root + '/Imported/c85471db_c635_45fc_bae4_6171d60be24f1')
assert mesh.get_editor_property('materials')[0].get_editor_property('material_interface') == material
unreal.log_warning('LUMBERJACK_MATERIAL_OK: packed 2048 texture imported and assigned to skeletal mesh')
