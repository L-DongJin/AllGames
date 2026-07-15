import random

import unreal


ASSET_PATH = "/Game/Data/DA_Choom_Test"
TEST_CHART_START_SECONDS = 5.0
TEST_CHART_END_SECONDS = 174.0
TEST_BPM = 120.0
RANDOM_SEED = 20260716


def create_or_update_song_data():
    asset = None
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.RhythmSongDataAsset)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_Choom_Test",
            "/Game/Data",
            unreal.RhythmSongDataAsset,
            factory,
        )

    if asset is None:
        raise RuntimeError("Failed to create DA_Choom_Test")

    music = unreal.EditorAssetLibrary.load_asset("/Game/Audio/Music/Choom")
    if music is None:
        raise RuntimeError("Could not load /Game/Audio/Music/Choom")

    notes = []
    random_stream = random.Random(RANDOM_SEED)
    seconds_per_beat = 60.0 / TEST_BPM
    previous_lane = -1
    beat_index = 0
    target_time = TEST_CHART_START_SECONDS
    while target_time <= TEST_CHART_END_SECONDS:
        lane_index = random_stream.randrange(9)
        while lane_index == previous_lane:
            lane_index = random_stream.randrange(9)

        note = unreal.RhythmNoteData()
        note.set_editor_property("lane_index", lane_index)
        note.set_editor_property("target_time_seconds", target_time)
        notes.append(note)

        previous_lane = lane_index
        beat_index += 1
        target_time = TEST_CHART_START_SECONDS + beat_index * seconds_per_beat

    asset.set_editor_property("song_title", "Choom")
    asset.set_editor_property("music", music)
    asset.set_editor_property("bpm", TEST_BPM)
    asset.set_editor_property("music_offset_seconds", 0.0)
    asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.NINE_KEY)
    asset.set_editor_property("difficulty", unreal.RhythmDifficulty.NORMAL)
    asset.set_editor_property("note_travel_time_seconds", 2.0)
    asset.set_editor_property("notes", notes)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

    conductor_blueprint = unreal.EditorAssetLibrary.load_asset(
        "/Game/Blueprints/Rhythm/BP_RhythmConductor"
    )
    if conductor_blueprint is None:
        raise RuntimeError("Could not load BP_RhythmConductor")

    conductor_cdo = unreal.get_default_object(conductor_blueprint.generated_class())
    conductor_cdo.set_editor_property("song_data", asset)
    unreal.EditorAssetLibrary.save_loaded_asset(
        conductor_blueprint, only_if_is_dirty=False
    )
    unreal.log(
        "Created DA_Choom_Test with {} deterministic test notes from {:.1f}s to {:.1f}s"
        .format(len(notes), TEST_CHART_START_SECONDS, TEST_CHART_END_SECONDS)
    )
    unreal.log("Assigned DA_Choom_Test to BP_RhythmConductor")


create_or_update_song_data()
