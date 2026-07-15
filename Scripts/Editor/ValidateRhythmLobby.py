import unreal


LOBBY_MAP = "/Game/Maps/LobbyMap"
SONG_CATALOG = "/Game/Data/DA_RhythmSongCatalog"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    require(unreal.EditorAssetLibrary.does_asset_exist(LOBBY_MAP), "LobbyMap is missing")
    unreal.EditorLoadingAndSavingUtils.load_map(LOBBY_MAP)
    catalog = unreal.load_asset(SONG_CATALOG)
    require(catalog is not None, "Rhythm song catalog is missing")
    songs = catalog.get_editor_property("songs")
    expected = [
        ("DA_Choom_Easy_5Key", "Choom", unreal.RhythmDifficulty.EASY, 322, [76, 75, 21, 75, 75]),
        ("DA_Choom_Normal_5Key", "Choom", unreal.RhythmDifficulty.NORMAL, 488, [113, 112, 39, 112, 112]),
        ("DA_Choom_Hard_5Key", "Choom", unreal.RhythmDifficulty.HARD, 576, [133, 133, 46, 133, 131]),
        ("DA_Choom_Expert_5Key", "Choom", unreal.RhythmDifficulty.EXPERT, 715, [166, 165, 54, 166, 164]),
        ("DA_Lemonade_Easy_5Key", "Lemonade", unreal.RhythmDifficulty.EASY, 337, [82, 80, 14, 81, 80]),
        ("DA_Lemonade_Normal_5Key", "Lemonade", unreal.RhythmDifficulty.NORMAL, 520, [124, 124, 25, 124, 123]),
        ("DA_Lemonade_Hard_5Key", "Lemonade", unreal.RhythmDifficulty.HARD, 605, [145, 144, 28, 144, 144]),
        ("DA_Lemonade_Expert_5Key", "Lemonade", unreal.RhythmDifficulty.EXPERT, 881, [213, 213, 32, 212, 211]),
        ("DA_ItsMe_Easy_5Key", "It'sMe", unreal.RhythmDifficulty.EASY, 208, [51, 49, 9, 50, 49]),
        ("DA_ItsMe_Normal_5Key", "It'sMe", unreal.RhythmDifficulty.NORMAL, 359, [85, 85, 20, 85, 84]),
        ("DA_ItsMe_Hard_5Key", "It'sMe", unreal.RhythmDifficulty.HARD, 426, [102, 101, 22, 101, 100]),
        ("DA_ItsMe_Expert_5Key", "It'sMe", unreal.RhythmDifficulty.EXPERT, 566, [137, 135, 24, 135, 135]),
    ]
    require(len(songs) == len(expected), "Song catalog must contain three songs with four difficulties each")
    summaries = []
    for song, (asset_name, title, difficulty, note_count, expected_lanes) in zip(songs, expected):
        require(song.get_name() == asset_name, "Unexpected catalog asset: {}".format(song.get_name()))
        require(str(song.get_editor_property("song_title")) == title, "Song title is incorrect for {}".format(asset_name))
        require(song.get_editor_property("difficulty") == difficulty, "Difficulty metadata is incorrect for {}".format(asset_name))
        notes = song.get_editor_property("notes")
        times = [note.get_editor_property("target_time_seconds") for note in notes]
        lanes = [note.get_editor_property("lane_index") for note in notes]
        lane_counts = [lanes.count(lane) for lane in range(5)]
        require(len(notes) == note_count, "Note count is incorrect for {}".format(asset_name))
        require(times == sorted(times) and len(times) == len(set(times)), "Chart times are not ordered and unique for {}".format(asset_name))
        require(all(0 <= lane < 5 for lane in lanes), "Invalid lane in {}".format(asset_name))
        require(lane_counts == expected_lanes, "Lane counts are incorrect for {}".format(asset_name))
        if title == "Choom":
            # These listening-reference windows are tests only. The generator must discover their
            # short rhythmic motifs from MIDI rather than containing song-specific timestamps.
            hook_windows = [(47.0, 49.0), (52.3, 54.5), (73.0, 75.3), (89.8, 91.0), (116.5, 118.8), (122.5, 124.8)]
            minimum_hook_notes = [2, 3, 4, 4][len(summaries)]
            require(
                all(sum(start <= time <= end for time in times) >= minimum_hook_notes for start, end in hook_windows),
                "A listening-reference hook was lost in {}".format(asset_name),
            )
        summaries.append("{}:{}".format(difficulty, note_count))
    expert_times = [note.get_editor_property("target_time_seconds") for note in songs[3].get_editor_property("notes")]
    require(sum(time >= 140.0 for time in expert_times) >= 145, "Expert chart is still too sparse after 140 seconds")
    for song_index, minimum_last_time in ((1, 185.0), (2, 135.0)):
        group = songs[song_index * 4:(song_index + 1) * 4]
        counts = [len(chart.get_editor_property("notes")) for chart in group]
        require(counts == sorted(counts) and len(set(counts)) == 4, "Difficulty density does not increase for song {}".format(song_index))
        require(all(chart.get_editor_property("notes")[-1].get_editor_property("target_time_seconds") >= minimum_last_time for chart in group), "A new song chart ends too early")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    require(world.get_name() == "LobbyMap", "LobbyMap is not the active startup map")
    game_mode = world.get_world_settings().get_editor_property("default_game_mode")
    require(game_mode is not None and game_mode.get_name() == "RhythmLobbyGameModeBase", "Lobby GameMode override is incorrect")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor_names = [actor.get_class().get_name() for actor in actor_subsystem.get_all_level_actors()]
    require(not any("RhythmConductor" in name for name in actor_names), "LobbyMap contains a RhythmConductor")
    require(not any("RhythmNoteSpawner" in name for name in actor_names), "LobbyMap contains a RhythmNoteSpawner")
    unreal.log("LOBBY VALIDATION PASSED: 3 songs / 12 charts {}, Choom Expert 140s+ {} notes".format(
        summaries, sum(time >= 140.0 for time in expert_times)
    ))


main()
