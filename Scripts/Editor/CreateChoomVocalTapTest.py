import csv
import os

import unreal


ASSET_PATH = "/Game/Data/DA_Choom_VocalTapTest"
MUSIC_PATH = "/Game/Audio/Music/Choom"
CONDUCTOR_PATH = "/Game/Blueprints/Rhythm/BP_RhythmConductor"
GAME_MODE_PATH = "/Game/Blueprints/Core/BP_RhythmGameMode"
RECORDING_PATH = os.path.join(
    unreal.Paths.project_saved_dir(), "ChartRecordings", "ChoomTapRecording.csv"
)


def load_recorded_times():
    if not os.path.isfile(RECORDING_PATH):
        raise RuntimeError("Tap recording not found: {}".format(RECORDING_PATH))
    with open(RECORDING_PATH, "r", encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.DictReader(stream))
    times = [float(row["target_time_seconds"]) for row in rows]
    if not times:
        raise RuntimeError("Tap recording contains no notes")
    return times


def create_or_update_asset():
    recorded_times = load_recorded_times()
    asset = None
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.RhythmSongDataAsset)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_Choom_VocalTapTest",
            "/Game/Data",
            unreal.RhythmSongDataAsset,
            factory,
        )
    if asset is None:
        raise RuntimeError("Failed to create DA_Choom_VocalTapTest")

    music = unreal.EditorAssetLibrary.load_asset(MUSIC_PATH)
    if music is None:
        raise RuntimeError("Could not load {}".format(MUSIC_PATH))

    notes = []
    for target_time in recorded_times:
        note = unreal.RhythmNoteData()
        note.set_editor_property("lane_index", 4)  # Space only for timing verification.
        note.set_editor_property("target_time_seconds", target_time)
        notes.append(note)

    asset.set_editor_property("song_title", "Choom - Vocal Tap Test")
    asset.set_editor_property("music", music)
    asset.set_editor_property("bpm", 120.0)
    asset.set_editor_property("music_offset_seconds", -0.1342)
    asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.NINE_KEY)
    asset.set_editor_property("difficulty", unreal.RhythmDifficulty.EASY)
    asset.set_editor_property("note_travel_time_seconds", 2.0)
    asset.set_editor_property("notes", notes)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

    conductor_blueprint = unreal.EditorAssetLibrary.load_asset(CONDUCTOR_PATH)
    conductor_cdo = unreal.get_default_object(conductor_blueprint.generated_class())
    conductor_cdo.set_editor_property("song_data", asset)
    unreal.EditorAssetLibrary.save_loaded_asset(conductor_blueprint, only_if_is_dirty=False)

    game_mode_blueprint = unreal.EditorAssetLibrary.load_asset(GAME_MODE_PATH)
    game_mode_cdo = unreal.get_default_object(game_mode_blueprint.generated_class())
    game_mode_cdo.set_editor_property("enable_tap_chart_recording", False)
    unreal.EditorAssetLibrary.save_loaded_asset(game_mode_blueprint, only_if_is_dirty=False)

    unreal.log(
        "Created DA_Choom_VocalTapTest from {} human taps ({:.4f}s to {:.4f}s)"
        .format(len(notes), recorded_times[0], recorded_times[-1])
    )
    unreal.log("Assigned vocal tap test to BP_RhythmConductor and disabled tap recording")


create_or_update_asset()
