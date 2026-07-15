"""Compare a human tap capture with the Choom beat grid and acoustic onsets."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path

import numpy as np

from AnalyzeRhythmAudio import read_pcm16_mono, spectral_flux, strong_onsets


def load_taps(path: Path) -> np.ndarray:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        return np.array(
            [float(row["target_time_seconds"]) for row in csv.DictReader(stream)],
            dtype=np.float64,
        )


def circular_grid_error(times: np.ndarray, interval: float, offset: float) -> np.ndarray:
    phase = np.mod(times - offset + interval * 0.5, interval) - interval * 0.5
    return np.abs(phase)


def best_grid_offset(times: np.ndarray, interval: float) -> tuple[float, float]:
    candidates = np.linspace(0.0, interval, 2000, endpoint=False)
    scores = [np.median(circular_grid_error(times, interval, value)) for value in candidates]
    index = int(np.argmin(scores))
    return float(candidates[index]), float(scores[index])


def section_stats(taps: np.ndarray, start: int = 0, end: int = 80, width: int = 5) -> list[dict]:
    result = []
    for left in range(start, end, width):
        selected = taps[(taps >= left) & (taps < left + width)]
        intervals = np.diff(selected)
        result.append({
            "range": "{}-{}".format(left, left + width),
            "tap_count": int(len(selected)),
            "median_interval": round(float(np.median(intervals)), 4) if len(intervals) else None,
        })
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", type=Path)
    parser.add_argument("wav", type=Path)
    args = parser.parse_args()

    taps = load_taps(args.csv)
    intervals = np.diff(taps)
    interval_groups = {
        "sixteenth_like_0.07_to_0.18": int(np.sum((intervals >= 0.07) & (intervals < 0.18))),
        "eighth_like_0.18_to_0.34": int(np.sum((intervals >= 0.18) & (intervals < 0.34))),
        "quarter_like_0.34_to_0.65": int(np.sum((intervals >= 0.34) & (intervals < 0.65))),
        "half_note_like_0.65_to_1.3": int(np.sum((intervals >= 0.65) & (intervals < 1.3))),
        "phrase_gap_1.3_plus": int(np.sum(intervals >= 1.3)),
    }

    eighth_offset, eighth_error = best_grid_offset(taps, 0.25)
    quarter_offset, quarter_error = best_grid_offset(taps, 0.5)
    calibrated_eighth_error = circular_grid_error(taps, 0.25, -0.1342)

    samples, sample_rate = read_pcm16_mono(args.wav)
    hop = 512
    envelope = spectral_flux(samples, sample_rate, hop=hop)
    onset_times = np.array(
        [item["time"] for item in strong_onsets(envelope, sample_rate / hop)],
        dtype=np.float64,
    )
    # The listening calibration found the game clock roughly 147.5 ms behind audible attacks.
    acoustic_tap_estimates = taps + 0.1475
    nearest_onset_distances = np.array([
        np.min(np.abs(onset_times - tap_time)) for tap_time in acoustic_tap_estimates
    ])
    # The accepted acoustic beat grid begins at 0.0133 s. At 120 BPM its sixteenth-note
    # subdivision is 0.125 s; this detects intentional syncopation between quarter beats.
    acoustic_sixteenth_error = circular_grid_error(acoustic_tap_estimates, 0.125, 0.0133)

    result = {
        "tap_count": int(len(taps)),
        "first_tap": round(float(taps[0]), 4),
        "last_tap": round(float(taps[-1]), 4),
        "median_interval": round(float(np.median(intervals)), 4),
        "interval_groups": interval_groups,
        "best_eighth_grid_offset_mod_0.25": round(eighth_offset, 5),
        "best_eighth_grid_median_error_ms": round(eighth_error * 1000.0, 2),
        "best_quarter_grid_offset_mod_0.5": round(quarter_offset, 5),
        "best_quarter_grid_median_error_ms": round(quarter_error * 1000.0, 2),
        "accepted_grid_median_error_ms": round(float(np.median(calibrated_eighth_error)) * 1000.0, 2),
        "acoustic_onset_match_median_ms": round(float(np.median(nearest_onset_distances)) * 1000.0, 2),
        "acoustic_onset_match_within_60ms_percent": round(float(np.mean(nearest_onset_distances <= 0.060) * 100.0), 1),
        "acoustic_sixteenth_grid_median_error_ms": round(float(np.median(acoustic_sixteenth_error)) * 1000.0, 2),
        "acoustic_sixteenth_grid_within_30ms_percent": round(float(np.mean(acoustic_sixteenth_error <= 0.030) * 100.0), 1),
        "sections": section_stats(taps),
    }
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
