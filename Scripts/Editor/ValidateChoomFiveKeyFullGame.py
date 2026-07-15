import unreal


MAP_PATH = "/Game/Maps/FiveKeyMap"
SONG_ASSET_PATH = "/Game/Data/DA_Choom_5Key_Full"
GAME_MODE_PATH = "/Game/Blueprints/Core/BP_RhythmGameMode"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def validate():
    require(unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH), "FiveKeyMap is missing")
    require(unreal.EditorAssetLibrary.does_asset_exist(SONG_ASSET_PATH), "5-key SongData is missing")

    song = unreal.EditorAssetLibrary.load_asset(SONG_ASSET_PATH)
    notes = list(song.get_editor_property("notes"))
    require(song.get_editor_property("key_mode") == unreal.RhythmChartKeyMode.FIVE_KEY, "SongData is not 5-key")
    require(len(notes) == 456, "Expected 456 notes, found {}".format(len(notes)))

    times = [note.get_editor_property("target_time_seconds") for note in notes]
    lanes = [note.get_editor_property("lane_index") for note in notes]
    require(times == sorted(times), "Note times are not sorted")
    require(len(set(round(value, 4) for value in times)) == len(times), "Simultaneous/duplicate note times found")
    require(min(lanes) >= 0 and max(lanes) <= 4, "5-key chart contains an out-of-range lane")

    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    conductors = [actor for actor in actors if isinstance(actor, unreal.RhythmConductor)]
    require(len(conductors) == 1, "FiveKeyMap must contain exactly one conductor")
    require(conductors[0].get_editor_property("song_data") == song, "FiveKeyMap conductor does not use the 5-key SongData")

    game_mode_blueprint = unreal.EditorAssetLibrary.load_asset(GAME_MODE_PATH)
    game_mode_cdo = unreal.get_default_object(game_mode_blueprint.generated_class())
    require(not game_mode_cdo.get_editor_property("enable_tap_chart_recording"), "Tap chart recording is still enabled")

    lane_counts = [lanes.count(index) for index in range(5)]
    unreal.log(
        "5-KEY VALIDATION PASSED: {} notes, {:.4f}s to {:.4f}s, lanes {}, map conductor override correct"
        .format(len(notes), times[0], times[-1], lane_counts)
    )


validate()
