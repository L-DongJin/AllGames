import unreal


EXPECTED = [
    ("DA_LoveAttack_Easy_5Key", "LoveAttack", "리센느-LoveAttack", unreal.RhythmDifficulty.EASY, 594, [117, 96, 164, 75, 142], 0, 177.0),
    ("DA_LoveAttack_Normal_5Key", "LoveAttack", "리센느-LoveAttack", unreal.RhythmDifficulty.NORMAL, 786, [157, 129, 218, 105, 177], 0, 178.0),
    ("DA_LoveAttack_Hard_5Key", "LoveAttack", "리센느-LoveAttack", unreal.RhythmDifficulty.HARD, 1095, [212, 178, 308, 150, 247], 1, 178.0),
    ("DA_LoveAttack_Expert_5Key", "LoveAttack", "리센느-LoveAttack", unreal.RhythmDifficulty.EXPERT, 1457, [276, 228, 418, 198, 337], 1, 179.0),
    ("DA_Suddenly_Easy_5Key", "갑자기", "아이오아이-갑자기", unreal.RhythmDifficulty.EASY, 523, [120, 119, 46, 119, 119], 13, 193.0),
    ("DA_Suddenly_Normal_5Key", "갑자기", "아이오아이-갑자기", unreal.RhythmDifficulty.NORMAL, 592, [136, 136, 51, 135, 134], 19, 193.0),
    ("DA_Suddenly_Hard_5Key", "갑자기", "아이오아이-갑자기", unreal.RhythmDifficulty.HARD, 857, [196, 195, 76, 196, 194], 39, 193.0),
    ("DA_Suddenly_Expert_5Key", "갑자기", "아이오아이-갑자기", unreal.RhythmDifficulty.EXPERT, 1052, [239, 239, 97, 239, 238], 76, 193.0),
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
        notes = chart.get_editor_property("notes")
        times = [note.get_editor_property("target_time_seconds") for note in notes]
        lanes = [note.get_editor_property("lane_index") for note in notes]
        durations = [note.get_editor_property("duration_seconds") for note in notes]
        require(len(notes) == count, "{} count is wrong".format(name))
        require(times == sorted(times) and len(times) == len(set(times)), "{} times are invalid".format(name))
        require([lanes.count(lane) for lane in range(5)] == lane_counts, "{} lane counts are wrong".format(name))
        require(sum(duration > 0.0 for duration in durations) == long_count, "{} hold count is wrong".format(name))
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

    require(len(catalog_names) == 36, "Catalog must contain nine songs and thirty-six charts")
    unreal.log("LOVEATTACK / SUDDENLY VALIDATION PASSED")


main()
