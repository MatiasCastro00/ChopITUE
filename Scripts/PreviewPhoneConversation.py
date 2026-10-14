"""Run with -ExecCmds=py ... so Slate can advance between preview frames."""
import unreal

frames = 0

def tick(delta):
    global frames
    frames += 1
    if frames in (10, 20, 30):
        if not unreal.ChopItPhoneWidgetEditorLibrary.export_phone_dialogue_preview():
            unreal.log_error('Phone dialogue preview failed')
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.SystemLibrary.quit_editor()
            return
    if frames >= 30:
        unreal.log('PHONE_DIALOGUE_PREVIEW_READY')
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()

handle = unreal.register_slate_post_tick_callback(tick)
