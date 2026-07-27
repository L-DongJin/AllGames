import unreal


def main():
    asset = unreal.load_asset("/Game/UI/WBP_GameSelect")
    generated = unreal.load_class(None, "/Game/UI/WBP_GameSelect.WBP_GameSelect_C")
    if asset is None or generated is None:
        raise RuntimeError("WBP_GameSelect could not be loaded")
    default_object = unreal.get_default_object(generated)
    if not isinstance(default_object, unreal.MainHubWidget):
        raise RuntimeError("WBP_GameSelect must inherit MainHubWidget")
    unreal.log("WBP_GAME_SELECT CLASS READY")


main()
