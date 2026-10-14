"""Create editable placeholder phone conversations using the existing dialogue system."""
import unreal

ROOT = '/Game/ChopIt/Phone'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def data_asset(name, cls):
    path = ROOT + '/' + name
    asset = unreal.load_asset(path)
    if asset:
        if not isinstance(asset, cls):
            raise RuntimeError('Wrong type at ' + path)
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = TOOLS.create_asset(name, ROOT, cls, factory)
    if not asset:
        raise RuntimeError('Could not create ' + path)
    return asset, True


def choice(choice_id, label, target):
    item = unreal.ChopItDialogueChoice()
    item.set_editor_property('choice_id', choice_id)
    item.set_editor_property('text', label)
    item.set_editor_property('next_line_id', target)
    return item


def line(line_id, speaker, text, next_line='', choices=None):
    item = unreal.ChopItDialogueLine()
    item.set_editor_property('line_id', line_id)
    item.set_editor_property('speaker', speaker)
    item.set_editor_property('text', text)
    item.set_editor_property('next_line_id', next_line)
    item.set_editor_property('choices', choices or [])
    item.set_editor_property('typewriter_speed', 1.0)
    return item


theme = unreal.load_asset('/Game/ChopIt/Dialogue/DA_DialogueTheme_WoodMetal')
if not theme:
    raise RuntimeError('Existing dialogue theme is missing')

variants = [
    ('Devil', 'Diabólico', 'Una voz cálida llega desde un lugar imposible. Te ofrece poder, pero aún no revela el precio.',
     'La oferta promete fuerza extraordinaria. Es solo una vista previa; todavía no concede mejoras.',
     'Todo pacto tiene una deuda. Las consecuencias aparecerán en una etapa futura.'),
    ('Angel', 'Angelical', 'Una voz serena cruza la línea. Te ofrece ayuda para sobrevivir a este bosque.',
     'La oferta promete una bendición. Es solo una vista previa; todavía no concede mejoras.',
     'No hay una penalización prevista para esta variante, pero sus efectos aún no están implementados.'),
]

for variant_id, display_name, opening, benefits, consequences in variants:
    speaker, fresh_speaker = data_asset('DA_PhoneSpeaker' + variant_id, unreal.ChopItDialogueSpeakerDefinition)
    if fresh_speaker:
        speaker.set_editor_property('speaker_id', variant_id)
        speaker.set_editor_property('display_name', display_name)
        speaker.set_editor_property('accent_color', unreal.LinearColor(1.0, 0.16, 0.08, 1.0)
                                    if variant_id == 'Devil' else unreal.LinearColor(0.82, 0.78, 1.0, 1.0))
    unreal.EditorAssetLibrary.save_loaded_asset(speaker, only_if_is_dirty=False)

    sequence, fresh_sequence = data_asset('DA_PhoneDialogue' + variant_id, unreal.ChopItDialogueSequence)
    if fresh_sequence or not sequence.get_editor_property('lines'):
        sequence.set_editor_property('dialogue_id', 'Phone' + variant_id)
        sequence.set_editor_property('theme', theme)
        sequence.set_editor_property('entry_line_id', 'Offer')
        sequence.set_editor_property('pause_world', True)
        sequence.set_editor_property('block_gameplay_input', True)
        sequence.set_editor_property('can_cancel', True)
        sequence.set_editor_property('lines', [
            line('Offer', speaker, opening, choices=[
                choice('Accept', 'Aceptar', 'Accepted'),
                choice('Reject', 'Rechazar', 'Rejected'),
                choice('Benefits', 'Mostrar beneficios', 'Benefits'),
                choice('Consequences', 'Mostrar consecuencias', 'Consequences'),
            ]),
            line('Benefits', speaker, benefits, next_line='Offer'),
            line('Consequences', speaker, consequences, next_line='Offer'),
            line('Accepted', speaker, 'Entendido. Por ahora, esta llamada termina sin aplicar recompensas.'),
            line('Rejected', speaker, 'La línea se corta. Quizá vuelvas a escucharla otro día.'),
        ])
    unreal.EditorAssetLibrary.save_loaded_asset(sequence, only_if_is_dirty=False)

    variant = unreal.load_asset(ROOT + '/DA_Phone' + variant_id)
    if not variant:
        raise RuntimeError('Variant missing: ' + variant_id)
    if not variant.get_editor_property('dialogue_sequence'):
        variant.set_editor_property('dialogue_sequence', sequence)
    unreal.EditorAssetLibrary.save_loaded_asset(variant, only_if_is_dirty=False)

unreal.log('PHONE_STAGE6_ASSETS_READY')
