import unreal


ASSET_PATH = "/Game/Data/DA_Choom_BeatSyncTest"
MUSIC_PATH = "/Game/Audio/Music/Choom"
CONDUCTOR_PATH = "/Game/Blueprints/Rhythm/BP_RhythmConductor"
BPM = 120.0
# Listening calibration: the first PIE test produced stable late-chart input errors near
# -147.5 ms, meaning audible attacks occurred that much earlier than the visual target.
BEAT_GRID_OFFSET_SECONDS = -0.1342
TEST_START_SECONDS = 10.0
TEST_END_SECONDS = 25.0
SPACE_LANE_INDEX = 4


def create_or_update_asset():
    asset = None
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.RhythmSongDataAsset)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_Choom_BeatSyncTest",
            "/Game/Data",
            unreal.RhythmSongDataAsset,
            factory,
        )
    if asset is None:
        raise RuntimeError("Failed to create DA_Choom_BeatSyncTest")

    music = unreal.EditorAssetLibrary.load_asset(MUSIC_PATH)
    if music is None:
        raise RuntimeError("Could not load {}".format(MUSIC_PATH))

    seconds_per_beat = 60.0 / BPM
    first_index = round((TEST_START_SECONDS - BEAT_GRID_OFFSET_SECONDS) / seconds_per_beat)
    notes = []
    beat_index = first_index
    while True:
        target_time = BEAT_GRID_OFFSET_SECONDS + beat_index * seconds_per_beat
        if target_time > TEST_END_SECONDS:
            break
        if target_time >= TEST_START_SECONDS:
            note = unreal.RhythmNoteData()
            note.set_editor_property("lane_index", SPACE_LANE_INDEX)
            note.set_editor_property("target_time_seconds", target_time)
            notes.append(note)
        beat_index += 1

    asset.set_editor_property("song_title", "Choom - Beat Sync Test")
    asset.set_editor_property("music", music)
    asset.set_editor_property("bpm", BPM)
    asset.set_editor_property("music_offset_seconds", BEAT_GRID_OFFSET_SECONDS)
    asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.NINE_KEY)
    asset.set_editor_property("difficulty", unreal.RhythmDifficulty.EASY)
    asset.set_editor_property("note_travel_time_seconds", 2.0)
    asset.set_editor_property("notes", notes)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

    conductor_blueprint = unreal.EditorAssetLibrary.load_asset(CONDUCTOR_PATH)
    if conductor_blueprint is None:
        raise RuntimeError("Could not load {}".format(CONDUCTOR_PATH))
    conductor_cdo = unreal.get_default_object(conductor_blueprint.generated_class())
    conductor_cdo.set_editor_property("song_data", asset)
    unreal.EditorAssetLibrary.save_loaded_asset(conductor_blueprint, only_if_is_dirty=False)

    unreal.log(
        "Created DA_Choom_BeatSyncTest: {} Space-lane beats, {:.4f}s to {:.4f}s, BPM {:.1f}, offset {:.4f}s"
        .format(len(notes), notes[0].target_time_seconds, notes[-1].target_time_seconds, BPM, BEAT_GRID_OFFSET_SECONDS)
    )
    unreal.log("Assigned DA_Choom_BeatSyncTest to BP_RhythmConductor")


create_or_update_asset()
