"""Generate true Easy/Normal/Hard/Expert 5-key charts from separated Choom MIDI stems."""

from __future__ import annotations

import argparse
import csv
import json
import wave
from dataclasses import dataclass
from pathlib import Path

from GenerateMidiFiveKeyChart import LISTENING_CALIBRATION_SECONDS, collapse_onsets, parse_midi
from GenerateMultiTrackMidiFiveKeyChart import (
    assign_lanes,
    choose_shared_alignment,
    has_nearby,
    nearest_index,
    refine_to_earlier_audio_attacks,
)
from GenerateChartQualityReport import generate_report


PLAYTEST_ADVANCE_SECONDS = 0.045


@dataclass(frozen=True)
class Profile:
    vocal_gap: float
    vocal_velocity: int
    drum_gap: float
    drum_velocity: int
    bass_gap: float
    bass_velocity: int
    other_gap: float
    other_velocity: int
    bass_vocal_radius: float
    other_vocal_radius: float
    final_gap: float
    hook_max_notes: int
    hook_min_score: float
    long_note_min_duration: float
    supplemental_ratio: float
    hold_max_taps: int
    hold_min_interval: float


PROFILES = {
    # The previous Normal/Hard/Expert profiles become the new Easy/Normal/Hard baseline.
    "Easy": Profile(0.19, 50, 0.20, 40, 0.35, 60, 0.40, 70, 0.45, 0.55, 0.09, 4, 137.0, 0.80, 0.03, 0, 30.0),
    "Normal": Profile(0.14, 42, 0.16, 36, 0.24, 52, 0.28, 62, 0.24, 0.32, 0.075, 6, 130.0, 0.65, 0.05, 1, 22.0),
    "Hard": Profile(0.09, 32, 0.10, 30, 0.14, 42, 0.14, 50, 0.10, 0.14, 0.06, 8, 125.0, 0.55, 0.08, 2, 16.0),
    # Expert preserves faster articulation and more uncovered accompaniment than the former Expert.
    "Expert": Profile(0.055, 20, 0.06, 20, 0.085, 30, 0.085, 38, 0.04, 0.06, 0.045, 12, 110.0, 0.45, 0.12, 3, 12.0),
}


# Memorable playable phrases should come from what the player actually hears as the lead.
# Bass/Other remain optional gap-filling material and must not become independent hook streams.
HOOK_ROLES = ("vocal",)
HOOK_ROLE_WEIGHT = {"vocal": 1.15}


def detect_role_bursts(events: list[dict], role: str) -> list[dict]:
    """Find syllable/fill-like runs before ordinary density reduction can erase them."""
    bursts = []
    current = []

    def finish() -> None:
        nonlocal current
        repeated_pair = (
            len(current) == 2
            and current[0]["pitch"] == current[1]["pitch"]
            and min(event["velocity"] for event in current) >= 45
            and max(event["velocity"] for event in current) >= 65
        )
        if len(current) >= 3 or repeated_pair:
            velocities = [event["velocity"] for event in current]
            score = (
                max(velocities) + sum(velocities) / len(velocities) * 0.5
                + len(current) * 8 + (20 if repeated_pair else 0)
            ) * HOOK_ROLE_WEIGHT[role]
            bursts.append({
                "role": role,
                "start": current[0]["midi_time"],
                "end": current[-1]["midi_time"],
                "events": list(current),
                "score": score,
            })
        current = []

    for event in events:
        if not current:
            current = [event]
            continue
        gap = event["midi_time"] - current[-1]["midi_time"]
        if 0.045 <= gap <= 0.23:
            current.append(event)
        else:
            finish()
            current = [event]
    finish()
    return bursts


def detect_hook_clusters(raw: dict[str, list[dict]]) -> list[dict]:
    """Merge leaking stem transcriptions and keep one clean representative pattern."""
    bursts = [
        burst
        for role in HOOK_ROLES
        for burst in detect_role_bursts(raw[role], role)
    ]
    bursts.sort(key=lambda burst: burst["start"])
    groups = []
    for burst in bursts:
        # Compare with the most recently started burst rather than the group's maximum end.
        # This prevents several neighboring but distinct phrases from merging transitively.
        if not groups or burst["start"] > groups[-1][-1]["end"] + 0.12:
            groups.append([burst])
        else:
            groups[-1].append(burst)

    clusters = []
    for hook_id, group in enumerate(groups):
        representative = max(group, key=lambda burst: burst["score"])
        cluster = dict(representative)
        cluster["hook_id"] = hook_id
        cluster["window_start"] = min(burst["start"] for burst in group)
        cluster["window_end"] = max(burst["end"] for burst in group)
        clusters.append(cluster)
    return clusters


def evenly_spaced(events: list[dict], maximum: int) -> list[dict]:
    if len(events) <= maximum:
        return events
    indices = [round(index * (len(events) - 1) / (maximum - 1)) for index in range(maximum)]
    return [events[index] for index in indices]


def select_hook_events(clusters: list[dict], profile: Profile) -> tuple[list[dict], list[dict]]:
    selected_clusters = [cluster for cluster in clusters if cluster["score"] >= profile.hook_min_score]
    selected_events = []
    for cluster in selected_clusters:
        for raw_event in evenly_spaced(cluster["events"], profile.hook_max_notes):
            event = dict(raw_event)
            event.update(
                role=cluster["role"], supports=set(), support_strength=0,
                is_hook=True, hook_id=cluster["hook_id"],
            )
            selected_events.append(event)
    return selected_clusters, selected_events


def reduce_events(events: list[dict], role: str, minimum_gap: float, minimum_velocity: int) -> list[dict]:
    output = []
    for raw in events:
        if raw["velocity"] < minimum_velocity:
            continue
        event = dict(raw)
        event.update(role=role, supports=set(), support_strength=0)
        if not output or event["midi_time"] - output[-1]["midi_time"] >= minimum_gap:
            output.append(event)
        elif event["velocity"] > output[-1]["velocity"] + 10:
            output[-1] = event
    return output


def merge_profile(raw: dict[str, list[dict]], profile: Profile, hook_clusters: list[dict]) -> tuple[list[dict], list[dict]]:
    reduced = {
        "vocal": reduce_events(raw["vocal"], "vocal", profile.vocal_gap, profile.vocal_velocity),
        "drums": reduce_events(raw["drums"], "drums", profile.drum_gap, profile.drum_velocity),
        "bass": reduce_events(raw["bass"], "bass", profile.bass_gap, profile.bass_velocity),
        "other": reduce_events(raw["other"], "other", profile.other_gap, profile.other_velocity),
    }
    selected_clusters, hook_events = select_hook_events(hook_clusters, profile)

    def inside_hook(event: dict) -> bool:
        return any(
            cluster["window_start"] - 0.04 <= event["midi_time"] <= cluster["window_end"] + 0.04
            for cluster in selected_clusters
        )

    # Easy through Normal prioritize a clean readable motif. Hard and Expert preserve the motif
    # while layering valid surrounding attacks; Expert uses substantially tighter source filters.
    if profile.hook_max_notes < 8:
        for role in reduced:
            reduced[role] = [event for event in reduced[role] if not inside_hook(event)]

    vocals = reduced["vocal"]
    output = list(vocals)

    for event in reduced["drums"]:
        index, distance = nearest_index(vocals, event["midi_time"])
        if distance <= 0.075:
            vocals[index]["supports"].add("drums")
            vocals[index]["support_strength"] = max(vocals[index]["support_strength"], event["velocity"])
        elif not has_nearby(sorted(output, key=lambda item: item["midi_time"]), event["midi_time"], profile.final_gap):
            output.append(event)

    supplemental = []
    for event in reduced["bass"]:
        if not has_nearby(vocals, event["midi_time"], profile.bass_vocal_radius):
            supplemental.append(event)

    for event in reduced["other"]:
        index, distance = nearest_index(vocals, event["midi_time"])
        if distance <= 0.06 and event["chord_size"] >= 2:
            vocals[index]["supports"].add("other")
            vocals[index]["support_strength"] = max(vocals[index]["support_strength"], event["velocity"])
        elif not has_nearby(vocals, event["midi_time"], profile.other_vocal_radius):
            supplemental.append(event)

    # Bass and accompaniment are allowed only as sparse punctuation in gaps left by vocals/drums.
    # Prefer the strongest candidates and cap their share so they cannot create a noisy second chart.
    supplement_limit = round(len(output) * profile.supplemental_ratio)
    for event in sorted(
        supplemental,
        key=lambda item: (item["velocity"] + item.get("chord_size", 1) * 8),
        reverse=True,
    ):
        if supplement_limit <= 0:
            break
        ordered_output = sorted(output, key=lambda item: item["midi_time"])
        if not has_nearby(ordered_output, event["midi_time"], max(profile.final_gap, 0.12)):
            output.append(event)
            supplement_limit -= 1
    output.extend(hook_events)
    return sorted(output, key=lambda event: event["midi_time"]), selected_clusters


def separate_close_events(events: list[dict], minimum_gap: float) -> list[dict]:
    priority = {"vocal": 4, "drums": 3, "bass": 2, "other": 1}
    output = []
    for event in sorted(events, key=lambda item: item["target_time"]):
        previous = output[-1] if output else None
        effective_gap = min(minimum_gap, 0.055) if previous and (event.get("is_hook") or previous.get("is_hook")) else minimum_gap
        if not output or event["target_time"] - output[-1]["target_time"] >= effective_gap:
            output.append(event)
            continue
        score = priority[event["role"]] * 100 + event["velocity"] + len(event["supports"]) * 20 + (1000 if event.get("is_hook") else 0)
        previous_score = priority[previous["role"]] * 100 + previous["velocity"] + len(previous["supports"]) * 20 + (1000 if previous.get("is_hook") else 0)
        if score > previous_score:
            output[-1] = event
    return output


def assign_long_note_durations(events: list[dict], profile: Profile) -> None:
    """Keep clear vocal sustains and limit extra taps while either hand is occupied."""
    for event in events:
        source_duration = event.get("duration", 0.0)
        event["duration_seconds"] = (
            min(source_duration, 2.25)
            if event["role"] == "vocal" and source_duration >= profile.long_note_min_duration
            else 0.0
        )
    last_hold_time = float("-inf")
    for event in events:
        if event["duration_seconds"] <= 0.0:
            continue
        if event["target_time"] - last_hold_time < profile.hold_min_interval:
            event["duration_seconds"] = 0.0
        else:
            last_hold_time = event["target_time"]
    remove_ids = set()
    for index, event in enumerate(events):
        if event["duration_seconds"] <= 0.0:
            continue
        end_time = event["target_time"] + event["duration_seconds"]
        taps_during_hold = [
            later for later in events[index + 1:]
            if later["target_time"] < end_time and later["duration_seconds"] <= 0.0
        ]
        for extra in taps_during_hold[profile.hold_max_taps:]:
            remove_ids.add(id(extra))
    if remove_ids:
        events[:] = [event for event in events if id(event) not in remove_ids]
    next_time_by_lane = {}
    for event in reversed(events):
        next_time = next_time_by_lane.get(event["lane"])
        if event["duration_seconds"] > 0.0 and next_time is not None:
            event["duration_seconds"] = min(
                event["duration_seconds"], max(0.0, next_time - event["target_time"] - 0.12)
            )
            if event["duration_seconds"] < 0.35:
                event["duration_seconds"] = 0.0
        next_time_by_lane[event["lane"]] = event["target_time"]


def write_chart(path: Path, events: list[dict]) -> None:
    fields = ["target_time_seconds", "duration_seconds", "lane_index", "role", "supports", "hook_id", "midi_time_seconds", "audio_refinement_seconds", "pitch", "velocity", "chord_size"]
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for event in events:
            writer.writerow({
                "target_time_seconds": f"{event['target_time']:.4f}",
                "duration_seconds": f"{event.get('duration_seconds', 0.0):.4f}",
                "lane_index": event["lane"],
                "role": event["role"],
                "supports": "+".join(sorted(event["supports"])),
                "hook_id": event.get("hook_id", ""),
                "midi_time_seconds": f"{event['midi_time']:.4f}",
                "audio_refinement_seconds": f"{event['audio_refinement']:.4f}",
                "pitch": event["pitch"],
                "velocity": event["velocity"],
                "chord_size": event["chord_size"],
            })


def main() -> None:
    parser = argparse.ArgumentParser()
    for role in ("vocal", "drums", "bass", "other"):
        parser.add_argument(f"--{role}", required=True, type=Path)
    parser.add_argument("--wav", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--song-name", default="Choom")
    parser.add_argument("--start-time", default=2.5, type=float)
    parser.add_argument("--end-time", type=float)
    args = parser.parse_args()

    raw = {}
    tempos = None
    for role in ("vocal", "drums", "bass", "other"):
        notes, _, role_tempos = parse_midi(getattr(args, role))
        raw[role] = collapse_onsets(notes)
        tempos = role_tempos if tempos is None else tempos
    bpm = 60_000_000 / tempos[0][1]
    acoustic_offset, measured = choose_shared_alignment(raw, args.wav, bpm)
    hook_clusters = detect_hook_clusters(raw)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    with wave.open(str(args.wav), "rb") as stream:
        audio_duration = stream.getnframes() / stream.getframerate()
    chart_end_time = min(args.end_time or audio_duration - 0.5, audio_duration - 0.1)
    summaries = {}
    print(args.song_name, "duration", round(audio_duration, 4), "measured offsets", measured, "shared", round(acoustic_offset, 4), "extra advance", -PLAYTEST_ADVANCE_SECONDS)

    for difficulty, profile in PROFILES.items():
        events, selected_hooks = merge_profile(raw, profile, hook_clusters)
        refine_to_earlier_audio_attacks(events, args.wav, acoustic_offset)
        for event in events:
            event["target_time"] -= PLAYTEST_ADVANCE_SECONDS
        events = [event for event in events if args.start_time <= event["target_time"] <= chart_end_time]
        events = separate_close_events(events, profile.final_gap)
        assign_lanes(events)
        assign_long_note_durations(events, profile)
        output = args.output_dir / f"{args.song_name}_{difficulty}_5Key.csv"
        write_chart(output, events)
        roles = {role: sum(event["role"] == role for event in events) for role in raw}
        lanes = [sum(event["lane"] == lane for event in events) for lane in range(5)]
        late = [sum(start <= event["target_time"] < start + 10 for event in events) for start in range(140, 180, 10)]
        hook_notes = sum(event.get("is_hook", False) for event in events)
        long_notes = sum(event.get("duration_seconds", 0.0) > 0.0 for event in events)
        summaries[difficulty] = {
            "notes": len(events), "lanes": lanes, "hooks": len(selected_hooks),
            "hook_notes": hook_notes, "long_notes": long_notes, "first_time": events[0]["target_time"],
            "last_time": events[-1]["target_time"],
        }
        print(difficulty, "notes", len(events), "long notes", long_notes, "hooks", len(selected_hooks), "hook notes", hook_notes, "roles", roles, "lanes", lanes, "140s+ bins", late, "range", round(events[0]["target_time"], 3), round(events[-1]["target_time"], 3))

    metadata = {
        "song_name": args.song_name,
        "bpm": bpm,
        "audio_duration_seconds": audio_duration,
        "acoustic_offset_seconds": acoustic_offset,
        "listening_calibration_seconds": LISTENING_CALIBRATION_SECONDS,
        "playtest_advance_seconds": PLAYTEST_ADVANCE_SECONDS,
        "music_offset_seconds": acoustic_offset - LISTENING_CALIBRATION_SECONDS - PLAYTEST_ADVANCE_SECONDS,
        "difficulties": summaries,
    }
    (args.output_dir / f"{args.song_name}_metadata.json").write_text(
        json.dumps(metadata, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    report = generate_report(args.output_dir, args.song_name, [args.wav], "master WAV")
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
