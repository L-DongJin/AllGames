import unreal


EXPECTED = [
    ("DA_HeavySerenade_Easy_5Key", "HeavySerenade", "엔믹스-HeavySerenade", unreal.RhythmDifficulty.EASY, 471, [89, 68, 152, 61, 101], 2, 176.0),
    ("DA_HeavySerenade_Normal_5Key", "HeavySerenade", "엔믹스-HeavySerenade", unreal.RhythmDifficulty.NORMAL, 752, [145, 115, 226, 99, 167], 1, 177.0),
    ("DA_HeavySerenade_Hard_5Key", "HeavySerenade", "엔믹스-HeavySerenade", unreal.RhythmDifficulty.HARD, 1063, [195, 188, 303, 149, 228], 0, 177.0),
    ("DA_HeavySerenade_Expert_5Key", "HeavySerenade", "엔믹스-HeavySerenade", unreal.RhythmDifficulty.EXPERT, 1539, [289, 243, 447, 220, 340], 0, 178.0),
    ("DA_Rude_Easy_5Key", "RUDE!", "Hearts2Hearts-RUDE", unreal.RhythmDifficulty.EASY, 524, [103, 77, 151, 73, 120], 0, 195.0),
    ("DA_Rude_Normal_5Key", "RUDE!", "Hearts2Hearts-RUDE", unreal.RhythmDifficulty.NORMAL, 858, [174, 140, 224, 115, 205], 2, 195.0),
    ("DA_Rude_Hard_5Key", "RUDE!", "Hearts2Hearts-RUDE", unreal.RhythmDifficulty.HARD, 1177, [237, 190, 302, 168, 280], 1, 195.0),
    ("DA_Rude_Expert_5Key", "RUDE!", "Hearts2Hearts-RUDE", unreal.RhythmDifficulty.EXPERT, 1731, [321, 281, 474, 250, 405], 1, 195.0),
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
        require(times[0] >= 2.25 and times[-1] >= minimum_last_time, "{} range is wrong".format(name))
        require(name in catalog_names, "{} is not in the catalog".format(name))
    require(len(catalog_names) == 44, "Catalog must contain eleven songs and forty-four charts")
    unreal.log("HEAVY SERENADE / RUDE VALIDATION PASSED")


main()
