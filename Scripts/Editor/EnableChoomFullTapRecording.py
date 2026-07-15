import unreal


ASSET_PATH = "/Game/Data/DA_Choom_FullTapRecording"
MUSIC_PATH = "/Game/Audio/Music/Choom"
CONDUCTOR_PATH = "/Game/Blueprints/Rhythm/BP_RhythmConductor"
GAME_MODE_PATH = "/Game/Blueprints/Core/BP_RhythmGameMode"


def enable_recording():
    asset = None
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.RhythmSongDataAsset)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_Choom_FullTapRecording",
            "/Game/Data",
            unreal.RhythmSongDataAsset,
            factory,
        )
    if asset is None:
        raise RuntimeError("Failed to create DA_Choom_FullTapRecording")

    music = unreal.EditorAssetLibrary.load_asset(MUSIC_PATH)
    if music is None:
        raise RuntimeError("Could not load {}".format(MUSIC_PATH))

    asset.set_editor_property("song_title", "Choom - Full Tap Recording")
    asset.set_editor_property("music", music)
    asset.set_editor_property("bpm", 120.0)
    asset.set_editor_property("music_offset_seconds", -0.1342)
    asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.NINE_KEY)
    asset.set_editor_property("difficulty", unreal.RhythmDifficulty.EASY)
    asset.set_editor_property("note_travel_time_seconds", 2.0)
    asset.set_editor_property("notes", [])
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

    conductor_blueprint = unreal.EditorAssetLibrary.load_asset(CONDUCTOR_PATH)
    conductor_cdo = unreal.get_default_object(conductor_blueprint.generated_class())
    conductor_cdo.set_editor_property("song_data", asset)
    unreal.EditorAssetLibrary.save_loaded_asset(conductor_blueprint, only_if_is_dirty=False)

    game_mode_blueprint = unreal.EditorAssetLibrary.load_asset(GAME_MODE_PATH)
    game_mode_cdo = unreal.get_default_object(game_mode_blueprint.generated_class())
    game_mode_cdo.set_editor_property("enable_tap_chart_recording", True)
    unreal.EditorAssetLibrary.save_loaded_asset(game_mode_blueprint, only_if_is_dirty=False)

    unreal.log("Enabled clean Choom tap recording from 1.0 to 80.0 seconds")
    unreal.log("No gameplay notes are assigned, so the capture is not visually prompted")


enable_recording()
