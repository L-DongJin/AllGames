import unreal


def main():
    room_map = unreal.load_asset("/Game/Maps/IdolQuizRoomMap")
    if room_map is None:
        raise RuntimeError("Shared room browser map is missing")

    for path in (
        "/Game/Common/Data/DA_Game_IdolQuiz",
        "/Game/Common/Data/DA_Game_DrawingQuiz",
    ):
        definition = unreal.load_asset(path)
        if definition is None:
            raise RuntimeError("Missing mini-game definition: " + path)
        definition.set_editor_property("entry_map", room_map)
        definition.set_editor_property("enabled", True)
        unreal.EditorAssetLibrary.save_loaded_asset(definition, only_if_is_dirty=False)

    unreal.log("SHARED MINI GAME LOBBY READY")


main()
