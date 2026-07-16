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
        ("DA_Choom_Easy_5Key", "Choom", unreal.RhythmDifficulty.EASY, 488, [113, 112, 39, 112, 112], 1),
        ("DA_Choom_Normal_5Key", "Choom", unreal.RhythmDifficulty.NORMAL, 576, [133, 133, 46, 133, 131], 1),
        ("DA_Choom_Hard_5Key", "Choom", unreal.RhythmDifficulty.HARD, 715, [166, 165, 54, 166, 164], 4),
        ("DA_Choom_Expert_5Key", "Choom", unreal.RhythmDifficulty.EXPERT, 815, [190, 190, 57, 189, 189], 9),
        ("DA_Lemonade_Easy_5Key", "Lemonade", unreal.RhythmDifficulty.EASY, 520, [124, 124, 25, 124, 123], 1),
        ("DA_Lemonade_Normal_5Key", "Lemonade", unreal.RhythmDifficulty.NORMAL, 605, [145, 144, 28, 144, 144], 4),
        ("DA_Lemonade_Hard_5Key", "Lemonade", unreal.RhythmDifficulty.HARD, 881, [213, 213, 32, 212, 211], 8),
        ("DA_Lemonade_Expert_5Key", "Lemonade", unreal.RhythmDifficulty.EXPERT, 1104, [267, 266, 40, 266, 265], 13),
        ("DA_ItsMe_Easy_5Key", "It'sMe", unreal.RhythmDifficulty.EASY, 359, [85, 85, 20, 85, 84], 1),
        ("DA_ItsMe_Normal_5Key", "It'sMe", unreal.RhythmDifficulty.NORMAL, 426, [102, 101, 22, 101, 100], 1),
        ("DA_ItsMe_Hard_5Key", "It'sMe", unreal.RhythmDifficulty.HARD, 566, [137, 135, 24, 135, 135], 2),
        ("DA_ItsMe_Expert_5Key", "It'sMe", unreal.RhythmDifficulty.EXPERT, 669, [162, 162, 24, 161, 160], 5),
        ("DA_CHASEME_Easy_5Key", "CHASE-ME", unreal.RhythmDifficulty.EASY, 583, [118, 85, 164, 79, 137], 1),
        ("DA_CHASEME_Normal_5Key", "CHASE-ME", unreal.RhythmDifficulty.NORMAL, 791, [155, 123, 229, 108, 176], 2),
        ("DA_CHASEME_Hard_5Key", "CHASE-ME", unreal.RhythmDifficulty.HARD, 1146, [231, 194, 297, 161, 263], 0),
        ("DA_CHASEME_Expert_5Key", "CHASE-ME", unreal.RhythmDifficulty.EXPERT, 1513, [299, 251, 398, 225, 340], 1),
        ("DA_CanonD_Easy_5Key", "CANON-D", unreal.RhythmDifficulty.EASY, 403, [89, 87, 52, 88, 87], 2),
        ("DA_CanonD_Normal_5Key", "CANON-D", unreal.RhythmDifficulty.NORMAL, 495, [109, 109, 60, 109, 108], 3),
        ("DA_CanonD_Hard_5Key", "CANON-D", unreal.RhythmDifficulty.HARD, 927, [204, 204, 112, 204, 203], 7),
        ("DA_CanonD_Expert_5Key", "CANON-D", unreal.RhythmDifficulty.EXPERT, 1182, [266, 265, 120, 266, 265], 15),
        ("DA_Drama_Easy_5Key", "Drama", unreal.RhythmDifficulty.EASY, 543, [104, 82, 163, 75, 119], 2),
        ("DA_Drama_Normal_5Key", "Drama", unreal.RhythmDifficulty.NORMAL, 864, [170, 135, 252, 117, 190], 1),
        ("DA_Drama_Hard_5Key", "Drama", unreal.RhythmDifficulty.HARD, 1246, [223, 205, 365, 180, 273], 0),
        ("DA_Drama_Expert_5Key", "Drama", unreal.RhythmDifficulty.EXPERT, 1834, [343, 296, 519, 276, 400], 0),
        ("DA_Bansanka_Easy_5Key", "만찬가", unreal.RhythmDifficulty.EASY, 565, [127, 126, 59, 127, 126], 5),
        ("DA_Bansanka_Normal_5Key", "만찬가", unreal.RhythmDifficulty.NORMAL, 655, [151, 150, 55, 150, 149], 13),
        ("DA_Bansanka_Hard_5Key", "만찬가", unreal.RhythmDifficulty.HARD, 1040, [236, 236, 97, 236, 235], 21),
        ("DA_Bansanka_Expert_5Key", "만찬가", unreal.RhythmDifficulty.EXPERT, 1253, [284, 284, 119, 284, 282], 43),
        ("DA_LoveAttack_Easy_5Key", "LoveAttack", unreal.RhythmDifficulty.EASY, 594, [117, 96, 164, 75, 142], 0),
        ("DA_LoveAttack_Normal_5Key", "LoveAttack", unreal.RhythmDifficulty.NORMAL, 786, [157, 129, 218, 105, 177], 0),
        ("DA_LoveAttack_Hard_5Key", "LoveAttack", unreal.RhythmDifficulty.HARD, 1095, [212, 178, 308, 150, 247], 1),
        ("DA_LoveAttack_Expert_5Key", "LoveAttack", unreal.RhythmDifficulty.EXPERT, 1457, [276, 228, 418, 198, 337], 1),
        ("DA_Suddenly_Easy_5Key", "갑자기", unreal.RhythmDifficulty.EASY, 523, [120, 119, 46, 119, 119], 13),
        ("DA_Suddenly_Normal_5Key", "갑자기", unreal.RhythmDifficulty.NORMAL, 592, [136, 136, 51, 135, 134], 19),
        ("DA_Suddenly_Hard_5Key", "갑자기", unreal.RhythmDifficulty.HARD, 857, [196, 195, 76, 196, 194], 39),
        ("DA_Suddenly_Expert_5Key", "갑자기", unreal.RhythmDifficulty.EXPERT, 1052, [239, 239, 97, 239, 238], 76),
        ("DA_HeavySerenade_Easy_5Key", "HeavySerenade", unreal.RhythmDifficulty.EASY, 471, [89, 68, 152, 61, 101], 2),
        ("DA_HeavySerenade_Normal_5Key", "HeavySerenade", unreal.RhythmDifficulty.NORMAL, 752, [145, 115, 226, 99, 167], 1),
        ("DA_HeavySerenade_Hard_5Key", "HeavySerenade", unreal.RhythmDifficulty.HARD, 1063, [195, 188, 303, 149, 228], 0),
        ("DA_HeavySerenade_Expert_5Key", "HeavySerenade", unreal.RhythmDifficulty.EXPERT, 1539, [289, 243, 447, 220, 340], 0),
        ("DA_Rude_Easy_5Key", "RUDE!", unreal.RhythmDifficulty.EASY, 524, [103, 77, 151, 73, 120], 0),
        ("DA_Rude_Normal_5Key", "RUDE!", unreal.RhythmDifficulty.NORMAL, 858, [174, 140, 224, 115, 205], 2),
        ("DA_Rude_Hard_5Key", "RUDE!", unreal.RhythmDifficulty.HARD, 1177, [237, 190, 302, 168, 280], 1),
        ("DA_Rude_Expert_5Key", "RUDE!", unreal.RhythmDifficulty.EXPERT, 1731, [321, 281, 474, 250, 405], 1),
    ]
    require(len(songs) == len(expected), "Song catalog must contain eleven songs with four difficulties each")
    summaries = []
    for song, (asset_name, title, difficulty, note_count, expected_lanes, expected_long_notes) in zip(songs, expected):
        require(song.get_name() == asset_name, "Unexpected catalog asset: {}".format(song.get_name()))
        require(str(song.get_editor_property("song_title")) == title, "Song title is incorrect for {}".format(asset_name))
        require(song.get_editor_property("difficulty") == difficulty, "Difficulty metadata is incorrect for {}".format(asset_name))
        notes = song.get_editor_property("notes")
        times = [note.get_editor_property("target_time_seconds") for note in notes]
        lanes = [note.get_editor_property("lane_index") for note in notes]
        durations = [note.get_editor_property("duration_seconds") for note in notes]
        lane_counts = [lanes.count(lane) for lane in range(5)]
        require(len(notes) == note_count, "Note count is incorrect for {}".format(asset_name))
        require(times == sorted(times) and len(times) == len(set(times)), "Chart times are not ordered and unique for {}".format(asset_name))
        require(all(0 <= lane < 5 for lane in lanes), "Invalid lane in {}".format(asset_name))
        require(lane_counts == expected_lanes, "Lane counts are incorrect for {}".format(asset_name))
        require(sum(duration > 0.0 for duration in durations) == expected_long_notes, "Long-note count is incorrect for {}".format(asset_name))
        require(all(0.0 <= duration <= 3.0 for duration in durations), "Invalid long-note duration in {}".format(asset_name))
        for index, note in enumerate(notes):
            duration = durations[index]
            if duration <= 0.0:
                continue
            next_same_lane = next(
                (times[later] for later in range(index + 1, len(notes)) if lanes[later] == lanes[index]),
                None,
            )
            require(
                next_same_lane is None or times[index] + duration <= next_same_lane - 0.1,
                "Long note overlaps the next note on its lane in {}".format(asset_name),
            )
        if title == "Choom":
            # These listening-reference windows are tests only. The generator must discover their
            # short rhythmic motifs from MIDI rather than containing song-specific timestamps.
            hook_windows = [(47.0, 49.0), (52.3, 54.5), (73.0, 75.3), (89.8, 91.0), (116.5, 118.8), (122.5, 124.8)]
            minimum_hook_notes = [3, 4, 4, 4][len(summaries)]
            require(
                all(sum(start <= time <= end for time in times) >= minimum_hook_notes for start, end in hook_windows),
                "A listening-reference hook was lost in {}".format(asset_name),
            )
        summaries.append("{}:{}".format(difficulty, note_count))
    expert_times = [note.get_editor_property("target_time_seconds") for note in songs[3].get_editor_property("notes")]
    require(sum(time >= 140.0 for time in expert_times) >= 170, "Expert chart is still too sparse after 140 seconds")
    for song_index, minimum_last_time in (
        (1, 185.0), (2, 135.0), (3, 186.0), (4, 196.0), (5, 213.0), (6, 216.0),
        (7, 177.0), (8, 193.0), (9, 176.0), (10, 195.0)
    ):
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
    unreal.log("LOBBY VALIDATION PASSED: 11 songs / 44 charts {}, Choom Expert 140s+ {} notes".format(
        summaries, sum(time >= 140.0 for time in expert_times)
    ))


main()
