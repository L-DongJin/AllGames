import json
import os

import unreal


CATALOG_PATH = "/Game/Data/DA_RhythmSongCatalog"
OUTPUT_PATH = os.path.join(
    unreal.Paths.project_dir(), "Docs", "Analysis", "CurrentChartBalance.json"
)


def metrics(chart):
    notes = list(chart.get_editor_property("notes"))
    times = [float(note.get_editor_property("target_time_seconds")) for note in notes]
    durations = [float(note.get_editor_property("duration_seconds")) for note in notes]
    playable_duration = max(1.0, times[-1] - times[0]) if times else 1.0
    peak_two_second_nps = 0.0
    end_index = 0
    for start_index, start_time in enumerate(times):
        end_index = max(end_index, start_index)
        while end_index < len(times) and times[end_index] < start_time + 2.0:
            end_index += 1
        peak_two_second_nps = max(
            peak_two_second_nps, (end_index - start_index) / 2.0
        )
    return {
        "asset": chart.get_path_name(),
        "song_id": str(chart.get_editor_property("song_id")),
        "title": str(chart.get_editor_property("song_title")),
        "difficulty": str(chart.get_editor_property("difficulty")).split(".")[-1],
        "chart_level": int(chart.get_editor_property("chart_level")),
        "note_count": len(notes),
        "average_nps": round(len(notes) / playable_duration, 4),
        "peak_two_second_nps": round(peak_two_second_nps, 4),
        "hold_count": sum(duration > 0.0 for duration in durations),
        "hold_seconds": round(sum(min(duration, 2.25) for duration in durations), 4),
        "first_time": round(times[0], 4) if times else None,
        "last_time": round(times[-1], 4) if times else None,
    }


def main():
    catalog = unreal.load_asset(CATALOG_PATH)
    if catalog is None:
        raise RuntimeError("Rhythm song catalog is missing")
    rows = [
        metrics(chart)
        for chart in catalog.get_editor_property("songs")
        if chart is not None
    ]
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as stream:
        json.dump(rows, stream, ensure_ascii=False, indent=2)
    unreal.log("CURRENT CHART BALANCE EXPORTED: {} charts".format(len(rows)))


main()
