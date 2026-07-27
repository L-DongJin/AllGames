import math

import unreal


CATALOG_PATH = "/Game/Data/DA_RhythmSongCatalog"
TITLE_BY_PREFIX = {
    "DA_Choom_": "/Game/Title/CHOOM",
    "DA_Lemonade_": "/Game/Title/Lemonade",
    "DA_ItsMe_": "/Game/Title/It_sMe",
    "DA_CHASEME_": "/Game/Title/체이스미",
    "DA_CanonD_": "/Game/Title/CanonD",
    "DA_Drama_": "/Game/Title/Drama",
    "DA_Bansanka_": "/Game/Title/만찬가",
    "DA_LoveAttack_": "/Game/Title/LoveAttack",
    "DA_Suddenly_": "/Game/Title/갑자기",
    "DA_HeavySerenade_": "/Game/Title/HeavySerenade",
    "DA_Rude_": "/Game/Title/RUDE_",
}


def chart_level(notes):
    if not notes:
        return 1
    times = [note.get_editor_property("target_time_seconds") for note in notes]
    durations = [note.get_editor_property("duration_seconds") for note in notes]
    playable_duration = max(1.0, times[-1] - times[0])
    average_nps = len(times) / playable_duration
    peak_two_second_nps = 0.0
    end_index = 0
    for start_index, start_time in enumerate(times):
        end_index = max(end_index, start_index)
        while end_index < len(times) and times[end_index] < start_time + 2.0:
            end_index += 1
        peak_two_second_nps = max(peak_two_second_nps, (end_index - start_index) / 2.0)
    hold_burden = sum(min(duration, 2.25) for duration in durations) / playable_duration
    raw = average_nps * 1.5 + peak_two_second_nps * 0.8 + hold_burden * 2.0
    return max(1, min(20, round(raw)))


def main():
    catalog = unreal.load_asset(CATALOG_PATH)
    if catalog is None:
        raise RuntimeError("Rhythm song catalog is missing")
    summaries = []
    for chart in catalog.get_editor_property("songs"):
        if chart is None:
            continue
        prefix = next((key for key in TITLE_BY_PREFIX if chart.get_name().startswith(key)), None)
        if prefix is None:
            raise RuntimeError("No title-image mapping for {}".format(chart.get_name()))
        texture = unreal.load_asset(TITLE_BY_PREFIX[prefix])
        if texture is None:
            raise RuntimeError("Missing title texture {}".format(TITLE_BY_PREFIX[prefix]))
        level = chart_level(chart.get_editor_property("notes"))
        chart.set_editor_property("title_image", texture)
        chart.set_editor_property("chart_level", level)
        unreal.EditorAssetLibrary.save_loaded_asset(chart, only_if_is_dirty=False)
        summaries.append("{}=Lv{}".format(chart.get_name(), level))
    unreal.log("CHART LEVELS / TITLE IMAGES READY: {}".format(", ".join(summaries)))


main()
