"""Create a role-aware 5-key chart from separated Vocal/Drums/Bass/Other MIDI transcriptions."""

from __future__ import annotations

import argparse
import bisect
import csv
import math
from pathlib import Path

import numpy as np

from AnalyzeRhythmAudio import read_pcm16_mono, spectral_flux, strong_onsets
from GenerateMidiFiveKeyChart import (
    LISTENING_CALIBRATION_SECONDS,
    collapse_onsets,
    estimate_audio_offset,
    parse_midi,
)


ROLE_RULES = {
    "vocal": (0.19, 50),
    "drums": (0.20, 40),
    "bass": (0.35, 60),
    "other": (0.40, 70),
}


def reduce_role(events: list[dict], role: str) -> list[dict]:
    minimum_gap, minimum_velocity = ROLE_RULES[role]
    reduced: list[dict] = []
    for raw in events:
        if raw["velocity"] < minimum_velocity:
            continue
        event = dict(raw)
        event["role"] = role
        event["supports"] = set()
        event["support_strength"] = 0
        if not reduced or event["midi_time"] - reduced[-1]["midi_time"] >= minimum_gap:
            reduced.append(event)
        elif event["velocity"] > reduced[-1]["velocity"] + 10:
            reduced[-1] = event
    return reduced


def nearest_index(events: list[dict], target_time: float) -> tuple[int, float]:
    times = [event["midi_time"] for event in events]
    index = bisect.bisect_left(times, target_time)
    choices = [choice for choice in (index - 1, index) if 0 <= choice < len(events)]
    if not choices:
        return -1, 999.0
    best = min(choices, key=lambda choice: abs(events[choice]["midi_time"] - target_time))
    return best, abs(events[best]["midi_time"] - target_time)


def has_nearby(events: list[dict], target_time: float, radius: float) -> bool:
    return nearest_index(events, target_time)[1] <= radius


def merge_roles(role_events: dict[str, list[dict]]) -> list[dict]:
    vocals = role_events["vocal"]
    output = list(vocals)

    # Drum attacks reinforce a coincident vocal syllable; otherwise they become standalone accents.
    for event in role_events["drums"]:
        vocal_index, distance = nearest_index(vocals, event["midi_time"])
        if distance <= 0.075:
            vocals[vocal_index]["supports"].add("drums")
            vocals[vocal_index]["support_strength"] = max(vocals[vocal_index]["support_strength"], event["velocity"])
        elif not has_nearby(sorted(output, key=lambda item: item["midi_time"]), event["midi_time"], 0.10):
            output.append(event)

    # Bass is used only to fill real vocal gaps, so it cannot double every melody onset.
    for event in role_events["bass"]:
        if not has_nearby(vocals, event["midi_time"], 0.45) \
                and not has_nearby(sorted(output, key=lambda item: item["midi_time"]), event["midi_time"], 0.20):
            output.append(event)

    # Other instruments contribute only sparse structural accents or chords in remaining space.
    for event in role_events["other"]:
        vocal_index, distance = nearest_index(vocals, event["midi_time"])
        if distance <= 0.060 and event["chord_size"] >= 2:
            vocals[vocal_index]["supports"].add("other")
            vocals[vocal_index]["support_strength"] = max(vocals[vocal_index]["support_strength"], event["velocity"])
        elif not has_nearby(vocals, event["midi_time"], 0.55) \
                and not has_nearby(sorted(output, key=lambda item: item["midi_time"]), event["midi_time"], 0.25):
            output.append(event)

    output.sort(key=lambda event: event["midi_time"])
    return output


def choose_shared_alignment(role_events: dict[str, list[dict]], wav_path: Path, bpm: float) -> tuple[float, dict[str, float]]:
    beat_seconds = 60.0 / bpm
    measured = {role: estimate_audio_offset(events, wav_path)[0] for role, events in role_events.items()}
    wrapped = {
        role: ((offset + beat_seconds * 0.5) % beat_seconds) - beat_seconds * 0.5
        for role, offset in measured.items()
    }
    common = float(np.median(list(wrapped.values())))
    inliers = [offset for offset in wrapped.values() if abs(offset - common) <= 0.12]
    if inliers:
        common = float(np.median(inliers))
    return common, measured


def refine_to_earlier_audio_attacks(events: list[dict], wav_path: Path, acoustic_offset: float) -> dict[str, list[float]]:
    samples, sample_rate = read_pcm16_mono(wav_path)
    hop = 512
    envelope = spectral_flux(samples, sample_rate, hop=hop)
    peaks = strong_onsets(envelope, sample_rate / hop)
    peak_times = [peak["time"] for peak in peaks]
    role_shifts: dict[str, list[float]] = {role: [] for role in ROLE_RULES}

    for event in events:
        predicted_acoustic_time = event["midi_time"] + acoustic_offset
        first = bisect.bisect_left(peak_times, predicted_acoustic_time - 0.14)
        last = bisect.bisect_right(peak_times, predicted_acoustic_time + 0.08)
        candidates = peaks[first:last]
        refinement = 0.0
        if candidates:
            closest_weighted = max(
                candidates,
                key=lambda peak: peak["strength"] * math.exp(-abs(peak["time"] - predicted_acoustic_time) / 0.075),
            )
            candidate_shift = closest_weighted["time"] - predicted_acoustic_time
            # The listening test reports late notes. Preserve already-correct/early notes and only
            # pull a MIDI onset toward a clearly earlier audible attack.
            if candidate_shift <= -0.015:
                refinement = max(candidate_shift, -0.14)
        event["audio_refinement"] = refinement
        event["target_time"] = predicted_acoustic_time + refinement - LISTENING_CALIBRATION_SECONDS
        role_shifts[event["role"]].append(refinement)
    return role_shifts


def assign_lanes(events: list[dict]) -> None:
    previous_lane = -1
    use_left_hand = True
    left_index = right_index = 0
    bass_index = 0

    for event in events:
        use_space = event["role"] == "drums" \
            or ("drums" in event["supports"] and event["support_strength"] >= 50) \
            or ("other" in event["supports"] and event["chord_size"] >= 2) \
            or (event["role"] == "other" and event["velocity"] >= 75)
        if use_space and previous_lane != 2:
            lane = 2
        elif event["role"] == "bass":
            lane = (0, 1, 3, 4)[bass_index % 4]
            bass_index += 1
        else:
            if use_left_hand:
                lane = (0, 1)[left_index % 2]
                left_index += 1
            else:
                lane = (3, 4)[right_index % 2]
                right_index += 1
            use_left_hand = not use_left_hand
        event["lane"] = lane
        previous_lane = lane


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--vocal", required=True, type=Path)
    parser.add_argument("--drums", required=True, type=Path)
    parser.add_argument("--bass", required=True, type=Path)
    parser.add_argument("--other", required=True, type=Path)
    parser.add_argument("--wav", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    paths = {role: getattr(args, role) for role in ROLE_RULES}
    raw_events = {}
    tempos = None
    for role, path in paths.items():
        notes, _, midi_tempos = parse_midi(path)
        raw_events[role] = collapse_onsets(notes)
        tempos = midi_tempos if tempos is None else tempos
    bpm = 60_000_000 / tempos[0][1]
    acoustic_offset, measured_offsets = choose_shared_alignment(raw_events, args.wav, bpm)
    reduced = {role: reduce_role(events, role) for role, events in raw_events.items()}
    merged = merge_roles(reduced)

    role_shifts = refine_to_earlier_audio_attacks(merged, args.wav, acoustic_offset)
    gameplay_events = [event for event in merged if 2.5 <= event["target_time"] <= 174.0]

    # Refining two neighboring MIDI events toward the same acoustic transient can make them nearly
    # simultaneous. Keep the musically higher-priority event instead of creating an accidental chord.
    role_priority = {"vocal": 4, "drums": 3, "bass": 2, "other": 1}
    separated: list[dict] = []
    for event in gameplay_events:
        if not separated or event["target_time"] - separated[-1]["target_time"] >= 0.09:
            separated.append(event)
        else:
            current_score = role_priority[event["role"]] * 100 + event["velocity"] + len(event["supports"]) * 20
            previous = separated[-1]
            previous_score = role_priority[previous["role"]] * 100 + previous["velocity"] + len(previous["supports"]) * 20
            if current_score > previous_score:
                separated[-1] = event
    gameplay_events = separated
    assign_lanes(gameplay_events)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="") as stream:
        fields = ["target_time_seconds", "lane_index", "role", "supports", "midi_time_seconds", "audio_refinement_seconds", "pitch", "velocity", "chord_size"]
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for event in gameplay_events:
            writer.writerow({
                "target_time_seconds": f"{event['target_time']:.4f}",
                "lane_index": event["lane"],
                "role": event["role"],
                "supports": "+".join(sorted(event["supports"])),
                "midi_time_seconds": f"{event['midi_time']:.4f}",
                "audio_refinement_seconds": f"{event['audio_refinement']:.4f}",
                "pitch": event["pitch"],
                "velocity": event["velocity"],
                "chord_size": event["chord_size"],
            })

    role_counts = {role: sum(event["role"] == role for event in gameplay_events) for role in ROLE_RULES}
    lane_counts = [sum(event["lane"] == lane for event in gameplay_events) for lane in range(5)]
    print("Raw onsets", {role: len(events) for role, events in raw_events.items()})
    print("Measured offsets", {role: round(value, 4) for role, value in measured_offsets.items()}, "shared", round(acoustic_offset, 4))
    print("Earlier audio refinements", {
        role: {
            "shifted": sum(shift < 0 for shift in shifts),
            "shifted_median_ms": round(float(np.median([shift for shift in shifts if shift < 0])) * 1000, 1)
                if any(shift < 0 for shift in shifts) else 0.0,
        }
        for role, shifts in role_shifts.items()
    })
    print(f"Role-aware Normal notes={len(gameplay_events)}, roles={role_counts}, lanes={lane_counts}")
    print(f"Range {gameplay_events[0]['target_time']:.4f}-{gameplay_events[-1]['target_time']:.4f}s -> {args.output}")


if __name__ == "__main__":
    main()
