import unreal


EXPECTED = [
    ("DA_Drama_Easy_5Key", "Drama", "에스파-Drama", unreal.RhythmDifficulty.EASY, 543, [104, 82, 163, 75, 119], 2, 213.0),
    ("DA_Drama_Normal_5Key", "Drama", "에스파-Drama", unreal.RhythmDifficulty.NORMAL, 864, [170, 135, 252, 117, 190], 1, 213.0),
    ("DA_Drama_Hard_5Key", "Drama", "에스파-Drama", unreal.RhythmDifficulty.HARD, 1246, [223, 205, 365, 180, 273], 0, 213.0),
    ("DA_Drama_Expert_5Key", "Drama", "에스파-Drama", unreal.RhythmDifficulty.EXPERT, 1834, [343, 296, 519, 276, 400], 0, 213.0),
    ("DA_Bansanka_Easy_5Key", "만찬가", "tuki-만찬가", unreal.RhythmDifficulty.EASY, 565, [127, 126, 59, 127, 126], 5, 216.0),
    ("DA_Bansanka_Normal_5Key", "만찬가", "tuki-만찬가", unreal.RhythmDifficulty.NORMAL, 655, [151, 150, 55, 150, 149], 13, 216.0),
    ("DA_Bansanka_Hard_5Key", "만찬가", "tuki-만찬가", unreal.RhythmDifficulty.HARD, 1040, [236, 236, 97, 236, 235], 21, 216.0),
    ("DA_Bansanka_Expert_5Key", "만찬가", "tuki-만찬가", unreal.RhythmDifficulty.EXPERT, 1253, [284, 284, 119, 284, 282], 43, 216.0),
]


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    catalog = unreal.load_asset("/Game/Data/DA_RhythmSongCatalog")
    require(catalog is not None, "Song catalog is missing")
    catalog_names = [asset.get_name() for asset in catalog.get_editor_property("songs") if asset]

    for name, title, music_name, difficulty, count, lane_counts, long_count, minimum_last_time in EXPECTED:
        chart = unreal.load_asset("/Game/Data/{}".format(name))
        require(chart is not None, "{} is missing".format(name))
        require(str(chart.get_editor_property("song_title")) == title, "{} title is wrong".format(name))
        require(chart.get_editor_property("music").get_name() == music_name, "{} music is wrong".format(name))
        require(chart.get_editor_property("difficulty") == difficulty, "{} difficulty is wrong".format(name))
        require(chart.get_editor_property("key_mode") == unreal.RhythmChartKeyMode.FIVE_KEY, "{} is not 5-key".format(name))
        notes = chart.get_editor_property("notes")
        times = [note.get_editor_property("target_time_seconds") for note in notes]
        lanes = [note.get_editor_property("lane_index") for note in notes]
        durations = [note.get_editor_property("duration_seconds") for note in notes]
        require(len(notes) == count, "{} count is wrong".format(name))
        require(times == sorted(times) and len(times) == len(set(times)), "{} times are invalid".format(name))
        require(all(0 <= lane < 5 for lane in lanes), "{} contains an invalid lane".format(name))
        require([lanes.count(lane) for lane in range(5)] == lane_counts, "{} lane counts are wrong".format(name))
        require(sum(duration > 0.0 for duration in durations) == long_count, "{} hold count is wrong".format(name))
        require(all(0.0 <= duration <= 3.0 for duration in durations), "{} has an invalid hold duration".format(name))
        require(times[0] >= 2.25 and times[-1] >= minimum_last_time, "{} playable range is wrong".format(name))
        require(name in catalog_names, "{} is not in the lobby catalog".format(name))
        for index, duration in enumerate(durations):
            if duration <= 0.0:
                continue
            next_same_lane = next(
                (times[later] for later in range(index + 1, len(notes)) if lanes[later] == lanes[index]),
                None,
            )
            require(
                next_same_lane is None or times[index] + duration <= next_same_lane - 0.1,
                "{} contains an overlapping hold".format(name),
            )

    require(len(catalog_names) == 28, "Catalog must contain seven songs and twenty-eight charts")
    unreal.log("DRAMA / BANSANKA VALIDATION PASSED: {}".format(
        [(name, count, long_count) for name, _, _, _, count, _, long_count, _ in EXPECTED]
    ))


main()
