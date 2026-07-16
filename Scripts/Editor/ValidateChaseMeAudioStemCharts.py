import unreal


EXPECTED = [
    ("DA_CHASEME_Easy_5Key", unreal.RhythmDifficulty.EASY, 583, 1),
    ("DA_CHASEME_Normal_5Key", unreal.RhythmDifficulty.NORMAL, 791, 2),
    ("DA_CHASEME_Hard_5Key", unreal.RhythmDifficulty.HARD, 1146, 0),
    ("DA_CHASEME_Expert_5Key", unreal.RhythmDifficulty.EXPERT, 1513, 1),
]


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    catalog = unreal.load_asset("/Game/Data/DA_RhythmSongCatalog")
    require(catalog is not None, "Song catalog is missing")
    catalog_names = [asset.get_name() for asset in catalog.get_editor_property("songs") if asset]

    for name, difficulty, count, long_count in EXPECTED:
        chart = unreal.load_asset("/Game/Data/{}".format(name))
        require(chart is not None, "{} is missing".format(name))
        require(str(chart.get_editor_property("song_title")) == "CHASE-ME", "{} title is wrong".format(name))
        require(chart.get_editor_property("difficulty") == difficulty, "{} difficulty is wrong".format(name))
        require(chart.get_editor_property("music").get_name() == "CHASE-ME", "{} music is wrong".format(name))
        require(chart.get_editor_property("key_mode") == unreal.RhythmChartKeyMode.FIVE_KEY, "{} is not 5-key".format(name))
        notes = chart.get_editor_property("notes")
        require(len(notes) == count, "{} count is wrong".format(name))
        times = [note.get_editor_property("target_time_seconds") for note in notes]
        lanes = [note.get_editor_property("lane_index") for note in notes]
        durations = [note.get_editor_property("duration_seconds") for note in notes]
        require(times == sorted(times) and len(times) == len(set(times)), "{} times are invalid".format(name))
        require(all(0 <= lane < 5 for lane in lanes), "{} contains an invalid lane".format(name))
        require(sum(duration > 0.0 for duration in durations) == long_count, "{} long-note count is wrong".format(name))
        require(times[0] >= 2.25, "{} starts before notes have enough visual travel time".format(name))
        require(times[-1] >= 186.0, "{} ends before the song tail".format(name))
        require(name in catalog_names, "{} is not in the lobby catalog".format(name))

    require(len(catalog_names) == 16, "Catalog must contain four songs and sixteen charts")
    unreal.log("CHASE-ME AUDIO-STEM VALIDATION PASSED: {}".format(
        [(name, count, long_count) for name, _, count, long_count in EXPECTED]
    ))


main()
