import csv
import os

import unreal


ASSET_PATH = "/Game/Data/DA_Choom_MultiTrack_Refined_Normal_5Key"
MUSIC_PATH = "/Game/Audio/Music/Choom"
CATALOG_PATH = "/Game/Data/DA_RhythmSongCatalog"
PRESERVED_CHARTS = [
    "/Game/Data/DA_Choom_5Key_Full",
    "/Game/Data/DA_Choom_MIDI_Normal_5Key",
    "/Game/Data/DA_Choom_MultiTrack_Normal_5Key",
]
CSV_PATH = os.path.join(unreal.Paths.project_saved_dir(), "ChartRecordings", "ChoomMultiTrackRefinedNormal5Key.csv")


def main():
    with open(CSV_PATH, "r", encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise RuntimeError("Refined multitrack CSV contains no notes")

    asset = unreal.load_asset(ASSET_PATH)
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.RhythmSongDataAsset)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_Choom_MultiTrack_Refined_Normal_5Key", "/Game/Data", unreal.RhythmSongDataAsset, factory
        )
    if asset is None:
        raise RuntimeError("Failed to create refined multitrack Song Data Asset")

    music = unreal.load_asset(MUSIC_PATH)
    notes = []
    for row in rows:
        note = unreal.RhythmNoteData()
        note.set_editor_property("lane_index", int(row["lane_index"]))
        note.set_editor_property("target_time_seconds", float(row["target_time_seconds"]))
        notes.append(note)

    asset.set_editor_property("song_title", "Choom - Multitrack Refined")
    asset.set_editor_property("music", music)
    asset.set_editor_property("bpm", 120.0)
    asset.set_editor_property("music_offset_seconds", -0.2185)
    asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.FIVE_KEY)
    asset.set_editor_property("difficulty", unreal.RhythmDifficulty.NORMAL)
    asset.set_editor_property("note_travel_time_seconds", 2.0)
    asset.set_editor_property("notes", notes)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

    catalog = unreal.load_asset(CATALOG_PATH)
    preserved = [unreal.load_asset(path) for path in PRESERVED_CHARTS]
    if catalog is None or music is None or any(chart is None for chart in preserved):
        raise RuntimeError("Required catalog, music, or preserved chart is missing")
    catalog.set_editor_property("songs", preserved + [asset])
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)

    lane_counts = [sum(int(row["lane_index"]) == lane for row in rows) for lane in range(5)]
    shifted = sum(float(row["audio_refinement_seconds"]) < 0 for row in rows)
    unreal.log("CHOOM MULTITRACK REFINED READY: {} notes, {} locally advanced, lanes {}, catalog entries 4".format(
        len(rows), shifted, lane_counts
    ))


main()
