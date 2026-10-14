"""Build a lightweight rotary telephone mesh and editable variant materials. Safe to rerun."""
import math
import os
import unreal

ROOT = '/Game/ChopIt/Phone'
SOURCE = os.path.join(unreal.Paths.project_dir(), 'SourceArt', 'Phone')
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
os.makedirs(SOURCE, exist_ok=True)
UPDATED_SOURCES = set()


class Mesh:
    def __init__(self):
        self.vertices = []
        self.faces = []

    def vertex(self, p):
        self.vertices.append(tuple(p))
        return len(self.vertices)

    def tri(self, a, b, c, material):
        self.faces.append((material, (a, b, c)))

    def quad(self, a, b, c, d, material):
        self.tri(a, b, c, material)
        self.tri(a, c, d, material)

    def box(self, center, size, material):
        x, y, z = center
        sx, sy, sz = (v * 0.5 for v in size)
        corners = [(x+dx*sx, y+dy*sy, z+dz*sz)
                   for dz in (-1, 1) for dy in (-1, 1) for dx in (-1, 1)]
        v = [self.vertex(p) for p in corners]
        for a, b, c, d in [(0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4),
                            (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)]:
            self.quad(v[a], v[b], v[c], v[d], material)

    def cylinder(self, center, axis, radius_bottom, radius_top, length, material, sides=16):
        ax = tuple(float(v) for v in axis)
        magnitude = math.sqrt(sum(v*v for v in ax))
        n = tuple(v/magnitude for v in ax)
        reference = (1, 0, 0) if abs(n[0]) < 0.8 else (0, 1, 0)
        u = (n[1]*reference[2]-n[2]*reference[1],
             n[2]*reference[0]-n[0]*reference[2],
             n[0]*reference[1]-n[1]*reference[0])
        um = math.sqrt(sum(v*v for v in u))
        u = tuple(v/um for v in u)
        v = (n[1]*u[2]-n[2]*u[1], n[2]*u[0]-n[0]*u[2], n[0]*u[1]-n[1]*u[0])
        rings = []
        for side, radius in [(-1, radius_bottom), (1, radius_top)]:
            ring = []
            for i in range(sides):
                a = 2*math.pi*i/sides
                ring.append(self.vertex(tuple(center[j] + n[j]*side*length*0.5
                                              + radius*(u[j]*math.cos(a)+v[j]*math.sin(a))
                                              for j in range(3))))
            rings.append(ring)
        bottom_center = self.vertex(tuple(center[j]-n[j]*length*0.5 for j in range(3)))
        top_center = self.vertex(tuple(center[j]+n[j]*length*0.5 for j in range(3)))
        for i in range(sides):
            j = (i + 1) % sides
            self.quad(rings[0][i], rings[0][j], rings[1][j], rings[1][i], material)
            self.tri(bottom_center, rings[0][j], rings[0][i], material)
            self.tri(top_center, rings[1][i], rings[1][j], material)

    def bevel_ring(self, width, depth, radius, z):
        x, y = width*0.5, depth*0.5
        return [self.vertex((px, py, z)) for px, py in [
            (-x+radius, -y), (x-radius, -y), (x, -y+radius), (x, y-radius),
            (x-radius, y), (-x+radius, y), (-x, y-radius), (-x, -y+radius)]]

    def body(self):
        rings = [self.bevel_ring(*row) for row in [
            (65, 43, 6, -16), (72, 49, 7, -12), (68, 46, 7, 7),
            (58, 40, 7, 15), (53, 36, 6, 17)]]
        for low, high in zip(rings, rings[1:]):
            for i in range(8):
                j = (i + 1) % 8
                self.quad(low[i], low[j], high[j], high[i], 'Shell')
        top = self.vertex((0, 0, 17))
        bottom = self.vertex((0, 0, -16))
        for i in range(8):
            j = (i + 1) % 8
            self.tri(top, rings[-1][i], rings[-1][j], 'Shell')
            self.tri(bottom, rings[0][j], rings[0][i], 'Dark')
        # Rotary dial: angled bezel, ivory face, ten inset number recesses.
        axis = (0, -0.27, 0.963)
        self.cylinder((0, -4, 18), axis, 15.8, 15.8, 2.0, 'Metal', 32)
        self.cylinder((0, -4.4, 19.4), axis, 13.5, 13.5, 1.0, 'Ivory', 32)
        for i in range(10):
            angle = 2*math.pi*i/10 - math.pi*0.5
            dx = 9.9*math.cos(angle)
            dy = 9.9*math.sin(angle)
            pos = (dx, -4.4 + 0.963*dy - 0.27*1.2, 19.4 + 0.27*dy + 0.963*1.2)
            self.cylinder(pos, axis, 2.65, 2.65, 0.45, 'Dark', 12)
            raised = tuple(pos[j] + axis[j]*0.34 for j in range(3))
            self.cylinder(raised, axis, 1.45, 1.45, 0.35, 'Ivory', 12)
        self.cylinder((0, -4.8, 21.0), axis, 3.8, 3.8, 0.8, 'Metal', 24)
        # Twin handset rests and black rubber feet.
        for x in (-25, 25):
            self.box((x, 11, 20), (9, 12, 6), 'Shell')
            self.box((x, 11, 23.5), (10, 13, 2), 'Dark')
        for x in (-28, 28):
            for y in (-17, 17):
                self.cylinder((x, y, -17.3), (0, 0, 1), 3.8, 3.8, 3.0, 'Dark', 10)
        self.box((0, -23.7, -2), (19, 1.6, 4), 'Metal')

    def handset(self):
        # Slightly arched grip joining two wide ear pieces.
        for start, end in [((-25, 0, 0), (-12, 0, 4)),
                           ((-12, 0, 4), (12, 0, 4)),
                           ((12, 0, 4), (25, 0, 0))]:
            delta = tuple(end[i]-start[i] for i in range(3))
            center = tuple((end[i]+start[i])*0.5 for i in range(3))
            self.cylinder(center, delta, 4.8, 4.8, math.sqrt(sum(v*v for v in delta)), 'Shell', 14)
        for x in (-29, 29):
            self.cylinder((x, 0, -3.5), (0, 0, 1), 9.7, 10.8, 8.0, 'Shell', 20)
            self.cylinder((x, 0, -7.7), (0, 0, 1), 9.0, 9.0, 0.8, 'Metal', 20)
            self.cylinder((x, 0, -8.2), (0, 0, 1), 7.5, 7.5, 0.5, 'Dark', 20)

    def save(self, filename):
        path = os.path.join(SOURCE, filename)
        if os.path.exists(path):
            # UE 5.8's OBJ translator requires valid UV indices on every polygon.
            with open(path, 'r', encoding='ascii') as obj:
                original = obj.read()
            if '\nvt ' not in original:
                updated = []
                for row in original.splitlines():
                    updated.append(row)
                    if row.startswith('mtllib '):
                        updated.extend(['vt 0 0', 'vt 1 0', 'vt 0 1'])
                updated = ['f ' + ' '.join(vertex + '/' + str(index)
                           for index, vertex in enumerate(row.split()[1:], 1))
                           if row.startswith('f ') else row for row in updated]
                with open(path, 'w', encoding='ascii') as obj:
                    obj.write('\n'.join(updated) + '\n')
                UPDATED_SOURCES.add(path)
            return path
        with open(path, 'w', encoding='ascii') as obj:
            obj.write('mtllib RotaryPhone.mtl\n')
            obj.write('vt 0 0\nvt 1 0\nvt 0 1\n')
            for p in self.vertices:
                obj.write('v {:.5f} {:.5f} {:.5f}\n'.format(*p))
            current = None
            for material, face in self.faces:
                if material != current:
                    obj.write('usemtl {}\n'.format(material))
                    current = material
                obj.write('f {}/1 {}/2 {}/3\n'.format(*face))
        UPDATED_SOURCES.add(path)
        return path


mtl_path = os.path.join(SOURCE, 'RotaryPhone.mtl')
if not os.path.exists(mtl_path):
    with open(mtl_path, 'w', encoding='ascii') as mtl:
        for name, rgb in [('Shell', (0.62, 0.05, 0.04)), ('Dark', (0.025, 0.018, 0.016)),
                          ('Metal', (0.64, 0.43, 0.16)), ('Ivory', (0.87, 0.78, 0.62))]:
            mtl.write('newmtl {}\nKd {} {} {}\n\n'.format(name, *rgb))

base = Mesh()
base.body()
receiver = Mesh()
receiver.handset()
base_path = base.save('SM_RotaryPhoneBase.obj')
receiver_path = receiver.save('SM_RotaryPhoneReceiver.obj')


def material(name, rgb, roughness=0.52, metallic=0.0):
    path = ROOT + '/' + name
    asset = unreal.load_asset(path)
    if asset:
        return asset
    asset = TOOLS.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if not asset:
        raise RuntimeError('Could not create ' + name)
    color = unreal.MaterialEditingLibrary.create_material_expression(asset, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant', unreal.LinearColor(*rgb, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(asset, unreal.MaterialExpressionConstant)
    rough.set_editor_property('r', roughness)
    unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    metal = unreal.MaterialEditingLibrary.create_material_expression(asset, unreal.MaterialExpressionConstant)
    metal.set_editor_property('r', metallic)
    unreal.MaterialEditingLibrary.connect_material_property(metal, '', unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.recompile_material(asset)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    return asset


materials = {
    'Shell': material('M_PhoneShellRed', (0.57, 0.022, 0.016)),
    'Dark': material('M_PhoneRubber', (0.018, 0.013, 0.012), 0.86),
    'Metal': material('M_PhoneBrass', (0.55, 0.34, 0.10), 0.32, 0.68),
    'Ivory': material('M_PhoneDialIvory', (0.78, 0.68, 0.53), 0.7),
}
angel_shell = material('M_PhoneShellAngel', (0.72, 0.66, 0.55), 0.42)
devil_shell = material('M_PhoneShellDevil', (0.30, 0.004, 0.012), 0.32)


def import_mesh(filename, asset_name):
    path = ROOT + '/' + asset_name
    asset = unreal.load_asset(path)
    if not asset or filename in UPDATED_SOURCES:
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', filename)
        task.set_editor_property('destination_path', ROOT)
        task.set_editor_property('destination_name', asset_name)
        task.set_editor_property('automated', True)
        task.set_editor_property('replace_existing', bool(asset))
        task.set_editor_property('save', True)
        TOOLS.import_asset_tasks([task])
        asset = unreal.load_asset(path)
    if not isinstance(asset, unreal.StaticMesh):
        raise RuntimeError('OBJ import failed for ' + filename)
    slots = asset.get_editor_property('static_materials')
    unreal.log('PHONE_MESH_SLOTS {} {}'.format(asset_name,
               [str(slot.material_slot_name) for slot in slots]))
    required = {'Shell', 'Dark', 'Metal'} | ({'Ivory'} if asset_name.endswith('Base') else set())
    if not required.issubset({str(slot.material_slot_name) for slot in slots}):
        raise RuntimeError('Missing material slots on ' + asset_name)
    for index, slot in enumerate(slots):
        slot_name = str(slot.material_slot_name)
        if slot_name not in materials:
            raise RuntimeError('Unexpected material slot ' + slot_name)
        asset.set_material(index, materials[slot_name])
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    return asset


base_asset = import_mesh(base_path, 'SM_RotaryPhoneBase')
receiver_asset = import_mesh(receiver_path, 'SM_RotaryPhoneReceiver')

for variant_id, shell in [('Devil', devil_shell), ('Angel', angel_shell)]:
    asset = unreal.load_asset(ROOT + '/DA_Phone' + variant_id)
    if not asset:
        raise RuntimeError('Missing phone variant ' + variant_id)
    if not asset.get_editor_property('base_mesh'):
        asset.set_editor_property('base_mesh', base_asset)
    if not asset.get_editor_property('receiver_mesh'):
        asset.set_editor_property('receiver_mesh', receiver_asset)
    if not asset.get_editor_property('base_material'):
        asset.set_editor_property('base_material', shell)
    if not asset.get_editor_property('receiver_material'):
        asset.set_editor_property('receiver_material', shell)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

unreal.log('PHONE_STAGE7_ASSETS_READY')
