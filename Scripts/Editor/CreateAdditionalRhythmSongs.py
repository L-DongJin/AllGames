import csv
import json
import os

import unreal


CATALOG_PATH = "/Game/Data/DA_RhythmSongCatalog"
DIFFICULTIES = [
    ("Easy", unreal.RhythmDifficulty.EASY),
    ("Normal", unreal.RhythmDifficulty.NORMAL),
    ("Hard", unreal.RhythmDifficulty.HARD),
    ("Expert", unreal.RhythmDifficulty.EXPERT),
]
SONGS = [
    {
        "source_name": "Lemonade",
        "asset_name": "Lemonade",
        "title": "Lemonade",
        "music": "/Game/Audio/Music/Lemonade",
        "recording_dir": "LemonadeDifficulties",
    },
    {
        "source_name": "ItsMe",
        "asset_name": "ItsMe",
        "title": "It'sMe",
        "music": "/Game/Audio/Music/It_sMe",
        "recording_dir": "ItsMeDifficulties",
    },
]


def load_rows(recording_dir, source_name, difficulty_name):
    path = os.path.join(
        unreal.Paths.project_saved_dir(), "ChartRecordings", recording_dir,
        "{}_{}_5Key.csv".format(source_name, difficulty_name),
    )
    with open(path, "r", encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream))


def load_metadata(config):
    path = os.path.join(
        unreal.Paths.project_saved_dir(), "ChartRecordings", config["recording_dir"],
        "{}_metadata.json".format(config["source_name"]),
    )
    with open(path, "r", encoding="utf-8") as stream:
        return json.load(stream)


def create_chart(config, difficulty_name, difficulty, music, metadata):
    rows = load_rows(config["recording_dir"], config["source_name"], difficulty_name)
    asset_name = "DA_{}_{}_5Key".format(config["asset_name"], difficulty_name)
    asset_path = "/Game/Data/{}".format(asset_name)
    asset = unreal.load_asset(asset_path) if unreal.EditorAssetLibrary.does_asset_exist(asset_path) else None
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
    asset.set_editor_property("song_title", config["title"])
    asset.set_editor_property("music", music)
    asset.set_editor_property("bpm", float(metadata["bpm"]))
    asset.set_editor_property("music_offset_seconds", float(metadata["music_offset_seconds"]))
    asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.FIVE_KEY)
    asset.set_editor_property("difficulty", difficulty)
    asset.set_editor_property("note_travel_time_seconds", 2.0)
    asset.set_editor_property("notes", notes)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    return asset


def main():
    catalog = unreal.load_asset(CATALOG_PATH)
    if catalog is None:
        raise RuntimeError("Rhythm song catalog is missing")

    charts = []
    summaries = []
    for config in SONGS:
        music = unreal.load_asset(config["music"])
        if music is None:
            raise RuntimeError("Music asset is missing: {}".format(config["music"]))
        metadata = load_metadata(config)
        counts = []
        for difficulty_name, difficulty in DIFFICULTIES:
            chart = create_chart(config, difficulty_name, difficulty, music, metadata)
            charts.append(chart)
            counts.append(len(chart.get_editor_property("notes")))
        summaries.append("{}={}".format(config["title"], counts))

    generated_prefixes = ("DA_Lemonade_", "DA_ItsMe_")
    existing = [
        chart for chart in catalog.get_editor_property("songs")
        if chart and not chart.get_name().startswith(generated_prefixes)
    ]
    catalog.set_editor_property("songs", existing + charts)
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)
    unreal.log("ADDITIONAL RHYTHM SONGS READY: {}".format(", ".join(summaries)))


main()
