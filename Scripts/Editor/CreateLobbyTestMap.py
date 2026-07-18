import unreal


SOURCE_MAP = "/Game/Maps/LobbyMap"
TEST_MAP = "/Game/Maps/LobbyTestMap"


def main():
    if not unreal.EditorAssetLibrary.does_asset_exist(TEST_MAP):
        if not unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP, TEST_MAP):
            raise RuntimeError("Failed to duplicate LobbyMap as LobbyTestMap")
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem.load_level(TEST_MAP):
        raise RuntimeError("Failed to load LobbyTestMap")
    if not level_subsystem.save_current_level():
        raise RuntimeError("Failed to save LobbyTestMap")
    unreal.log("LOBBY TEST MAP READY: login bypass is editor-only")


main()
