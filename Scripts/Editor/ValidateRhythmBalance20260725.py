import unreal


DIFFICULTIES = ("Easy", "Normal", "Hard", "Expert")
LEVELS = {
    "Choom": (7, 9, 11, 13),
    "Lemonade": (8, 10, 12, 14),
    "ItsMe": (7, 9, 11, 13),
    "CHASEME": (9, 11, 16, 20),
    "Drama": (8, 11, 15, 18),
    "Bansanka": (8, 10, 12, 14),
    "LoveAttack": (9, 12, 15, 20),
    "Suddenly": (8, 10, 12, 14),
    "HeavySerenade": (8, 11, 15, 19),
    "Rude": (8, 12, 15, 19),
    "CanonD": (8, 10, 14, 17),
    "Sheesh": (8, 11, 14, 18),
    "Drip": (8, 11, 15, 20),
    "IveBangBang": (9, 11, 13, 15),
    "KiiiKiii404": (9, 11, 13, 15),
    "YenaCatch": (9, 11, 13, 15),
}
REBUILT_EXPERT_COUNTS = {
    "Drama": 1455,
    "HeavySerenade": 1297,
    "Rude": 1389,
    "Sheesh": 1120,
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    catalog = unreal.load_asset("/Game/Data/DA_RhythmSongCatalog")
    require(catalog is not None, "Rhythm song catalog is missing")
    catalog_names = {
        chart.get_name()
        for chart in catalog.get_editor_property("songs")
        if chart is not None
    }
    require(len(catalog_names) == 64, "Catalog must contain 64 unique charts")

    for prefix, expected_levels in LEVELS.items():
        actual_levels = []
        counts = []
        for difficulty_index, difficulty in enumerate(DIFFICULTIES):
            asset_name = "DA_{}_{}_5Key".format(prefix, difficulty)
            require(asset_name in catalog_names, "Catalog entry missing: {}".format(asset_name))
            chart = unreal.load_asset("/Game/Data/{}".format(asset_name))
            require(chart is not None, "Chart missing: {}".format(asset_name))
            level = int(chart.get_editor_property("chart_level"))
            require(
                level == expected_levels[difficulty_index],
                "Unexpected level in {}: {}".format(asset_name, level),
            )
            notes = list(chart.get_editor_property("notes"))
            times = [
                float(note.get_editor_property("target_time_seconds")) for note in notes
            ]
            lanes = [int(note.get_editor_property("lane_index")) for note in notes]
            require(times == sorted(times), "Unsorted notes: {}".format(asset_name))
            require(len(times) == len(set(times)), "Simultaneous notes: {}".format(asset_name))
            require(all(0 <= lane < 5 for lane in lanes), "Invalid lane: {}".format(asset_name))
            actual_levels.append(level)
            counts.append(len(notes))

        require(max(actual_levels) <= 20, "Level cap exceeded: {}".format(prefix))
        require(
            all(actual_levels[index + 1] - actual_levels[index] >= 2 for index in range(3)),
            "Difficulty level gap is too small: {}".format(prefix),
        )
        require(
            counts == sorted(counts) and len(set(counts)) == 4,
            "Difficulty note counts do not increase: {}".format(prefix),
        )
        if prefix in REBUILT_EXPERT_COUNTS:
            require(
                counts[-1] == REBUILT_EXPERT_COUNTS[prefix],
                "Unexpected rebuilt Expert count: {}".format(prefix),
            )
            expert = unreal.load_asset(
                "/Game/Data/DA_{}_Expert_5Key".format(prefix)
            )
            require(
                int(expert.get_editor_property("chart_version")) >= 2,
                "Rebuilt Expert ChartVersion was not incremented: {}".format(prefix),
            )

    unreal.log("RHYTHM BALANCE 20260725 VALIDATED: 16 songs / 64 charts")


main()
