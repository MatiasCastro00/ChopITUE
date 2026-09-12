"""Read-only check that the authored PSX HUD labels are in English."""
import unreal

blueprint = unreal.load_asset('/Game/ChopIt/UI/WBP_PSX_HUD')
tree = blueprint.get_editor_property('widget_tree')
expected = {
    'QuotaText': 'QUOTA   120 / 200',
    'DayText': 'DAY 1',
    'MissionHeader': 'MISSIONS',
    'MissionTitle': 'Feed the Furnace',
    'MissionDescription': 'Deliver the wood quota\nBefore the day ends',
    'LevelText': 'LEVEL 3',
}
for name, value in expected.items():
    widget = tree.find_widget(name)
    assert widget.get_text().to_string() == value, '{}: {}'.format(name, widget.get_text())
unreal.log('PSX_HUD_ENGLISH_VERIFIED {}'.format(', '.join(expected)))
