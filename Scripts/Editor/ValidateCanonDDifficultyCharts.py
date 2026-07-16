import unreal


EXPECTED = [
    ("DA_CanonD_Easy_5Key", unreal.RhythmDifficulty.EASY, 403, [89, 87, 52, 88, 87], 2),
    ("DA_CanonD_Normal_5Key", unreal.RhythmDifficulty.NORMAL, 495, [109, 109, 60, 109, 108], 3),
    ("DA_CanonD_Hard_5Key", unreal.RhythmDifficulty.HARD, 927, [204, 204, 112, 204, 203], 7),
    ("DA_CanonD_Expert_5Key", unreal.RhythmDifficulty.EXPERT, 1182, [266, 265, 120, 266, 265], 15),
]


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    catalog = unreal.load_asset("/Game/Data/DA_RhythmSongCatalog")
    require(catalog is not None, "Song catalog is missing")
    catalog_names = [asset.get_name() for asset in catalog.get_editor_property("songs") if asset]

    for name, difficulty, count, lane_counts, long_count in EXPECTED:
        chart = unreal.load_asset("/Game/Data/{}".format(name))
        require(chart is not None, "{} is missing".format(name))
        require(str(chart.get_editor_property("song_title")) == "CANON-D", "{} title is wrong".format(name))
        require(chart.get_editor_property("difficulty") == difficulty, "{} difficulty is wrong".format(name))
        require(chart.get_editor_property("music").get_name() == "CANON-D", "{} music is wrong".format(name))
        require(chart.get_editor_property("key_mode") == unreal.RhythmChartKeyMode.FIVE_KEY, "{} is not 5-key".format(name))
        notes = chart.get_editor_property("notes")
        require(len(notes) == count, "{} count is wrong".format(name))
        times = [note.get_editor_property("target_time_seconds") for note in notes]
        lanes = [note.get_editor_property("lane_index") for note in notes]
        durations = [note.get_editor_property("duration_seconds") for note in notes]
        require(times == sorted(times) and len(times) == len(set(times)), "{} times are invalid".format(name))
        require([lanes.count(lane) for lane in range(5)] == lane_counts, "{} lane counts are wrong".format(name))
        require(sum(duration > 0.0 for duration in durations) == long_count, "{} hold count is wrong".format(name))
        require(times[0] >= 2.0 and times[-1] >= 196.0, "{} playable range is wrong".format(name))
        require(name in catalog_names, "{} is not in the lobby catalog".format(name))

    require(len(catalog_names) == 20, "Catalog must contain five songs and twenty charts")
    unreal.log("CANON-D VALIDATION PASSED: {}".format(
        [(name, count, long_count) for name, _, count, _, long_count in EXPECTED]
    ))


main()
