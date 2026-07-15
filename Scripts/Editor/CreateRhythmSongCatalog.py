import unreal


CATALOG_PATH = "/Game/Data/DA_RhythmSongCatalog"
INITIAL_SONG_PATH = "/Game/Data/DA_Choom_5Key_Full"


def main():
    song = unreal.load_asset(INITIAL_SONG_PATH)
    if song is None:
        raise RuntimeError("Initial Choom song asset is missing")

    catalog = unreal.load_asset(CATALOG_PATH)
    if catalog is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.RhythmSongCatalogDataAsset)
        catalog = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_RhythmSongCatalog", "/Game/Data", unreal.RhythmSongCatalogDataAsset, factory
        )
    if catalog is None:
        raise RuntimeError("Failed to create rhythm song catalog")

    catalog.set_editor_property("songs", [song])
    if not unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False):
        raise RuntimeError("Failed to save rhythm song catalog")
    unreal.log("RHYTHM SONG CATALOG READY: 1 song (Choom)")


main()
