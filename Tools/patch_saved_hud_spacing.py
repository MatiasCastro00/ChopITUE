import unreal
path = '/Game/ChopIt/UI/WBP_PSX_HUD'
asset = unreal.load_asset(path)
def widget(name):
    result = unreal.load_object(None, path + '.WBP_PSX_HUD:WidgetTree.' + name)
    assert result, name
    return result
health = widget('HealthFill')
health.slot.set_size(unreal.Vector2D(156, health.slot.get_size().y))
quota = widget('QuotaFill')
empty = widget('QuotaLog')
quota.slot.set_layout(empty.slot.get_layout())
assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
unreal.log('HUD_PATCH_OK health_width=156 quota_matches_empty=True')
