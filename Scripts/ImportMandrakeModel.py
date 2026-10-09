import os
import sys
import unreal

ROOT = '/Game/ChopIt/Items/Mandrake'
tools = unreal.AssetToolsHelpers.get_asset_tools()
unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
source = os.path.abspath(os.path.join(unreal.Paths.project_dir(), 'SourceArt', 'Mandrake'))
tasks = []
for filename, name in [('textura.jpg', 'T_Mandrake_Albedo'), ('Mandrake_Recovered.fbx', 'SM_Mandrake')]:
    task = unreal.AssetImportTask()
    task.filename = os.path.join(source, filename)
    task.destination_path = ROOT
    task.destination_name = name
    task.automated = True
    task.save = True
    task.replace_existing = '--reimport' in sys.argv
    if filename.endswith('.fbx'):
        options = unreal.FbxImportUI()
        options.set_editor_property('automated_import_should_detect_type', False)
        options.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)
        options.set_editor_property('import_as_skeletal', False)
        options.set_editor_property('import_materials', False)
        options.set_editor_property('import_textures', False)
        options.static_mesh_import_data.set_editor_property('combine_meshes', True)
        options.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs', False)
        options.static_mesh_import_data.set_editor_property('auto_generate_collision', False)
        task.options = options
    if not unreal.EditorAssetLibrary.does_asset_exist(ROOT + '/' + name) or ('--reimport' in sys.argv and filename.endswith('.fbx')):
        tasks.append(task)
tools.import_asset_tasks(tasks)
mesh = unreal.load_asset(ROOT + '/SM_Mandrake')
if not mesh:
    raise RuntimeError('Mandrake mesh import failed')
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
settings=editor.get_lod_build_settings(mesh,0)
settings.set_editor_property('build_scale3d',unreal.Vector(100,100,100))
settings.set_editor_property('recompute_normals',True)
settings.set_editor_property('recompute_tangents',True)
settings.set_editor_property('use_full_precision_u_vs',True)
editor.set_lod_build_settings(mesh,0,settings)
unreal.log('MANDRAKE_BOUNDS ' + str(mesh.get_bounds()))
for i, slot in enumerate(mesh.get_editor_property('static_materials')):
    unreal.log('MANDRAKE_SLOT {} {}'.format(i, slot.material_slot_name))
unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
