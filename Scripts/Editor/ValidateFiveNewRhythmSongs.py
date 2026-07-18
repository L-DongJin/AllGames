import unreal


CATALOG_PATH = "/Game/Data/DA_RhythmSongCatalog"
PREFIXES = ("Sheesh", "Drip", "IveBangBang", "KiiiKiii404", "YenaCatch")
DIFFICULTIES = ("Easy", "Normal", "Hard", "Expert")


def main():
    catalog = unreal.load_asset(CATALOG_PATH)
    if catalog is None:
        raise RuntimeError("Rhythm song catalog is missing")
    catalog_names = {chart.get_name() for chart in catalog.get_editor_property("songs") if chart}
    summaries = []
    for prefix in PREFIXES:
        previous_count = -1
        for difficulty in DIFFICULTIES:
            name = "DA_{}_{}_5Key".format(prefix, difficulty)
            chart = unreal.load_asset("/Game/Data/{}".format(name))
            if chart is None or name not in catalog_names:
                raise RuntimeError("Missing chart or catalog entry: {}".format(name))
            notes = chart.get_editor_property("notes")
            if len(notes) <= previous_count:
                raise RuntimeError("Difficulty density did not increase: {}".format(name))
            previous_count = len(notes)
            last_time = -1.0
            for note in notes:
                time = note.get_editor_property("target_time_seconds")
                lane = note.get_editor_property("lane_index")
                if time <= last_time or not 0 <= lane < 5:
                    raise RuntimeError("Invalid order or lane in {}".format(name))
                last_time = time
            if chart.get_editor_property("song_id").is_none():
                raise RuntimeError("Missing stable SongId: {}".format(name))
            level = chart.get_editor_property("chart_level")
            if not 1 <= level <= 24:
                raise RuntimeError("Invalid chart level: {}".format(name))
            summaries.append((name, len(notes), level, round(last_time, 3)))
    if not unreal.EditorAssetLibrary.does_asset_exist("/Game/Maps/LobbyTestMap"):
        raise RuntimeError("LobbyTestMap is missing")
    unreal.log("FIVE NEW SONGS VALIDATED: {}".format(summaries))


main()
