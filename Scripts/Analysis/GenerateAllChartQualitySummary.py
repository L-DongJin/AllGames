"""Combine per-song chart QA reports into one catalog-level priority report."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


STATUS_RANK = {"good": 0, "caution": 1, "review": 2}


def load_reports(root: Path) -> list[dict]:
    reports = []
    for path in sorted(root.rglob("*_quality_report.json")):
        report = json.loads(path.read_text(encoding="utf-8"))
        report["_source_path"] = str(path)
        reports.append(report)
    return reports


def summarize(report: dict) -> dict:
    difficulties = report["difficulties"]
    worst_status = max(
        (data["status"] for data in difficulties.values()),
        key=lambda status: STATUS_RANK[status],
    )
    matches = [data["audio_onset_match_percent"] for data in difficulties.values()]
    windows = []
    for difficulty, data in difficulties.items():
        for window in data["recommended_listening_windows"]:
            windows.append({
                "difficulty": difficulty,
                "label": window["label"],
                "score": window["score"],
                "reasons": window["reasons"],
                "match_percent": window["onset_match_percent"],
            })
    windows.sort(key=lambda item: item["score"], reverse=True)
    return {
        "song_name": report["song_name"],
        "worst_status": worst_status,
        "minimum_match_percent": min(matches),
        "average_match_percent": round(sum(matches) / len(matches), 1),
        "difficulty_matches": {
            difficulty: data["audio_onset_match_percent"]
            for difficulty, data in difficulties.items()
        },
        "caution_count": sum(data["status"] == "caution" for data in difficulties.values()),
        "review_count": sum(data["status"] == "review" for data in difficulties.values()),
        "long_note_overlap_count": sum(
            data["long_note_overlap_count"] for data in difficulties.values()
        ),
        "priority_windows": windows[:5],
        "source_report": report["_source_path"],
    }


def write_markdown(path: Path, summaries: list[dict]) -> None:
    lines = [
        "# All Songs Chart Quality Summary",
        "",
        f"- Songs analyzed: {len(summaries)}",
        f"- Difficulties analyzed: {len(summaries) * 4}",
        "- Priority is based on automatic risk metrics; it is not a musical quality ranking.",
        "",
        "## Catalog ranking",
        "",
        "| Priority | Song | Status | Easy | Normal | Hard | Expert | Suggested first check |",
        "| ---: | --- | --- | ---: | ---: | ---: | ---: | --- |",
    ]
    for index, summary in enumerate(summaries, 1):
        matches = summary["difficulty_matches"]
        first_window = summary["priority_windows"][0] if summary["priority_windows"] else None
        first_label = (
            f"{first_window['difficulty']} {first_window['label']}"
            if first_window else "No high-risk window"
        )
        lines.append(
            f"| {index} | {summary['song_name']} | {summary['worst_status'].upper()} | "
            f"{matches['Easy']:.1f}% | {matches['Normal']:.1f}% | "
            f"{matches['Hard']:.1f}% | {matches['Expert']:.1f}% | {first_label} |"
        )
    lines.extend(["", "## Per-song listening priorities", ""])
    for summary in summaries:
        lines.extend([
            f"### {summary['song_name']} - {summary['worst_status'].upper()}",
            "",
            f"- Minimum / average onset match: {summary['minimum_match_percent']:.1f}% / "
            f"{summary['average_match_percent']:.1f}%",
            f"- Long-note overlaps: {summary['long_note_overlap_count']}",
        ])
        if not summary["priority_windows"]:
            lines.append("- No high-risk five-second window detected.")
        else:
            for window in summary["priority_windows"]:
                lines.append(
                    f"- {window['difficulty']} {window['label']} - "
                    f"{', '.join(window['reasons'])}; match {window['match_percent']:.1f}%"
                )
        lines.append("")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--recordings-root", required=True, type=Path)
    parser.add_argument("--output-json", required=True, type=Path)
    parser.add_argument("--output-markdown", required=True, type=Path)
    args = parser.parse_args()
    reports = load_reports(args.recordings_root)
    summaries = [summarize(report) for report in reports]
    summaries.sort(key=lambda item: (
        -STATUS_RANK[item["worst_status"]],
        item["minimum_match_percent"],
        -len(item["priority_windows"]),
        item["song_name"],
    ))
    output = {
        "song_count": len(summaries),
        "difficulty_count": len(summaries) * 4,
        "songs": summaries,
    }
    args.output_json.parent.mkdir(parents=True, exist_ok=True)
    args.output_json.write_text(json.dumps(output, ensure_ascii=False, indent=2), encoding="utf-8")
    write_markdown(args.output_markdown, summaries)
    print("Catalog quality summary:", [
        (item["song_name"], item["worst_status"], item["minimum_match_percent"])
        for item in summaries
    ])


if __name__ == "__main__":
    main()
