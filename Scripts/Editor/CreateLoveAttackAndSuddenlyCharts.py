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
        "source_name": "LoveAttack",
        "asset_prefix": "LoveAttack",
        "title": "LoveAttack",
        "music": "/Game/Audio/Music/리센느-LoveAttack",
        "recording_dir": "LoveAttackAudioStemDifficulties",
        "preview_start": 48.0,
    },
    {
        "source_name": "Suddenly",
        "asset_prefix": "Suddenly",
        "title": "갑자기",
        "music": "/Game/Audio/Music/아이오아이-갑자기",
        "recording_dir": "SuddenlyDifficulties",
        "preview_start": 52.0,
    },
]


def load_metadata(config):
    path = os.path.join(
        unreal.Paths.project_saved_dir(), "ChartRecordings", config["recording_dir"],
        "{}_metadata.json".format(config["source_name"]),
    )
    with open(path, "r", encoding="utf-8") as stream:
        return json.load(stream)


def load_rows(config, difficulty_name):
    path = os.path.join(
        unreal.Paths.project_saved_dir(), "ChartRecordings", config["recording_dir"],
        "{}_{}_5Key.csv".format(config["source_name"], difficulty_name),
    )
    with open(path, "r", encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream))


def create_chart(config, difficulty_name, difficulty, music, metadata):
    asset_name = "DA_{}_{}_5Key".format(config["asset_prefix"], difficulty_name)
    asset_path = "/Game/Data/{}".format(asset_name)
    asset = unreal.load_asset(asset_path)
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.RhythmSongDataAsset)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, "/Game/Data", unreal.RhythmSongDataAsset, factory
        )

    notes = []
    for row in load_rows(config, difficulty_name):
        note = unreal.RhythmNoteData()
        note.set_editor_property("target_time_seconds", float(row["target_time_seconds"]))
        note.set_editor_property("duration_seconds", float(row.get("duration_seconds", 0.0) or 0.0))
        note.set_editor_property("lane_index", int(row["lane_index"]))
        notes.append(note)

    asset.set_editor_property("song_title", config["title"])
    asset.set_editor_property("music", music)
    asset.set_editor_property("preview_start_time_seconds", config["preview_start"])
    asset.set_editor_property("preview_duration_seconds", 15.0)
    asset.set_editor_property("preview_volume", 0.65)
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

    generated = []
    generated_prefixes = tuple("DA_{}_".format(config["asset_prefix"]) for config in SONGS)
    for config in SONGS:
        music = unreal.load_asset(config["music"])
        if music is None:
            raise RuntimeError("Music asset is missing: {}".format(config["music"]))
        metadata = load_metadata(config)
        for difficulty_name, difficulty in DIFFICULTIES:
            generated.append(create_chart(config, difficulty_name, difficulty, music, metadata))

    existing = [
        chart for chart in catalog.get_editor_property("songs")
        if chart and not chart.get_name().startswith(generated_prefixes)
    ]
    catalog.set_editor_property("songs", existing + generated)
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)
    unreal.log("LOVEATTACK / SUDDENLY CHARTS READY: {}".format(
        [(chart.get_name(), len(chart.get_editor_property("notes"))) for chart in generated]
    ))


main()
