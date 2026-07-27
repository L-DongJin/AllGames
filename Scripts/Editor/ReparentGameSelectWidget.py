import unreal


def main():
    blueprint = unreal.load_asset("/Game/UI/WBP_GameSelect")
    parent = unreal.load_class(None, "/Script/AllGames.MainHubWidget")
    if blueprint is None or parent is None:
        raise RuntimeError("WBP_GameSelect or MainHubWidget class is missing")
    unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, parent)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    unreal.log("WBP_GAME_SELECT REPARENTED")


main()
