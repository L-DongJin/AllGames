import unreal


ASSET_PATH = "/Game/UI/WBP_RhythmLobby"


def main():
    parent_class = unreal.load_class(None, "/Script/AllGames.RhythmLobbyWidget")
    if parent_class is None:
        raise RuntimeError("URhythmLobbyWidget class is unavailable")

    asset = unreal.load_asset(ASSET_PATH)
    if asset is None:
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "WBP_RhythmLobby",
            "/Game/UI",
            unreal.WidgetBlueprint,
            factory,
        )
    if asset is None:
        raise RuntimeError("Failed to create WBP_RhythmLobby")

    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    unreal.log("WBP_RhythmLobby READY: assign Lobby Background Image under Rhythm|Appearance")


main()
