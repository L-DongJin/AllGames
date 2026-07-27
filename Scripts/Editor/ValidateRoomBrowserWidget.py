import unreal


def main():
    cls = unreal.load_class(None, "/Game/UI/WBP_RoomBrowser.WBP_RoomBrowser_C")
    if cls is None:
        raise RuntimeError("WBP_RoomBrowser could not be loaded")
    if not isinstance(unreal.get_default_object(cls), unreal.IdolQuizRoomWidget):
        raise RuntimeError("WBP_RoomBrowser must inherit IdolQuizRoomWidget")
    unreal.log("WBP_ROOM_BROWSER CLASS READY")


main()
