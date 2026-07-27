"""Apply the catalog-wide Lv1-20 ladder and import four reduced Expert charts."""

import csv
import os

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
REBUILT_EXPERTS = {
    "Drama": "Drama",
    "HeavySerenade": "HeavySerenade",
    "Rude": "Rude",
    "Sheesh": "Sheesh",
}
SOURCE_ROOT = os.path.join(
    unreal.Paths.project_dir(), "SourceData", "Rhythm", "Balance20260725"
)
REPORT_PATH = os.path.join(
    unreal.Paths.project_dir(), "Docs", "Analysis", "RhythmBalance20260725.md"
)


def load_notes(csv_path):
    with open(csv_path, "r", encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.DictReader(stream))
    notes = []
    previous_time = -1.0
    for row in rows:
        note = unreal.RhythmNoteData()
        target_time = float(row["target_time_seconds"])
        lane = int(row["lane_index"])
        if target_time <= previous_time or not 0 <= lane < 5:
            raise RuntimeError("Invalid chart row in {}".format(csv_path))
        note.set_editor_property("target_time_seconds", target_time)
        note.set_editor_property(
            "duration_seconds", float(row.get("duration_seconds", 0.0) or 0.0)
        )
        note.set_editor_property("lane_index", lane)
        notes.append(note)
        previous_time = target_time
    return notes


def main():
    summaries = []
    for prefix, levels in LEVELS.items():
        if len(levels) != 4 or any(
            levels[index + 1] - levels[index] < 2 for index in range(3)
        ):
            raise RuntimeError("Invalid difficulty ladder for {}".format(prefix))
        if levels[-1] > 20:
            raise RuntimeError("Level cap exceeded for {}".format(prefix))

        for difficulty_index, difficulty in enumerate(DIFFICULTIES):
            asset_name = "DA_{}_{}_5Key".format(prefix, difficulty)
            asset = unreal.load_asset("/Game/Data/{}".format(asset_name))
            if asset is None:
                raise RuntimeError("Missing chart asset {}".format(asset_name))

            if difficulty == "Expert" and prefix in REBUILT_EXPERTS:
                source_name = REBUILT_EXPERTS[prefix]
                csv_path = os.path.join(
                    SOURCE_ROOT,
                    source_name,
                    "{}_Expert_5Key.csv".format(source_name),
                )
                old_count = len(asset.get_editor_property("notes"))
                notes = load_notes(csv_path)
                if len(notes) >= old_count:
                    raise RuntimeError("Expert chart was not reduced: {}".format(asset_name))
                asset.set_editor_property("notes", notes)
                asset.set_editor_property(
                    "chart_version",
                    max(2, int(asset.get_editor_property("chart_version"))),
                )

            asset.set_editor_property("chart_level", levels[difficulty_index])
            unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
            summaries.append(
                (
                    asset_name,
                    levels[difficulty_index],
                    len(asset.get_editor_property("notes")),
                    int(asset.get_editor_property("chart_version")),
                )
            )

    lines = [
        "# Rhythm chart balance - 2026-07-25",
        "",
        "- Shared displayed level range: 1-20",
        "- Every song has at least two levels between adjacent difficulty names.",
        "- Drama, HeavySerenade, RUDE!, and SHEESH Expert were rebuilt from aligned WAV stems.",
        "- Rebuilt Expert charts increment their ChartVersion so old leaderboard scores are not mixed.",
        "",
        "| Chart | Level | Notes | Chart version |",
        "|---|---:|---:|---:|",
    ]
    lines.extend(
        "| {} | {} | {} | {} |".format(name, level, notes, version)
        for name, level, notes, version in summaries
    )
    os.makedirs(os.path.dirname(REPORT_PATH), exist_ok=True)
    with open(REPORT_PATH, "w", encoding="utf-8") as stream:
        stream.write("\n".join(lines) + "\n")
    unreal.log("RHYTHM BALANCE 20260725 APPLIED: {} charts".format(len(summaries)))


main()
