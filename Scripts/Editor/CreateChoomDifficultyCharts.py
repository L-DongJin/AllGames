import csv
import os

import unreal


MUSIC_PATH = "/Game/Audio/Music/Choom"
CATALOG_PATH = "/Game/Data/DA_RhythmSongCatalog"
OUTPUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "ChartRecordings", "ChoomDifficulties")
DIFFICULTIES = [
    ("Easy", unreal.RhythmDifficulty.EASY),
    ("Normal", unreal.RhythmDifficulty.NORMAL),
    ("Hard", unreal.RhythmDifficulty.HARD),
    ("Expert", unreal.RhythmDifficulty.EXPERT),
]


def create_chart(name, difficulty, music):
    csv_path = os.path.join(OUTPUT_DIR, "Choom_{}_5Key.csv".format(name))
    with open(csv_path, "r", encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.DictReader(stream))
    asset_name = "DA_Choom_{}_5Key".format(name)
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
        note.set_editor_property("lane_index", int(row["lane_index"]))
        note.set_editor_property("target_time_seconds", float(row["target_time_seconds"]))
        note.set_editor_property("duration_seconds", float(row.get("duration_seconds", 0.0) or 0.0))
        notes.append(note)
    asset.set_editor_property("song_title", "Choom")
    asset.set_editor_property("music", music)
    asset.set_editor_property("bpm", 120.0)
    asset.set_editor_property("music_offset_seconds", -0.2635)
    asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.FIVE_KEY)
    asset.set_editor_property("difficulty", difficulty)
    asset.set_editor_property("note_travel_time_seconds", 2.0)
    asset.set_editor_property("notes", notes)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    return asset, rows


def main():
    music = unreal.load_asset(MUSIC_PATH)
    catalog = unreal.load_asset(CATALOG_PATH)
    if music is None or catalog is None:
        raise RuntimeError("Choom music or song catalog is missing")
    charts = []
    summaries = []
    for name, difficulty in DIFFICULTIES:
        chart, rows = create_chart(name, difficulty, music)
        charts.append(chart)
        lanes = [sum(int(row["lane_index"]) == lane for row in rows) for lane in range(5)]
        summaries.append("{}={} {}".format(name, len(rows), lanes))
    # Only the four production difficulty charts appear in the lobby. Older comparison assets remain on disk.
    catalog.set_editor_property("songs", charts)
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)
    unreal.log("CHOOM DIFFICULTIES READY: {}".format(", ".join(summaries)))


main()
