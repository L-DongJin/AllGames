import unreal


MAP_PATH = "/Game/Maps/DrawingQuizMap"
DATA_DIR = "/Game/Common/Data"
DEFINITION_PATH = DATA_DIR + "/DA_Game_DrawingQuiz"
CATALOG_PATH = DATA_DIR + "/DA_MiniGameCatalog"


def create_or_load_definition():
    definition = unreal.load_asset(DEFINITION_PATH)
    if definition is not None:
        return definition
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.MiniGameDefinitionDataAsset)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_Game_DrawingQuiz", DATA_DIR, unreal.MiniGameDefinitionDataAsset, factory
    )


def main():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        levels.load_level(MAP_PATH)
    elif not levels.new_level(MAP_PATH):
        raise RuntimeError("Failed to create DrawingQuizMap")

    game_mode = unreal.load_class(None, "/Script/AllGames.DrawingQuizGameMode")
    if game_mode is None:
        raise RuntimeError("DrawingQuizGameMode is unavailable")
    editor.get_editor_world().get_world_settings().set_editor_property("default_game_mode", game_mode)
    if not levels.save_current_level():
        raise RuntimeError("Failed to save DrawingQuizMap")

    definition = create_or_load_definition()
    definition.set_editor_property("game_id", "DRAWING_QUIZ")
    definition.set_editor_property("display_name", "실시간 그림 퀴즈")
    definition.set_editor_property("description", "친구가 그린 그림을 보고 채팅으로 가장 먼저 정답을 맞히는 게임입니다.")
    # Every network mini-game enters the shared EOS room browser first.
    definition.set_editor_property("entry_map", unreal.load_asset("/Game/Maps/IdolQuizRoomMap"))
    definition.set_editor_property("enabled", True)
    unreal.EditorAssetLibrary.save_loaded_asset(definition, only_if_is_dirty=False)

    catalog = unreal.load_asset(CATALOG_PATH)
    if catalog is None:
        raise RuntimeError("Mini-game catalog is missing")
    rhythm = unreal.load_asset(DATA_DIR + "/DA_Game_Rhythm")
    quiz = unreal.load_asset(DATA_DIR + "/DA_Game_IdolQuiz")
    if rhythm is None or quiz is None:
        raise RuntimeError("MainHub definitions are missing")
    catalog.set_editor_property("games", [rhythm, quiz])
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)
    unreal.log("DRAWING QUIZ PROTOTYPE READY")


main()
