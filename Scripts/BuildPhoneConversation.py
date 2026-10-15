"""Author the phone offer screen and its editable three-upgrade sequences."""
import unreal

root = '/Game/NewChopIt/Art/Models/Phone/'
bargain_root = root + 'Bargains'
tools = unreal.AssetToolsHelpers.get_asset_tools()


def modifier(stat, operation, magnitude):
    item = unreal.ChopItStatModifier()
    item.set_editor_property('stat', getattr(unreal.ChopItCombatStat, stat))
    item.set_editor_property('operation', getattr(unreal.ChopItModifierOperation, operation))
    item.set_editor_property('magnitude', magnitude)
    return item


# Name, benefit, price, max-health factor, (stat, operation, factor) tuples.
# Strong tradeoffs make each call change how the player approaches combat or harvesting.
bargains = [
    ('HachaDeVidrio', 'Hacha de vidrio', 'Daño x2,5.', 'Vida máxima -40%.', .60,
     [('DAMAGE', 'MULTIPLY', 2.5)]),
    ('SierraFrenetica', 'Sierra frenética', 'Ataques x2,4.', 'Daño -40%; alcance -25%.', 1.,
     [('ATTACK_SPEED', 'MULTIPLY', 2.4), ('DAMAGE', 'MULTIPLY', .6), ('RANGE', 'MULTIPLY', .75)]),
    ('BrazoInfinito', 'Brazo infinito', 'Alcance x2,2.', 'Movimiento -45%; ataques -25%.', 1.,
     [('RANGE', 'MULTIPLY', 2.2), ('MOVEMENT_SPEED', 'MULTIPLY', .55), ('ATTACK_SPEED', 'MULTIPLY', .75)]),
    ('FiloHambriento', 'Filo hambriento', 'Probabilidad crítica +65 puntos.', 'Daño normal -50%; vida máxima -20%.', .80,
     [('CRITICAL_CHANCE', 'ADD', .65), ('DAMAGE', 'MULTIPLY', .5)]),
    ('TroncosMalditos', 'Troncos malditos', 'Madera recogida x2, hasta llenar la mochila.', 'Daño -45%; movimiento -20%.', 1.,
     [('WOOD_YIELD', 'MULTIPLY', 2.), ('DAMAGE', 'MULTIPLY', .55), ('MOVEMENT_SPEED', 'MULTIPLY', .8)]),
    ('PiesPrestados', 'Pies prestados', 'Movimiento x1,8.', 'Alcance -50%; vida máxima -25%.', .75,
     [('MOVEMENT_SPEED', 'MULTIPLY', 1.8), ('RANGE', 'MULTIPLY', .5)]),
    ('VerdugoCercano', 'Verdugo cercano', 'Daño x2; movimiento +20%.', 'Alcance -60%.', 1.,
     [('DAMAGE', 'MULTIPLY', 2.), ('MOVEMENT_SPEED', 'MULTIPLY', 1.2), ('RANGE', 'MULTIPLY', .4)]),
    ('FortunaDeSangre', 'Fortuna de sangre', 'Suerte +100 puntos.', 'Vida máxima -50%; daño -20%.', .5,
     [('LUCK', 'ADD', 100.), ('DAMAGE', 'MULTIPLY', .8)]),
    ('UltimoAliento', 'Último aliento', 'Daño x2; ataques x1,5.', 'Vida máxima -55%; movimiento -20%.', .45,
     [('DAMAGE', 'MULTIPLY', 2.), ('ATTACK_SPEED', 'MULTIPLY', 1.5), ('MOVEMENT_SPEED', 'MULTIPLY', .8)]),
    ('GuardiaInmovil', 'Guardia inmóvil', 'Vida máxima x2; alcance +30%.', 'Movimiento -55%; ataques -30%.', 2.,
     [('RANGE', 'MULTIPLY', 1.3), ('MOVEMENT_SPEED', 'MULTIPLY', .45), ('ATTACK_SPEED', 'MULTIPLY', .7)]),
]

for bargain_id, name, benefit, cost, health_factor, modifier_specs in bargains:
    asset_name = 'DA_PhoneBargain_' + bargain_id
    path = bargain_root + '/' + asset_name
    asset = unreal.load_asset(path)
    if not asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', unreal.ChopItPhoneBargainDefinition)
        asset = tools.create_asset(asset_name, bargain_root, unreal.ChopItPhoneBargainDefinition, factory)
    if not isinstance(asset, unreal.ChopItPhoneBargainDefinition):
        raise RuntimeError('Could not create ' + path)
    asset.set_editor_property('bargain_id', bargain_id)
    asset.set_editor_property('display_name', name)
    asset.set_editor_property('benefit_text', benefit)
    asset.set_editor_property('cost_text', cost)
    asset.set_editor_property('max_health_multiplier', health_factor)
    asset.set_editor_property('modifiers', [modifier(*spec) for spec in modifier_specs])
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Could not save ' + path)

material_path = root + 'M_PhonePortrait_UI'
material = unreal.load_asset(material_path)
if not material:
    material = unreal.EditorAssetLibrary.duplicate_asset(
        '/Game/NewChopIt/Art/Models/Chest/M_ChestReveal_UI', material_path)
if not material:
    raise RuntimeError('Could not create the phone UMG portrait material')
material.set_editor_property('material_domain', unreal.MaterialDomain.MD_UI)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)

if not unreal.ChopItPhoneWidgetEditorLibrary.build_editable_phone_dialogue():
    raise RuntimeError('Phone dialogue Widget Blueprints failed to compile')
for name in ['WBP_PhonePresentation', 'WBP_PhoneConversation']:
    if not unreal.EditorAssetLibrary.save_asset(root + name, only_if_is_dirty=False):
        raise RuntimeError('Could not save ' + name)
widget_class = unreal.EditorAssetLibrary.load_blueprint_class(root + 'WBP_PhoneConversation')
if not widget_class:
    raise RuntimeError('Missing authored dialogue class')

for variant in ['Devil', 'Angel']:
    sequence = unreal.load_asset(root + 'DA_PhoneDialogue' + variant)
    sequence.set_editor_property('widget_class', widget_class)
    sequence.set_editor_property('pause_world', True)
    sequence.set_editor_property('block_gameplay_input', True)
    offer = next((line for line in sequence.get_editor_property('lines')
                  if str(line.get_editor_property('line_id')) == 'Offer'), None)
    if not offer:
        raise RuntimeError('Missing Offer line for ' + variant)
    opening = ('Te escuché cortar. <color value="#FF8A24"><shake amp="0.8">Muy bien.</shake></color> '
               'Elegí tu poder. <color value="#FF682A">Leé el precio antes de aceptar.</color>' if variant == 'Devil' else
               'Tus manos aún pueden salvar este lugar. '
               '<color value="#F5D77B">Toda ayuda exige un sacrificio.</color> Elegí con cuidado.')
    offer.set_editor_property('text', opening)
    offer.set_editor_property('next_line_id', '')
    choices = []
    for bargain_id in ['HachaDeVidrio', 'SierraFrenetica', 'BrazoInfinito']:
        bargain = unreal.load_asset(bargain_root + '/DA_PhoneBargain_' + bargain_id)
        if not bargain:
            raise RuntimeError('Missing bargain ' + bargain_id)
        choice = unreal.ChopItDialogueChoice()
        choice.set_editor_property('choice_id', bargain.get_editor_property('bargain_id'))
        choice.set_editor_property('text', bargain.get_editor_property('display_name'))
        choice.set_editor_property('description', 'BENEFICIO: ' + str(bargain.get_editor_property('benefit_text')) +
                                   '\nPRECIO: ' + str(bargain.get_editor_property('cost_text')))
        choice.set_editor_property('next_line_id', '')
        choices.append(choice)
    offer.set_editor_property('choices', choices)
    sequence.set_editor_property('entry_line_id', 'Offer')
    sequence.set_editor_property('can_cancel', True)
    sequence.set_editor_property('lines', [offer])
    if not unreal.EditorAssetLibrary.save_loaded_asset(sequence, only_if_is_dirty=False):
        raise RuntimeError('Could not save phone conversation ' + variant)
unreal.log('PHONE_CONVERSATION_READY')

if '-PhoneDialoguePreview' in unreal.SystemLibrary.get_command_line():
    if not unreal.ChopItPhoneWidgetEditorLibrary.export_phone_dialogue_preview():
        raise RuntimeError('Could not render phone dialogue preview')
    unreal.log('PHONE_DIALOGUE_PREVIEW_READY')
