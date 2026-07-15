import csv
import os

import unreal


ASSET_PATH = "/Game/Data/DA_Choom_HumanGroove_1_80"
MUSIC_PATH = "/Game/Audio/Music/Choom"
CONDUCTOR_PATH = "/Game/Blueprints/Rhythm/BP_RhythmConductor"
GAME_MODE_PATH = "/Game/Blueprints/Core/BP_RhythmGameMode"
RECORDING_PATH = os.path.join(
    unreal.Paths.project_saved_dir(), "ChartRecordings", "ChoomTapRecording_1_80.csv"
)

BPM = 120.0
GAMEPLAY_GRID_OFFSET_SECONDS = -0.1342
SIXTEENTH_SECONDS = 60.0 / BPM / 4.0
SNAP_WINDOW_SECONDS = 0.035
MINIMUM_NOTE_INTERVAL_SECONDS = 0.090


def load_taps():
    if not os.path.isfile(RECORDING_PATH):
        raise RuntimeError("Tap recording not found: {}".format(RECORDING_PATH))
    with open(RECORDING_PATH, "r", encoding="utf-8-sig", newline="") as stream:
        return [float(row["target_time_seconds"]) for row in csv.DictReader(stream)]


def clean_tap_times(raw_times):
    cleaned = []
    snapped_count = 0
    preserved_count = 0
    merged_count = 0
    for raw_time in raw_times:
        grid_index = round(
            (raw_time - GAMEPLAY_GRID_OFFSET_SECONDS) / SIXTEENTH_SECONDS
        )
        grid_time = GAMEPLAY_GRID_OFFSET_SECONDS + grid_index * SIXTEENTH_SECONDS
        if abs(raw_time - grid_time) <= SNAP_WINDOW_SECONDS:
            target_time = grid_time
            snapped_count += 1
        else:
            target_time = raw_time
            preserved_count += 1

        if cleaned and target_time - cleaned[-1] < MINIMUM_NOTE_INTERVAL_SECONDS:
            merged_count += 1
            continue
        cleaned.append(target_time)
    return cleaned, snapped_count, preserved_count, merged_count


def assign_hand_friendly_lanes(times):
    # Center-first cycles reduce large jumps while alternating hands on dense passages.
    left_cycle = [3, 2, 1, 0, 2, 3, 1, 0]
    right_cycle = [5, 6, 7, 8, 6, 5, 7, 8]
    left_index = 0
    right_index = 0
    next_side = "left"
    lanes = []
    for index, target_time in enumerate(times):
        gap = target_time - times[index - 1] if index > 0 else 999.0
        use_space_accent = gap >= 0.70 or (index % 16 == 0 and gap >= 0.34)
        if use_space_accent and (not lanes or lanes[-1] != 4):
            lanes.append(4)
            continue

        if next_side == "left":
            lane = left_cycle[left_index % len(left_cycle)]
            left_index += 1
            next_side = "right"
        else:
            lane = right_cycle[right_index % len(right_cycle)]
            right_index += 1
            next_side = "left"
        lanes.append(lane)
    return lanes


def create_or_update_chart():
    raw_times = load_taps()
    times, snapped_count, preserved_count, merged_count = clean_tap_times(raw_times)
    lanes = assign_hand_friendly_lanes(times)

    asset = None
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.RhythmSongDataAsset)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_Choom_HumanGroove_1_80",
            "/Game/Data",
            unreal.RhythmSongDataAsset,
            factory,
        )
    if asset is None:
        raise RuntimeError("Failed to create DA_Choom_HumanGroove_1_80")

    music = unreal.EditorAssetLibrary.load_asset(MUSIC_PATH)
    if music is None:
        raise RuntimeError("Could not load {}".format(MUSIC_PATH))

    notes = []
    for target_time, lane_index in zip(times, lanes):
        note = unreal.RhythmNoteData()
        note.set_editor_property("lane_index", lane_index)
        note.set_editor_property("target_time_seconds", target_time)
        notes.append(note)

    asset.set_editor_property("song_title", "Choom - Human Groove 1-80")
    asset.set_editor_property("music", music)
    asset.set_editor_property("bpm", BPM)
    asset.set_editor_property("music_offset_seconds", GAMEPLAY_GRID_OFFSET_SECONDS)
    asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.NINE_KEY)
    asset.set_editor_property("difficulty", unreal.RhythmDifficulty.NORMAL)
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
        "Created DA_Choom_HumanGroove_1_80: {} notes from {} taps; {} snapped, {} preserved, {} merged"
        .format(len(notes), len(raw_times), snapped_count, preserved_count, merged_count)
    )
    unreal.log("Assigned human groove chart and disabled tap recording")


create_or_update_chart()
