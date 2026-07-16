"""Create automatic rhythm-chart QA metrics and prioritized listening windows."""

from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path

import numpy as np

from AnalyzeRhythmAudio import read_pcm16_mono, spectral_flux


DIFFICULTIES = ("Easy", "Normal", "Hard", "Expert")
WINDOW_SECONDS = 5.0
MATCH_RADIUS_SECONDS = 0.10


def load_chart(path: Path) -> list[dict]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.DictReader(stream))
    return [
        {
            "time": float(row["target_time_seconds"]),
            "duration": float(row.get("duration_seconds", 0.0) or 0.0),
            "lane": int(row["lane_index"]),
        }
        for row in rows
    ]


def load_activity(paths: list[Path]) -> tuple[np.ndarray, float, float]:
    envelopes = []
    sample_rate = None
    duration = None
    hop = 512
    for path in paths:
        samples, rate = read_pcm16_mono(path)
        current_duration = len(samples) / rate
        if sample_rate is not None and (rate != sample_rate or abs(current_duration - duration) > 0.01):
            raise ValueError("Quality-report audio inputs must have matching duration and sample rate")
        envelope = spectral_flux(samples, rate, hop=hop)
        envelopes.append(envelope / max(float(np.percentile(envelope, 99.0)), 1e-6))
        sample_rate = rate
        duration = current_duration
    combined = np.maximum.reduce(envelopes)
    return combined, sample_rate / hop, duration


def robust_z(values: np.ndarray) -> np.ndarray:
    median = float(np.median(values))
    deviation = float(np.median(np.abs(values - median)))
    if deviation < 1e-6:
        return np.zeros_like(values)
    return (values - median) / (1.4826 * deviation)


def format_time(seconds: float) -> str:
    total = max(0, int(seconds))
    return f"{total // 60}:{total % 60:02d}"


def analyze_chart(notes: list[dict], envelope: np.ndarray, rate: float, duration: float) -> dict:
    times = np.array([note["time"] for note in notes], dtype=np.float64)
    lane_counts = [sum(note["lane"] == lane for note in notes) for lane in range(5)]
    intervals = np.diff(times)
    window_count = max(1, math.ceil(duration / WINDOW_SECONDS))
    window_starts = np.arange(window_count, dtype=np.float64) * WINDOW_SECONDS
    note_counts = np.array([
        int(np.sum((times >= start) & (times < start + WINDOW_SECONDS)))
        for start in window_starts
    ], dtype=np.float64)
    activity = np.array([
        float(np.mean(envelope[
            max(0, int(start * rate)):
            min(len(envelope), max(1, int((start + WINDOW_SECONDS) * rate)))
        ]))
        for start in window_starts
    ])

    peak_threshold = float(np.percentile(envelope, 78.0))
    peak_indices = np.flatnonzero(
        (envelope >= peak_threshold)
        & (envelope >= np.roll(envelope, 1))
        & (envelope >= np.roll(envelope, -1))
    )
    peak_times = peak_indices / rate
    nearest_errors = []
    for time in times:
        index = int(np.searchsorted(peak_times, time))
        candidates = peak_times[max(0, index - 1):min(len(peak_times), index + 2)]
        nearest_errors.append(float(np.min(np.abs(candidates - time))) if len(candidates) else 999.0)
    nearest_errors = np.array(nearest_errors)
    matched = nearest_errors <= MATCH_RADIUS_SECONDS

    density_z = robust_z(note_counts)
    activity_z = robust_z(activity)
    suspicious = []
    for index, start in enumerate(window_starts):
        # Gameplay intentionally reserves the opening for countdown and full note travel.
        # Do not report that expected preparation window as a sparse/low-match defect.
        if start < 2.0:
            continue
        in_window = (times >= start) & (times < start + WINDOW_SECONDS)
        local_match = float(np.mean(matched[in_window])) if np.any(in_window) else 0.0
        local_error = float(np.median(nearest_errors[in_window])) if np.any(in_window) else 999.0
        reasons = []
        score = 0.0
        if activity_z[index] >= 0.75 and density_z[index] <= -0.75:
            reasons.append("active audio but sparse notes")
            score += min(3.0, activity_z[index] - density_z[index])
        if density_z[index] >= 2.5:
            reasons.append("unusually high note density")
            score += min(3.0, density_z[index] - 1.5)
        if note_counts[index] >= 3 and local_match < 0.62:
            reasons.append("low audio-onset match")
            score += (0.72 - local_match) * 5.0
        if local_error > 0.075 and local_error < 10.0:
            reasons.append("large median onset distance")
            score += min(2.0, local_error * 8.0)
        if reasons:
            suspicious.append({
                "start_seconds": round(float(start), 3),
                "end_seconds": round(min(float(start + WINDOW_SECONDS), duration), 3),
                "label": f"{format_time(start)}-{format_time(min(start + WINDOW_SECONDS, duration))}",
                "score": round(score, 3),
                "note_count": int(note_counts[index]),
                "audio_activity": round(float(activity[index]), 4),
                "onset_match_percent": round(local_match * 100.0, 1),
                "median_onset_distance_ms": round(local_error * 1000.0, 1),
                "reasons": reasons,
            })
    suspicious.sort(key=lambda item: item["score"], reverse=True)

    longest_gap = float(np.max(intervals)) if len(intervals) else duration
    lane_mean = len(notes) / 5.0 if notes else 0.0
    lane_imbalance = (
        max(abs(count - lane_mean) for count in lane_counts) / lane_mean
        if lane_mean > 0.0 else 0.0
    )
    holds = [note for note in notes if note["duration"] > 0.0]
    overlap_count = 0
    for index, note in enumerate(notes):
        if note["duration"] <= 0.0:
            continue
        next_same_lane = next(
            (later["time"] for later in notes[index + 1:] if later["lane"] == note["lane"]),
            None,
        )
        if next_same_lane is not None and note["time"] + note["duration"] > next_same_lane - 0.1:
            overlap_count += 1

    match_percent = float(np.mean(matched) * 100.0) if len(matched) else 0.0
    median_error_ms = float(np.median(nearest_errors) * 1000.0) if len(nearest_errors) else 0.0
    status = "good"
    if overlap_count or match_percent < 55.0 or len(suspicious) >= 8:
        status = "review"
    elif match_percent < 70.0 or len(suspicious) >= 4 or lane_imbalance > 0.55:
        status = "caution"
    return {
        "status": status,
        "note_count": len(notes),
        "notes_per_second": round(len(notes) / max(duration, 1.0), 3),
        "maximum_five_second_density": int(np.max(note_counts)) if len(note_counts) else 0,
        "longest_note_gap_seconds": round(longest_gap, 3),
        "audio_onset_match_percent": round(match_percent, 1),
        "median_onset_distance_ms": round(median_error_ms, 1),
        "lane_counts": lane_counts,
        "lane_imbalance_percent": round(lane_imbalance * 100.0, 1),
        "long_note_count": len(holds),
        "long_note_overlap_count": overlap_count,
        "recommended_listening_windows": suspicious[:6],
    }


def write_markdown(path: Path, report: dict) -> None:
    lines = [
        f"# {report['song_name']} Chart Quality Report",
        "",
        f"- Audio duration: {report['audio_duration_seconds']:.3f} seconds",
        f"- Analysis source: {report['audio_analysis_source']}",
        "- This is an automatic risk report, not a guarantee of musical fun.",
        "",
    ]
    for difficulty in DIFFICULTIES:
        data = report["difficulties"][difficulty]
        lines.extend([
            f"## {difficulty}: {data['status'].upper()}",
            "",
            f"- Notes: {data['note_count']} ({data['notes_per_second']} per second)",
            f"- Audio-onset match: {data['audio_onset_match_percent']}%",
            f"- Median onset distance: {data['median_onset_distance_ms']} ms",
            f"- Maximum 5-second density: {data['maximum_five_second_density']}",
            f"- Longest note gap: {data['longest_note_gap_seconds']} seconds",
            f"- Lane counts: {data['lane_counts']} (imbalance {data['lane_imbalance_percent']}%)",
            f"- Long notes / overlaps: {data['long_note_count']} / {data['long_note_overlap_count']}",
            "",
            "### Recommended listening windows",
            "",
        ])
        windows = data["recommended_listening_windows"]
        if not windows:
            lines.append("- No high-risk window detected.")
        else:
            for window in windows:
                lines.append(
                    f"- {window['label']} — {', '.join(window['reasons'])}; "
                    f"{window['note_count']} notes, match {window['onset_match_percent']}%"
                )
        lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")


def generate_report(
    output_dir: Path,
    song_name: str,
    audio_paths: list[Path],
    audio_source_label: str,
) -> dict:
    envelope, rate, duration = load_activity(audio_paths)
    report = {
        "song_name": song_name,
        "audio_duration_seconds": duration,
        "audio_analysis_source": audio_source_label,
        "window_seconds": WINDOW_SECONDS,
        "match_radius_seconds": MATCH_RADIUS_SECONDS,
        "difficulties": {},
    }
    for difficulty in DIFFICULTIES:
        notes = load_chart(output_dir / f"{song_name}_{difficulty}_5Key.csv")
        report["difficulties"][difficulty] = analyze_chart(notes, envelope, rate, duration)
    json_path = output_dir / f"{song_name}_quality_report.json"
    markdown_path = output_dir / f"{song_name}_quality_report.md"
    json_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    write_markdown(markdown_path, report)
    return report


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--song-name", required=True)
    parser.add_argument("--audio", required=True, nargs="+", type=Path)
    parser.add_argument("--source-label", default="master WAV")
    args = parser.parse_args()
    report = generate_report(args.output_dir, args.song_name, args.audio, args.source_label)
    for difficulty, data in report["difficulties"].items():
        print(
            difficulty,
            data["status"],
            f"match={data['audio_onset_match_percent']}%",
            "review=", [window["label"] for window in data["recommended_listening_windows"]],
        )


if __name__ == "__main__":
    main()
