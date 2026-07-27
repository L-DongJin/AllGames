import unreal


def path(value):
    return value.get_path_name() if value else "None"


def main():
    cls = unreal.load_class(None, "/Game/UI/WBP_IdolQuizRoom.WBP_IdolQuizRoom_C")
    if cls is None:
        raise RuntimeError("WBP_IdolQuizRoom class is missing")
    cdo = unreal.get_default_object(cls)
    for name in (
        "create_button_normal_image", "create_button_hovered_image", "create_button_pressed_image",
        "refresh_button_normal_image", "refresh_button_hovered_image", "refresh_button_pressed_image",
    ):
        unreal.log("ROOM APPEARANCE {}={}".format(name, path(cdo.get_editor_property(name))))


main()
