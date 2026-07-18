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
        "source_name": "Sheesh", "asset_prefix": "Sheesh", "song_id": "SHEESH",
        "title": "SHEESH", "music": "/Game/Audio/Music/베이비몬스터-SHEESH",
        "recording_dir": "SheeshAudioStemDifficulties", "preview_start": 52.0,
    },
    {
        "source_name": "Drip", "asset_prefix": "Drip", "song_id": "DRIP",
        "title": "DRIP", "music": "/Game/Audio/Music/베이비몬스터-DRIP",
        "recording_dir": "DripAudioStemDifficulties", "preview_start": 48.0,
    },
    {
        "source_name": "IveBangBang", "asset_prefix": "IveBangBang", "song_id": "IVE_BANGBANG",
        "title": "BANG BANG", "music": "/Game/Audio/Music/아이브-BANGBANG",
        "recording_dir": "IveBangBangDifficulties", "preview_start": 48.0,
    },
    {
        "source_name": "KiiiKiii404", "asset_prefix": "KiiiKiii404", "song_id": "KIIIKIII_404",
        "title": "404 (New Era)", "music": "/Game/Audio/Music/KiiiKiii-404",
        "recording_dir": "KiiiKiii404Difficulties", "preview_start": 50.0,
    },
    {
        "source_name": "YenaCatch", "asset_prefix": "YenaCatch", "song_id": "YENA_CATCH_CATCH",
        "title": "캐치캐치", "music": "/Game/Audio/Music/YENA-캐치캐치",
        "recording_dir": "YenaCatchDifficulties", "preview_start": 47.0,
    },
]


def recording_path(config, filename):
    return os.path.join(unreal.Paths.project_saved_dir(), "ChartRecordings", config["recording_dir"], filename)


def chart_level(notes):
    times = [note.get_editor_property("target_time_seconds") for note in notes]
    durations = [note.get_editor_property("duration_seconds") for note in notes]
    if not times:
        return 1
    playable_duration = max(1.0, times[-1] - times[0])
    peak_two_second_nps = 0.0
    end_index = 0
    for start_index, start_time in enumerate(times):
        end_index = max(end_index, start_index)
        while end_index < len(times) and times[end_index] < start_time + 2.0:
            end_index += 1
        peak_two_second_nps = max(peak_two_second_nps, (end_index - start_index) / 2.0)
    hold_burden = sum(min(duration, 2.25) for duration in durations) / playable_duration
    raw = len(times) / playable_duration * 1.5 + peak_two_second_nps * 0.8 + hold_burden * 2.0
    return max(1, min(24, round(raw)))


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
    csv_path = recording_path(config, "{}_{}_5Key.csv".format(config["source_name"], difficulty_name))
    with open(csv_path, "r", encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.DictReader(stream))
    notes = []
    for row in rows:
        note = unreal.RhythmNoteData()
        note.set_editor_property("target_time_seconds", float(row["target_time_seconds"]))
        note.set_editor_property("duration_seconds", float(row.get("duration_seconds", 0.0) or 0.0))
        note.set_editor_property("lane_index", int(row["lane_index"]))
        notes.append(note)
    asset.set_editor_property("song_id", config["song_id"])
    asset.set_editor_property("chart_version", 1)
    asset.set_editor_property("song_title", config["title"])
    asset.set_editor_property("music", music)
    asset.set_editor_property("preview_start_time_seconds", config["preview_start"])
    asset.set_editor_property("preview_duration_seconds", 15.0)
    asset.set_editor_property("preview_volume", 0.65)
    asset.set_editor_property("bpm", float(metadata["bpm"]))
    asset.set_editor_property("music_offset_seconds", float(metadata["music_offset_seconds"]))
    asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.FIVE_KEY)
    asset.set_editor_property("difficulty", difficulty)
    asset.set_editor_property("chart_level", chart_level(notes))
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
        with open(recording_path(config, "{}_metadata.json".format(config["source_name"])), "r", encoding="utf-8") as stream:
            metadata = json.load(stream)
        for difficulty_name, difficulty in DIFFICULTIES:
            generated.append(create_chart(config, difficulty_name, difficulty, music, metadata))
    existing = [
        chart for chart in catalog.get_editor_property("songs")
        if chart and not chart.get_name().startswith(generated_prefixes)
    ]
    catalog.set_editor_property("songs", existing + generated)
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)
    unreal.log("FIVE NEW SONGS READY: {}".format(
        [(chart.get_name(), len(chart.get_editor_property("notes")), chart.get_editor_property("chart_level")) for chart in generated]
    ))


main()
