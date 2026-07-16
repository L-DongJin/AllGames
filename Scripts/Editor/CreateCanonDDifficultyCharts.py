import csv
import json
import os

import unreal


CATALOG_PATH = "/Game/Data/DA_RhythmSongCatalog"
MUSIC_PATH = "/Game/Audio/Music/CANON-D"
RECORDING_DIR = "CanonDDifficulties"
DIFFICULTIES = [
    ("Easy", unreal.RhythmDifficulty.EASY),
    ("Normal", unreal.RhythmDifficulty.NORMAL),
    ("Hard", unreal.RhythmDifficulty.HARD),
    ("Expert", unreal.RhythmDifficulty.EXPERT),
]


def main():
    catalog = unreal.load_asset(CATALOG_PATH)
    music = unreal.load_asset(MUSIC_PATH)
    if catalog is None or music is None:
        raise RuntimeError("CANON-D catalog or master music asset is missing")

    base_dir = os.path.join(unreal.Paths.project_saved_dir(), "ChartRecordings", RECORDING_DIR)
    with open(os.path.join(base_dir, "CanonD_metadata.json"), "r", encoding="utf-8") as stream:
        metadata = json.load(stream)

    new_charts = []
    for difficulty_name, difficulty in DIFFICULTIES:
        csv_path = os.path.join(base_dir, "CanonD_{}_5Key.csv".format(difficulty_name))
        with open(csv_path, "r", encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream))

        asset_name = "DA_CanonD_{}_5Key".format(difficulty_name)
        asset_path = "/Game/Data/{}".format(asset_name)
        asset = unreal.load_asset(asset_path)
        if asset is None:
            factory = unreal.DataAssetFactory()
            factory.set_editor_property("data_asset_class", unreal.RhythmSongDataAsset)
            asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                asset_name, "/Game/Data", unreal.RhythmSongDataAsset, factory
            )

        notes = []
        for row in rows:
            note = unreal.RhythmNoteData()
            note.set_editor_property("target_time_seconds", float(row["target_time_seconds"]))
            note.set_editor_property("duration_seconds", float(row.get("duration_seconds", 0.0) or 0.0))
            note.set_editor_property("lane_index", int(row["lane_index"]))
            notes.append(note)

        asset.set_editor_property("song_title", "CANON-D")
        asset.set_editor_property("music", music)
        asset.set_editor_property("preview_start_time_seconds", 55.0)
        asset.set_editor_property("preview_duration_seconds", 15.0)
        asset.set_editor_property("preview_volume", 0.65)
        asset.set_editor_property("bpm", float(metadata["bpm"]))
        asset.set_editor_property("music_offset_seconds", float(metadata["music_offset_seconds"]))
        asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.FIVE_KEY)
        asset.set_editor_property("difficulty", difficulty)
        asset.set_editor_property("note_travel_time_seconds", 2.0)
        asset.set_editor_property("notes", notes)
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        new_charts.append(asset)

    existing = [
        chart for chart in catalog.get_editor_property("songs")
        if chart and not chart.get_name().startswith("DA_CanonD_")
    ]
    catalog.set_editor_property("songs", existing + new_charts)
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)
    unreal.log("CANON-D CHARTS READY: {}".format(
        [len(chart.get_editor_property("notes")) for chart in new_charts]
    ))


main()
