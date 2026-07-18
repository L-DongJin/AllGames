import unreal


DATA_DIR = "/Game/Common/Data"
CATALOG_PATH = DATA_DIR + "/DA_MiniGameCatalog"
MAIN_HUB_MAP = "/Game/Maps/MainHubMap"


def get_or_create(name, asset_class):
    path = DATA_DIR + "/" + name
    asset = unreal.load_asset(path)
    if asset is not None:
        return asset
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", asset_class)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, DATA_DIR, asset_class, factory)


def main():
    rhythm = get_or_create("DA_Game_Rhythm", unreal.MiniGameDefinitionDataAsset)
    rhythm.set_editor_property("game_id", "RHYTHM")
    rhythm.set_editor_property("display_name", "리듬게임")
    rhythm.set_editor_property("description", "5키 리듬게임에서 곡과 난이도를 선택해 플레이합니다.")
    rhythm.set_editor_property("entry_map", unreal.load_asset("/Game/Maps/LobbyMap"))
    rhythm.set_editor_property("enabled", True)
    unreal.EditorAssetLibrary.save_loaded_asset(rhythm, only_if_is_dirty=False)

    idol = get_or_create("DA_Game_IdolQuiz", unreal.MiniGameDefinitionDataAsset)
    idol.set_editor_property("game_id", "IDOL_QUIZ")
    idol.set_editor_property("display_name", "아이돌 얼굴 맞히기")
    idol.set_editor_property("description", "친구들과 채팅으로 가장 먼저 아이돌 이름을 맞히는 게임입니다.")
    idol.set_editor_property("enabled", False)
    unreal.EditorAssetLibrary.save_loaded_asset(idol, only_if_is_dirty=False)

    catalog = get_or_create("DA_MiniGameCatalog", unreal.MiniGameCatalogDataAsset)
    catalog.set_editor_property("games", [rhythm, idol])
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not unreal.EditorAssetLibrary.does_asset_exist(MAIN_HUB_MAP):
        if not level_subsystem.new_level(MAIN_HUB_MAP):
            raise RuntimeError("Failed to create MainHubMap")
    else:
        level_subsystem.load_level(MAIN_HUB_MAP)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    game_mode = unreal.load_class(None, "/Script/AllGames.MainHubGameModeBase")
    if game_mode is None:
        raise RuntimeError("MainHubGameModeBase is unavailable")
    world.get_world_settings().set_editor_property("default_game_mode", game_mode)
    if not level_subsystem.save_current_level():
        raise RuntimeError("Failed to save MainHubMap")
    unreal.log("MAIN HUB READY: Rhythm enabled, Idol Quiz coming soon")


main()
