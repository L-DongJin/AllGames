import unreal


def main():
    rhythm = unreal.load_asset("/Game/Common/Data/DA_Game_Rhythm")
    quiz = unreal.load_asset("/Game/Common/Data/DA_Game_IdolQuiz")
    catalog = unreal.load_asset("/Game/Common/Data/DA_MiniGameCatalog")
    room_map = unreal.load_asset("/Game/Maps/IdolQuizRoomMap")
    if not rhythm or not quiz or not catalog or not room_map:
        raise RuntimeError("Required MainHub catalog assets are missing")

    quiz.set_editor_property("game_id", "QUIZ")
    quiz.set_editor_property("display_name", "퀴즈게임")
    quiz.set_editor_property(
        "description",
        "방을 만들고 인물 퀴즈 또는 실시간 그림 퀴즈를 친구들과 플레이합니다.",
    )
    quiz.set_editor_property("entry_map", room_map)
    quiz.set_editor_property("enabled", True)
    unreal.EditorAssetLibrary.save_loaded_asset(quiz, only_if_is_dirty=False)

    # DrawingQuiz remains as a reusable definition asset, but the MainHub exposes
    # one combined Quiz card because game type is selected while creating a room.
    catalog.set_editor_property("games", [rhythm, quiz])
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)
    unreal.log("MAIN HUB TWO-CARD CATALOG READY")


main()
