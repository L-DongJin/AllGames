import unreal


LOBBY_MAP = "/Game/Maps/LobbyMap"


def main():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not unreal.EditorAssetLibrary.does_asset_exist(LOBBY_MAP):
        if not level_subsystem.new_level(LOBBY_MAP):
            raise RuntimeError("Failed to create LobbyMap")
    else:
        level_subsystem.load_level(LOBBY_MAP)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

    lobby_game_mode = unreal.load_class(None, "/Script/AllGames.RhythmLobbyGameModeBase")
    if lobby_game_mode is None:
        raise RuntimeError("RhythmLobbyGameModeBase is unavailable; build the Editor target first")
    world.get_world_settings().set_editor_property("default_game_mode", lobby_game_mode)

    if not level_subsystem.save_current_level():
        raise RuntimeError("Failed to save LobbyMap")
    unreal.log("LobbyMap created with RhythmLobbyGameModeBase and no gameplay audio actors")


main()
