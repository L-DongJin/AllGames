import unreal


MAPS = [
    ("/Game/Maps/IdolQuizRoomMap", "/Script/AllGames.IdolQuizRoomGameModeBase"),
    ("/Game/Maps/IdolQuizLobbyMap", "/Script/AllGames.IdolQuizLobbyGameModeBase"),
]


def main():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    for map_path, game_mode_path in MAPS:
        if unreal.EditorAssetLibrary.does_asset_exist(map_path):
            levels.load_level(map_path)
        elif not levels.new_level(map_path):
            raise RuntimeError("Failed to create {}".format(map_path))
        world = editor.get_editor_world()
        game_mode = unreal.load_class(None, game_mode_path)
        if game_mode is None:
            raise RuntimeError("Failed to load {}".format(game_mode_path))
        world.get_world_settings().set_editor_property("default_game_mode", game_mode)
        levels.save_current_level()

    definition = unreal.load_asset("/Game/Common/Data/DA_Game_IdolQuiz")
    room_map = unreal.load_asset("/Game/Maps/IdolQuizRoomMap")
    if definition is None or room_map is None:
        raise RuntimeError("Idol Quiz definition or room map is missing")
    definition.set_editor_property("entry_map", room_map)
    definition.set_editor_property("enabled", True)
    unreal.EditorAssetLibrary.save_loaded_asset(definition, only_if_is_dirty=False)
    unreal.log("IDOL QUIZ MULTIPLAYER MAPS READY: browser and lobby maps, MainHub entry updated")


main()
