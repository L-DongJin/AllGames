import csv
import os

import unreal


ASSET_PATH = "/Game/Data/DA_Choom_5Key_Full"
MUSIC_PATH = "/Game/Audio/Music/Choom"
SOURCE_MAP = "/Game/Maps/TestMap"
FIVE_KEY_MAP = "/Game/Maps/FiveKeyMap"
GAME_MODE_PATH = "/Game/Blueprints/Core/BP_RhythmGameMode"
TIMING_PATH = os.path.join(
    unreal.Paths.project_saved_dir(), "ChartRecordings", "ChoomFiveKeyFullChart.csv"
)


def load_timing_rows():
    if not os.path.isfile(TIMING_PATH):
        raise RuntimeError("Generated timing CSV not found: {}".format(TIMING_PATH))
    with open(TIMING_PATH, "r", encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise RuntimeError("Generated timing CSV contains no notes")
    return [
        {
            "time": float(row["target_time_seconds"]),
            "source": row["source"],
            "strength": float(row["strength"]),
        }
        for row in rows
    ]


def assign_five_key_lanes(rows):
    # 5-key lanes are D/F/Space/J/K => indices 0/1/2/3/4.
    left_cycle = [1, 0, 1, 0]
    right_cycle = [3, 4, 3, 4]
    left_index = 0
    right_index = 0
    next_side = "left"
    lanes = []
    for index, row in enumerate(rows):
        gap = row["time"] - rows[index - 1]["time"] if index > 0 else 999.0
        strong_onset = row["source"] == "onset" and row["strength"] >= 1.2
        use_space = (
            index == 0
            or gap >= 0.75
            or (strong_onset and gap >= 0.24)
            or (index % 16 == 0 and gap >= 0.34)
        )
        if use_space and (not lanes or lanes[-1] != 2):
            lanes.append(2)
            continue

        if next_side == "left":
            lanes.append(left_cycle[left_index % len(left_cycle)])
            left_index += 1
            next_side = "right"
        else:
            lanes.append(right_cycle[right_index % len(right_cycle)])
            right_index += 1
            next_side = "left"
    return lanes


def create_song_asset(rows, lanes):
    asset = None
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.RhythmSongDataAsset)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_Choom_5Key_Full",
            "/Game/Data",
            unreal.RhythmSongDataAsset,
            factory,
        )
    if asset is None:
        raise RuntimeError("Failed to create DA_Choom_5Key_Full")

    music = unreal.EditorAssetLibrary.load_asset(MUSIC_PATH)
    if music is None:
        raise RuntimeError("Could not load {}".format(MUSIC_PATH))

    notes = []
    for row, lane_index in zip(rows, lanes):
        note = unreal.RhythmNoteData()
        note.set_editor_property("lane_index", lane_index)
        note.set_editor_property("target_time_seconds", row["time"])
        notes.append(note)

    asset.set_editor_property("song_title", "Choom - 5 Key Full")
    asset.set_editor_property("music", music)
    asset.set_editor_property("bpm", 120.0)
    asset.set_editor_property("music_offset_seconds", -0.1342)
    asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.FIVE_KEY)
    asset.set_editor_property("difficulty", unreal.RhythmDifficulty.NORMAL)
    asset.set_editor_property("note_travel_time_seconds", 2.0)
    asset.set_editor_property("notes", notes)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    return asset


def create_and_configure_map(song_asset):
    if not unreal.EditorAssetLibrary.does_asset_exist(FIVE_KEY_MAP):
        if not unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP, FIVE_KEY_MAP):
            raise RuntimeError("Failed to duplicate TestMap as FiveKeyMap")

    unreal.EditorLoadingAndSavingUtils.load_map(FIVE_KEY_MAP)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    conductors = [
        actor for actor in actor_subsystem.get_all_level_actors()
        if isinstance(actor, unreal.RhythmConductor)
    ]
    if len(conductors) != 1:
        raise RuntimeError("FiveKeyMap must contain exactly one RhythmConductor; found {}".format(len(conductors)))
    conductors[0].set_editor_property("song_data", song_asset)
    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        raise RuntimeError("Failed to save FiveKeyMap")


def disable_tap_recording():
    game_mode_blueprint = unreal.EditorAssetLibrary.load_asset(GAME_MODE_PATH)
    game_mode_cdo = unreal.get_default_object(game_mode_blueprint.generated_class())
    game_mode_cdo.set_editor_property("enable_tap_chart_recording", False)
    unreal.EditorAssetLibrary.save_loaded_asset(game_mode_blueprint, only_if_is_dirty=False)


def main():
    rows = load_timing_rows()
    lanes = assign_five_key_lanes(rows)
    asset = create_song_asset(rows, lanes)
    create_and_configure_map(asset)
    disable_tap_recording()
    lane_counts = [lanes.count(index) for index in range(5)]
    unreal.log(
        "Created full 5-key Choom game: {} notes, {:.4f}s to {:.4f}s, lane counts {}"
        .format(len(rows), rows[0]["time"], rows[-1]["time"], lane_counts)
    )
    unreal.log("FiveKeyMap uses its own DA_Choom_5Key_Full Conductor instance override")


main()
