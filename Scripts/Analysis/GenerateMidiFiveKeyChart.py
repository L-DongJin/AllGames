"""Align a standard MIDI file to its WAV and generate a playable single-note 5-key chart candidate."""

from __future__ import annotations

import argparse
import csv
import struct
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from AnalyzeRhythmAudio import read_pcm16_mono, spectral_flux


LISTENING_CALIBRATION_SECONDS = 0.1475


@dataclass
class MidiNote:
    tick: int
    time: float
    pitch: int
    velocity: int


def read_vlq(data: bytes, index: int) -> tuple[int, int]:
    value = 0
    while True:
        byte = data[index]
        index += 1
        value = (value << 7) | (byte & 0x7F)
        if byte < 0x80:
            return value, index


def parse_midi(path: Path) -> tuple[list[MidiNote], int, list[tuple[int, int]]]:
    data = path.read_bytes()
    if data[:4] != b"MThd":
        raise ValueError("Not a standard MIDI file")
    _, track_count, ticks_per_quarter = struct.unpack(">HHH", data[8:14])
    position = 8 + struct.unpack(">I", data[4:8])[0]
    raw_notes: list[tuple[int, int, int]] = []
    tempos = [(0, 500_000)]

    for _ in range(track_count):
        if data[position:position + 4] != b"MTrk":
            raise ValueError("Invalid MIDI track header")
        length = struct.unpack(">I", data[position + 4:position + 8])[0]
        track = data[position + 8:position + 8 + length]
        position += 8 + length
        index = tick = 0
        running_status = None
        while index < len(track):
            delta, index = read_vlq(track, index)
            tick += delta
            status = track[index]
            if status < 0x80:
                if running_status is None:
                    raise ValueError("Invalid MIDI running status")
                status = running_status
            else:
                index += 1
                if status < 0xF0:
                    running_status = status

            if status == 0xFF:
                meta_type = track[index]
                index += 1
                size, index = read_vlq(track, index)
                payload = track[index:index + size]
                index += size
                if meta_type == 0x51 and size == 3:
                    tempos.append((tick, int.from_bytes(payload, "big")))
            elif status in (0xF0, 0xF7):
                size, index = read_vlq(track, index)
                index += size
            else:
                message_type = status & 0xF0
                if message_type in (0xC0, 0xD0):
                    values = [track[index]]
                    index += 1
                else:
                    values = [track[index], track[index + 1]]
                    index += 2
                if message_type == 0x90 and values[1] > 0:
                    raw_notes.append((tick, values[0], values[1]))

    tempos = sorted(set(tempos))

    def tick_to_seconds(target_tick: int) -> float:
        seconds = 0.0
        previous_tick = 0
        tempo = 500_000
        for tempo_tick, new_tempo in tempos:
            if tempo_tick > target_tick:
                break
            seconds += (tempo_tick - previous_tick) * tempo / 1_000_000 / ticks_per_quarter
            previous_tick = tempo_tick
            tempo = new_tempo
        return seconds + (target_tick - previous_tick) * tempo / 1_000_000 / ticks_per_quarter

    notes = [MidiNote(tick, tick_to_seconds(tick), pitch, velocity) for tick, pitch, velocity in raw_notes]
    return notes, ticks_per_quarter, tempos


def collapse_onsets(notes: list[MidiNote]) -> list[dict]:
    grouped: dict[int, list[MidiNote]] = defaultdict(list)
    for note in notes:
        grouped[note.tick].append(note)
    events = []
    for tick, chord in sorted(grouped.items()):
        representative = max(chord, key=lambda note: (note.velocity + max(0, note.pitch - 60), note.pitch))
        events.append({
            "midi_time": chord[0].time,
            "pitch": representative.pitch,
            "velocity": max(note.velocity for note in chord),
            "chord_size": len(chord),
            "low_pitch": min(note.pitch for note in chord),
            "high_pitch": max(note.pitch for note in chord),
        })
    return events


def estimate_audio_offset(events: list[dict], wav_path: Path) -> tuple[float, float]:
    samples, sample_rate = read_pcm16_mono(wav_path)
    hop = 512
    envelope = spectral_flux(samples, sample_rate, hop=hop)
    envelope_rate = sample_rate / hop
    envelope = np.maximum.reduce([np.roll(envelope, shift) for shift in range(-2, 3)])

    strengths = np.array([event["velocity"] * (1.0 + 0.12 * (event["chord_size"] - 1)) for event in events])
    threshold = float(np.percentile(strengths, 60.0))
    selected = [(event, strength) for event, strength in zip(events, strengths) if strength >= threshold]
    times = np.array([event["midi_time"] for event, _ in selected])
    weights = np.sqrt(np.array([strength for _, strength in selected]))
    weights /= weights.sum()

    offsets = np.arange(-2.0, 2.0001, 0.001)
    scores = np.zeros_like(offsets)
    for index, offset in enumerate(offsets):
        frames = np.rint((times + offset) * envelope_rate).astype(int)
        valid = (frames >= 0) & (frames < len(envelope))
        scores[index] = float(np.sum(envelope[frames[valid]] * weights[valid]))
    best_index = int(np.argmax(scores))
    return float(offsets[best_index]), float(scores[best_index])


def event_salience(event: dict) -> float:
    score = float(event["velocity"])
    score += min(event["chord_size"] - 1, 3) * 8.0
    if event["low_pitch"] <= 36:
        score += 10.0
    if event["high_pitch"] >= 72:
        score += 7.0
    return score


def select_normal_events(events: list[dict], acoustic_offset: float) -> list[dict]:
    candidates = []
    for event in events:
        target_time = event["midi_time"] + acoustic_offset - LISTENING_CALIBRATION_SECONDS
        if 2.5 <= target_time <= 174.0:
            item = dict(event)
            item["target_time"] = target_time
            item["salience"] = event_salience(event)
            candidates.append(item)

    # Keep the strongest musical event in each 1/8-beat-sized window. Very strong accents may
    # occupy an adjacent sixteenth, while ordinary filler is limited to an accessible Normal density.
    windows: dict[int, list[dict]] = defaultdict(list)
    for event in candidates:
        windows[int(round(event["target_time"] / 0.125))].append(event)
    reduced = [max(group, key=lambda event: event["salience"]) for _, group in sorted(windows.items())]

    selected: list[dict] = []
    for event in reduced:
        if not selected:
            selected.append(event)
            continue
        gap = event["target_time"] - selected[-1]["target_time"]
        required_gap = 0.115 if event["salience"] >= 100.0 else 0.20
        if gap >= required_gap:
            selected.append(event)
        elif event["salience"] > selected[-1]["salience"] + 12.0:
            selected[-1] = event
    return selected


def assign_lanes(events: list[dict]) -> None:
    left_index = right_index = 0
    use_left_hand = True
    for index, event in enumerate(events):
        is_accent = (event["chord_size"] >= 2 and event["velocity"] >= 60) \
            or (index % 8 == 0 and event["velocity"] >= 70)
        if is_accent:
            lane = 2
        else:
            if use_left_hand:
                lane = (0, 1)[left_index % 2]
                left_index += 1
            else:
                lane = (3, 4)[right_index % 2]
                right_index += 1
            use_left_hand = not use_left_hand
        event["lane"] = lane


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--midi", required=True, type=Path)
    parser.add_argument("--wav", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    notes, ticks_per_quarter, tempos = parse_midi(args.midi)
    events = collapse_onsets(notes)
    acoustic_offset, alignment_score = estimate_audio_offset(events, args.wav)
    selected = select_normal_events(events, acoustic_offset)
    assign_lanes(selected)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=[
            "target_time_seconds", "lane_index", "midi_time_seconds", "pitch", "velocity", "chord_size", "salience"
        ])
        writer.writeheader()
        for event in selected:
            writer.writerow({
                "target_time_seconds": f"{event['target_time']:.4f}",
                "lane_index": event["lane"],
                "midi_time_seconds": f"{event['midi_time']:.4f}",
                "pitch": event["pitch"],
                "velocity": event["velocity"],
                "chord_size": event["chord_size"],
                "salience": f"{event['salience']:.2f}",
            })

    lane_counts = [sum(event["lane"] == lane for event in selected) for lane in range(5)]
    bpm_values = sorted({round(60_000_000 / tempo, 4) for _, tempo in tempos})
    print(f"MIDI notes={len(notes)}, onsets={len(events)}, PPQ={ticks_per_quarter}, BPM={bpm_values}")
    print(f"Audio alignment offset={acoustic_offset:+.4f}s, score={alignment_score:.6f}, gameplay calibration=-{LISTENING_CALIBRATION_SECONDS:.4f}s")
    print(f"Normal candidate notes={len(selected)}, range={selected[0]['target_time']:.4f}-{selected[-1]['target_time']:.4f}s, lanes={lane_counts}")
    print(args.output)


if __name__ == "__main__":
    main()
