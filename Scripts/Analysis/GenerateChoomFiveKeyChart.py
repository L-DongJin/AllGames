"""Generate full-song 5-key chart timing candidates from human taps and WAV onsets."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path

from AnalyzeRhythmAudio import read_pcm16_mono, spectral_flux, strong_onsets


BPM = 120.0
ACOUSTIC_GRID_OFFSET = 0.0133
LISTENING_CALIBRATION = 0.1475
GAMEPLAY_GRID_OFFSET = ACOUSTIC_GRID_OFFSET - LISTENING_CALIBRATION
SIXTEENTH_SECONDS = 60.0 / BPM / 4.0


def load_human_taps(path: Path) -> list[float]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        return [float(row["target_time_seconds"]) for row in csv.DictReader(stream)]


def clean_human_taps(raw_times: list[float]) -> tuple[list[float], int, int]:
    result: list[float] = []
    snapped = 0
    preserved = 0
    for raw_time in raw_times:
        grid_index = round((raw_time - GAMEPLAY_GRID_OFFSET) / SIXTEENTH_SECONDS)
        grid_time = GAMEPLAY_GRID_OFFSET + grid_index * SIXTEENTH_SECONDS
        if abs(raw_time - grid_time) <= 0.035:
            target_time = grid_time
            snapped += 1
        else:
            target_time = raw_time
            preserved += 1
        if not result or target_time - result[-1] >= 0.090:
            result.append(target_time)
    return result, snapped, preserved


def generate_onset_timing(wav_path: Path) -> list[tuple[float, float]]:
    samples, sample_rate = read_pcm16_mono(wav_path)
    hop = 512
    envelope = spectral_flux(samples, sample_rate, hop=hop)
    onsets = strong_onsets(envelope, sample_rate / hop)

    grid_strength: dict[int, float] = {}
    for onset in onsets:
        if onset["time"] < 80.0 or onset["time"] > 174.0:
            continue
        grid_index = round((onset["time"] - ACOUSTIC_GRID_OFFSET) / SIXTEENTH_SECONDS)
        grid_strength[grid_index] = max(grid_strength.get(grid_index, 0.0), onset["strength"])

    selected: list[tuple[float, float]] = []
    for grid_index, strength in sorted(grid_strength.items()):
        if strength < 0.4:
            continue
        target_time = GAMEPLAY_GRID_OFFSET + grid_index * SIXTEENTH_SECONDS
        gap = target_time - selected[-1][0] if selected else 999.0
        if gap >= 0.24 or (gap >= 0.12 and strength >= 1.25):
            selected.append((target_time, strength))
    return selected


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--taps", required=True, type=Path)
    parser.add_argument("--wav", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    human_times, snapped, preserved = clean_human_taps(load_human_taps(args.taps))
    onset_times = generate_onset_timing(args.wav)
    rows = [(time, "human", 1.0) for time in human_times]
    for target_time, strength in onset_times:
        if rows and target_time - rows[-1][0] < 0.090:
            continue
        rows.append((target_time, "onset", strength))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(["target_time_seconds", "source", "strength"])
        for target_time, source, strength in rows:
            writer.writerow(["{:.4f}".format(target_time), source, "{:.4f}".format(strength)])

    print(
        "Generated {} full-song timing rows: {} human ({} snapped, {} preserved), {} onset"
        .format(len(rows), len(human_times), snapped, preserved, len(rows) - len(human_times))
    )
    print("First {:.4f}s, last {:.4f}s -> {}".format(rows[0][0], rows[-1][0], args.output))


if __name__ == "__main__":
    main()
