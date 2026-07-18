import unreal


def main():
    for path in (
        "/Game/Maps/MainHubMap",
        "/Game/Common/Data/DA_Game_Rhythm",
        "/Game/Common/Data/DA_Game_IdolQuiz",
        "/Game/Common/Data/DA_MiniGameCatalog",
    ):
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            raise RuntimeError("Missing MainHub asset: {}".format(path))
    catalog = unreal.load_asset("/Game/Common/Data/DA_MiniGameCatalog")
    games = catalog.get_editor_property("games")
    if len(games) != 2:
        raise RuntimeError("Expected two initial mini-game definitions")
    if not games[0].get_editor_property("enabled") or games[0].get_editor_property("entry_map") is None:
        raise RuntimeError("Rhythm game must be enabled and linked to LobbyMap")
    if games[1].get_editor_property("enabled"):
        raise RuntimeError("Idol Quiz must remain disabled until its playable map exists")
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem.load_level("/Game/Maps/MainHubMap"):
        raise RuntimeError("Unable to load MainHubMap")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    game_mode = world.get_world_settings().get_editor_property("default_game_mode")
    if game_mode is None or game_mode.get_name() != "MainHubGameModeBase":
        raise RuntimeError("MainHubMap does not use MainHubGameModeBase")
    unreal.log("MAIN HUB VALIDATED: catalog-driven entries, Rhythm enabled, Idol Quiz disabled")


main()
