import unreal


WIDGET_PATH = "/Game/UI/WBP_RhythmGameplay"
widget = unreal.load_asset(WIDGET_PATH)
widget_class = unreal.load_class(None, "{}.WBP_RhythmGameplay_C".format(WIDGET_PATH))
if widget is None or widget_class is None:
    raise RuntimeError("WBP_RhythmGameplay is missing")

defaults = unreal.get_default_object(widget_class)
defaults.set_editor_property("judgement_feedback_vertical_position", 0.16)
defaults.set_editor_property("judgement_feedback_max_size", unreal.Vector2D(620.0, 270.0))
unreal.EditorAssetLibrary.save_loaded_asset(widget, only_if_is_dirty=False)
unreal.log("RHYTHM JUDGEMENT FEEDBACK TUNED: y=0.16, max=620x270")

