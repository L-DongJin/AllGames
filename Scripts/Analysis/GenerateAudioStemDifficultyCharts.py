"""Generate 5-key rhythm charts directly from aligned vocal/drum/bass/music WAV stems.

The separated stems are authoring inputs only. Runtime gameplay still uses one imported
master SoundWave and the generated URhythmSongDataAsset note arrays.
"""

from __future__ import annotations

import argparse
import csv
import json
import wave
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from AnalyzeRhythmAudio import read_pcm16_mono, spectral_flux, tempo_candidates
from GenerateChartQualityReport import generate_report


@dataclass(frozen=True)
class Profile:
    percentile: dict[str, float]
    minimum_gap: dict[str, float]
    final_gap: float
    roles: tuple[str, ...]
    long_threshold: float
    supplemental_ratio: float
    hold_max_taps: int
    hold_min_interval: float


PROFILES = {
    "Easy": Profile(
        {"vocals": 91.0, "drums": 92.0, "bass": 94.0, "music": 96.0},
        {"vocals": 0.24, "drums": 0.25, "bass": 0.40, "music": 0.50},
        0.13, ("vocals", "drums", "bass", "music"), 1.05, 0.02, 0, 30.0,
    ),
    "Normal": Profile(
        {"vocals": 87.0, "drums": 88.0, "bass": 91.0, "music": 94.0},
        {"vocals": 0.17, "drums": 0.18, "bass": 0.30, "music": 0.38},
        0.095, ("vocals", "drums", "bass", "music"), 0.85, 0.04, 1, 22.0,
    ),
    "Hard": Profile(
        {"vocals": 82.0, "drums": 83.0, "bass": 87.0, "music": 91.0},
        {"vocals": 0.115, "drums": 0.12, "bass": 0.21, "music": 0.27},
        0.070, ("vocals", "drums", "bass", "music"), 0.68, 0.07, 2, 16.0,
    ),
    "Expert": Profile(
        # Expert remains denser than Hard, but no longer keeps nearly every weak
        # stem onset. This targets the shared Lv20 ceiling and emphasizes audible
        # vocal/drum attacks instead of noisy bass/FX fill.
        {"vocals": 79.0, "drums": 79.0, "bass": 84.0, "music": 89.0},
        {"vocals": 0.09, "drums": 0.09, "bass": 0.16, "music": 0.20},
        0.06, ("vocals", "drums", "bass", "music"), 0.55, 0.08, 3, 12.0,
    ),
}

ROLE_PRIORITY = {"vocals": 4, "drums": 3, "bass": 2, "music": 1}
LANE_PATTERNS = {
    "vocals": (1, 2, 3, 2, 0, 2, 4, 2),
    "drums": (0, 4, 1, 3, 0, 4, 2),
    "bass": (0, 1, 0, 2, 4, 3, 4, 2),
    "music": (3, 1, 4, 0, 2),
}


def load_stem(path: Path) -> tuple[np.ndarray, int]:
    samples, rate = read_pcm16_mono(path)
    peak = float(np.max(np.abs(samples)))
    if peak > 1e-6:
        samples = samples / peak
    return samples, rate


def peak_events(envelope: np.ndarray, rate: float, percentile: float, minimum_gap: float, role: str) -> list[dict]:
    threshold = float(np.percentile(envelope, percentile))
    local_radius = max(1, round(0.035 * rate))
    candidates = []
    for index in range(local_radius, len(envelope) - local_radius):
        strength = float(envelope[index])
        if strength < threshold:
            continue
        if strength != float(envelope[index - local_radius:index + local_radius + 1].max()):
            continue
        candidates.append({"time": index / rate, "strength": strength, "role": role})

    selected = []
    for event in candidates:
        if not selected or event["time"] - selected[-1]["time"] >= minimum_gap:
            selected.append(event)
        elif event["strength"] > selected[-1]["strength"] * 1.12:
            selected[-1] = event
    return selected


def merge_events(role_events: dict[str, list[dict]], profile: Profile) -> list[dict]:
    primary = sorted(
        (dict(event) for role in ("vocals", "drums") for event in role_events[role]),
        key=lambda item: item["time"],
    )
    merged = []
    for event in primary:
        if not merged or event["time"] - merged[-1]["time"] > profile.final_gap:
            merged.append(event)
            continue
        # Strength is already normalized per stem. Role priority is only a tie-breaker;
        # it must not allow vocal separation leakage to erase clear drum attacks.
        existing_score = merged[-1]["strength"] + ROLE_PRIORITY[merged[-1]["role"]] * 0.04
        new_score = event["strength"] + ROLE_PRIORITY[event["role"]] * 0.04
        if new_score > existing_score:
            merged[-1] = event

    supplemental_limit = round(len(merged) * profile.supplemental_ratio)
    supplemental = sorted(
        (dict(event) for role in ("bass", "music") for event in role_events[role]),
        key=lambda item: item["strength"],
        reverse=True,
    )
    for event in supplemental:
        if supplemental_limit <= 0:
            break
        if all(abs(event["time"] - existing["time"]) >= max(profile.final_gap, 0.14) for existing in merged):
            merged.append(event)
            supplemental_limit -= 1
    merged.sort(key=lambda item: item["time"])
    return merged


def assign_lanes_and_holds(events: list[dict], vocals: np.ndarray, rate: int, profile: Profile) -> None:
    counters = {role: 0 for role in LANE_PATTERNS}
    last_lane = -1
    vocal_strengths = [event["strength"] for event in events if event["role"] == "vocals"]
    hold_strength = float(np.percentile(vocal_strengths, 94.0)) if vocal_strengths else float("inf")
    for index, event in enumerate(events):
        pattern = LANE_PATTERNS[event["role"]]
        lane = pattern[counters[event["role"]] % len(pattern)]
        counters[event["role"]] += 1
        if lane == last_lane:
            lane = (lane + 2) % 5
        event["lane"] = lane
        event["duration"] = 0.0
        if event["role"] == "vocals" and event["strength"] >= hold_strength:
            next_vocal_time = next(
                (later["time"] for later in events[index + 1:] if later["role"] == "vocals"),
                event["time"] + 3.0,
            )
            available = next_vocal_time - event["time"] - 0.12
            if available >= profile.long_threshold:
                event["duration"] = min(available, 2.25)
        last_lane = lane

    last_hold_time = float("-inf")
    for event in events:
        if event["duration"] <= 0.0:
            continue
        if event["time"] - last_hold_time < profile.hold_min_interval:
            event["duration"] = 0.0
        else:
            last_hold_time = event["time"]
    remove_ids = set()
    for index, event in enumerate(events):
        if event["duration"] <= 0.0:
            continue
        end_time = event["time"] + event["duration"]
        taps_during_hold = [
            later for later in events[index + 1:]
            if later["time"] < end_time and later["duration"] <= 0.0
        ]
        for extra in taps_during_hold[profile.hold_max_taps:]:
            remove_ids.add(id(extra))
    if remove_ids:
        events[:] = [event for event in events if id(event) not in remove_ids]

    next_time_by_lane = {}
    for event in reversed(events):
        next_time = next_time_by_lane.get(event["lane"])
        if event["duration"] > 0.0 and next_time is not None:
            event["duration"] = min(event["duration"], max(0.0, next_time - event["time"] - 0.12))
        if event["duration"] < 0.35:
            event["duration"] = 0.0
        next_time_by_lane[event["lane"]] = event["time"]


def write_chart(path: Path, events: list[dict]) -> None:
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(
            stream,
            fieldnames=("target_time_seconds", "duration_seconds", "lane_index", "role", "strength"),
        )
        writer.writeheader()
        for event in events:
            writer.writerow({
                "target_time_seconds": f"{event['time']:.4f}",
                "duration_seconds": f"{event['duration']:.4f}",
                "lane_index": event["lane"],
                "role": event["role"],
                "strength": f"{event['strength']:.4f}",
            })


def main() -> None:
    parser = argparse.ArgumentParser()
    for role in ("vocals", "drums", "bass", "music"):
        parser.add_argument(f"--{role}", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--song-name", required=True)
    # Gameplay notes need enough lead time to enter from the top of the lane after GO.
    # The production charts currently use a two-second base travel time, with a small
    # safety margin for the first frame and UI construction.
    parser.add_argument("--start-time", type=float, default=2.25)
    args = parser.parse_args()

    stems = {}
    sample_rate = None
    duration = None
    for role in ("vocals", "drums", "bass", "music"):
        samples, rate = load_stem(getattr(args, role))
        current_duration = len(samples) / rate
        if sample_rate is not None and (rate != sample_rate or abs(current_duration - duration) > 0.001):
            raise ValueError("Stem WAV files must use the same sample rate, start point, and duration")
        stems[role] = samples
        sample_rate = rate
        duration = current_duration

    hop = 512
    envelopes = {role: spectral_flux(samples, sample_rate, hop=hop) for role, samples in stems.items()}
    envelope_rate = sample_rate / hop
    tempos = tempo_candidates(envelopes["drums"], envelope_rate)
    bpm = tempos[0]["bpm"] if tempos else 120.0
    args.output_dir.mkdir(parents=True, exist_ok=True)
    summaries = {}

    for difficulty, profile in PROFILES.items():
        role_events = {
            role: peak_events(
                envelopes[role], envelope_rate,
                profile.percentile[role], profile.minimum_gap[role], role,
            )
            for role in profile.roles
        }
        events = merge_events(role_events, profile)
        events = [event for event in events if args.start_time <= event["time"] <= duration - 0.25]
        assign_lanes_and_holds(events, stems["vocals"], sample_rate, profile)
        write_chart(args.output_dir / f"{args.song_name}_{difficulty}_5Key.csv", events)
        lanes = [sum(event["lane"] == lane for event in events) for lane in range(5)]
        summaries[difficulty] = {
            "notes": len(events),
            "long_notes": sum(event["duration"] > 0.0 for event in events),
            "lanes": lanes,
            "first_time": events[0]["time"] if events else None,
            "last_time": events[-1]["time"] if events else None,
            "roles": {role: sum(event["role"] == role for event in events) for role in profile.roles},
        }
        print(difficulty, summaries[difficulty])

    metadata = {
        "song_name": args.song_name,
        "source": "aligned_audio_stems",
        "bpm": bpm,
        "audio_duration_seconds": duration,
        "music_offset_seconds": 0.0,
        "difficulties": summaries,
        "tempo_candidates": tempos,
    }
    (args.output_dir / f"{args.song_name}_metadata.json").write_text(
        json.dumps(metadata, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    report = generate_report(
        args.output_dir,
        args.song_name,
        [args.vocals, args.drums, args.bass, args.music],
        "combined aligned vocal/drum/bass/music stems",
    )
    print("Quality report", {
        difficulty: {
            "status": data["status"],
            "match": data["audio_onset_match_percent"],
            "review_windows": [window["label"] for window in data["recommended_listening_windows"]],
        }
        for difficulty, data in report["difficulties"].items()
    })


if __name__ == "__main__":
    main()
