import unreal


CATALOG_PATH = "/Game/Data/DA_RhythmSongCatalog"
DIFFICULTIES = ("Easy", "Normal", "Hard", "Expert")
EXPECTED = {
    "Choom": ((422, 1), (501, 1), (574, 2), (615, 3)),
    "Lemonade": ((544, 2), (619, 3), (753, 3), (816, 7)),
    "ItsMe": ((307, 2), (359, 2), (452, 3), (488, 5)),
    "CHASEME": ((588, 1), (742, 2), (1128, 0), (1422, 1)),
    # CANON-D is intentionally excluded from the rebalance and must remain byte-for-byte
    # semantically equivalent to its previous chart counts.
    "CanonD": ((403, 2), (495, 3), (927, 7), (1182, 15)),
    "Drama": ((547, 2), (866, 1), (1169, 1), (1663, 0)),
    "Bansanka": ((628, 1), (725, 4), (907, 4), (973, 7)),
    "LoveAttack": ((606, 0), (714, 0), (1056, 0), (1402, 0)),
    "Suddenly": ((527, 5), (593, 7), (687, 10), (754, 12)),
    "HeavySerenade": ((479, 1), (731, 0), (1014, 0), (1479, 0)),
    "Rude": ((535, 0), (838, 1), (1071, 0), (1630, 0)),
}
HOLD_MAX_TAPS = (0, 1, 2, 3)


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    catalog = unreal.load_asset(CATALOG_PATH)
    require(catalog is not None, "Rhythm song catalog is missing")
    songs = catalog.get_editor_property("songs")
    require(len(songs) == 44, "Catalog must contain 44 production charts")

    by_name = {song.get_name(): song for song in songs if song}
    for prefix, expected_difficulties in EXPECTED.items():
        counts = []
        for difficulty_index, difficulty_name in enumerate(DIFFICULTIES):
            asset_name = "DA_{}_{}_5Key".format(prefix, difficulty_name)
            chart = by_name.get(asset_name)
            require(chart is not None, "Missing chart {}".format(asset_name))
            notes = chart.get_editor_property("notes")
            expected_notes, expected_holds = expected_difficulties[difficulty_index]
            require(len(notes) == expected_notes, "Unexpected note count in {}".format(asset_name))
            times = [note.get_editor_property("target_time_seconds") for note in notes]
            lanes = [note.get_editor_property("lane_index") for note in notes]
            durations = [note.get_editor_property("duration_seconds") for note in notes]
            require(times == sorted(times), "Unsorted note times in {}".format(asset_name))
            require(len(times) == len(set(times)), "Simultaneous/duplicate notes in {}".format(asset_name))
            require(all(0 <= lane < 5 for lane in lanes), "Invalid lane in {}".format(asset_name))
            require(
                sum(duration > 0.0 for duration in durations) == expected_holds,
                "Unexpected hold count in {}".format(asset_name),
            )
            for index, duration in enumerate(durations):
                if duration <= 0.0:
                    continue
                if prefix == "CanonD":
                    continue
                end_time = times[index] + duration
                taps_during_hold = sum(
                    times[index] < times[later] < end_time and durations[later] <= 0.0
                    for later in range(index + 1, len(notes))
                )
                require(
                    taps_during_hold <= HOLD_MAX_TAPS[difficulty_index],
                    "Hold overload in {} at {:.3f}".format(asset_name, times[index]),
                )
            counts.append(len(notes))
        require(counts == sorted(counts) and len(set(counts)) == 4, "Difficulty density failed for {}".format(prefix))

    unreal.log("RHYTHM REBALANCE VALIDATION PASSED: 44 charts, CANON-D preserved")


main()
