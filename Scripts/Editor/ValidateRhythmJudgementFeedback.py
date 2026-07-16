import unreal


widget_class = unreal.load_class(None, "/Game/UI/WBP_RhythmGameplay.WBP_RhythmGameplay_C")
if widget_class is None:
    raise RuntimeError("WBP_RhythmGameplay class is missing")

defaults = unreal.get_default_object(widget_class)
position = defaults.get_editor_property("judgement_feedback_vertical_position")
size = defaults.get_editor_property("judgement_feedback_max_size")
if abs(position - 0.16) > 0.001:
    raise RuntimeError("Judgement feedback vertical position is incorrect: {}".format(position))
if abs(size.x - 620.0) > 0.01 or abs(size.y - 270.0) > 0.01:
    raise RuntimeError("Judgement feedback max size is incorrect: {}".format(size))
unreal.log("RHYTHM JUDGEMENT FEEDBACK VALIDATED: y={}, max={}x{}".format(
    position, size.x, size.y
))


