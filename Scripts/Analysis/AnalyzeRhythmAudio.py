"""Estimate tempo, beat-grid phase, and strong onset candidates from a PCM WAV file.

This intentionally uses only Python's standard library and NumPy so the analysis can
be repeated without installing audio packages. Results are chart-authoring hints and
must be verified by listening; vocals and syncopated phrases still need human edits.
"""

from __future__ import annotations

import argparse
import json
import wave
from pathlib import Path

import numpy as np


def read_pcm16_mono(path: Path) -> tuple[np.ndarray, int]:
    with wave.open(str(path), "rb") as wav:
        if wav.getsampwidth() != 2:
            raise ValueError("Only 16-bit PCM WAV files are currently supported")
        sample_rate = wav.getframerate()
        channels = wav.getnchannels()
        samples = np.frombuffer(wav.readframes(wav.getnframes()), dtype="<i2")
    samples = samples.reshape(-1, channels).astype(np.float32).mean(axis=1)
    return samples / 32768.0, sample_rate


def spectral_flux(samples: np.ndarray, sample_rate: int, frame_size: int = 2048, hop: int = 512) -> np.ndarray:
    window = np.hanning(frame_size).astype(np.float32)
    frequencies = np.fft.rfftfreq(frame_size, 1.0 / sample_rate)
    band = (frequencies >= 45.0) & (frequencies <= 10000.0)
    frame_count = 1 + (len(samples) - frame_size) // hop
    flux = np.zeros(frame_count, dtype=np.float32)
    previous = None
    for start_frame in range(0, frame_count, 256):
        count = min(256, frame_count - start_frame)
        starts = (start_frame + np.arange(count)) * hop
        frames = np.stack([samples[start : start + frame_size] for start in starts])
        spectrum = np.log1p(12.0 * np.abs(np.fft.rfft(frames * window, axis=1))[:, band])
        if previous is None:
            differences = np.diff(spectrum, axis=0, prepend=spectrum[:1])
        else:
            differences = np.diff(spectrum, axis=0, prepend=previous[None, :])
        flux[start_frame : start_frame + count] = np.maximum(differences, 0.0).mean(axis=1)
        previous = spectrum[-1]
    # Remove the slowly changing floor while preserving sharp attacks.
    smooth_width = max(3, round(0.20 * sample_rate / hop))
    floor = np.convolve(flux, np.ones(smooth_width) / smooth_width, mode="same")
    flux = np.maximum(flux - floor, 0.0)
    scale = np.percentile(flux, 99.0)
    return flux / max(float(scale), 1e-6)


def tempo_candidates(envelope: np.ndarray, envelope_rate: float) -> list[dict[str, float]]:
    centered = envelope - envelope.mean()
    min_bpm, max_bpm = 70.0, 190.0
    candidates = []
    for bpm in np.arange(min_bpm, max_bpm + 0.05, 0.05):
        lag = int(round(envelope_rate * 60.0 / bpm))
        if lag <= 0 or lag >= len(centered):
            continue
        base = float(np.dot(centered[:-lag], centered[lag:]) / (len(centered) - lag))
        # Reinforce meters whose half-beat and double-beat structure is also present.
        half_lag = max(1, lag // 2)
        double_lag = lag * 2
        half = float(np.dot(centered[:-half_lag], centered[half_lag:]) / (len(centered) - half_lag))
        double = 0.0
        if double_lag < len(centered):
            double = float(np.dot(centered[:-double_lag], centered[double_lag:]) / (len(centered) - double_lag))
        candidates.append((base + 0.25 * half + 0.35 * double, float(bpm)))
    candidates.sort(reverse=True)
    selected: list[dict[str, float]] = []
    for score, bpm in candidates:
        if all(abs(bpm - item["bpm"]) > 1.5 for item in selected):
            selected.append({"bpm": round(bpm, 2), "score": round(score, 6)})
        if len(selected) == 8:
            break
    return selected


def best_grid_offset(envelope: np.ndarray, envelope_rate: float, bpm: float) -> tuple[float, float]:
    beat_seconds = 60.0 / bpm
    offsets = np.linspace(0.0, beat_seconds, 500, endpoint=False)
    duration = len(envelope) / envelope_rate
    best_offset, best_score = 0.0, -1.0
    for offset in offsets:
        beat_times = np.arange(offset, duration, beat_seconds)
        indices = np.clip(np.rint(beat_times * envelope_rate).astype(int), 0, len(envelope) - 1)
        # Include adjacent frames because an audio attack rarely lands on the exact analysis hop.
        score = float(np.maximum.reduce([
            envelope[np.clip(indices - 1, 0, len(envelope) - 1)],
            envelope[indices],
            envelope[np.clip(indices + 1, 0, len(envelope) - 1)],
        ]).mean())
        if score > best_score:
            best_offset, best_score = float(offset), score
    return best_offset, best_score


def strong_onsets(envelope: np.ndarray, envelope_rate: float) -> list[dict[str, float]]:
    radius = max(1, round(0.075 * envelope_rate))
    threshold = float(np.percentile(envelope, 86.0))
    peaks = []
    for index in range(radius, len(envelope) - radius):
        value = float(envelope[index])
        if value >= threshold and value == float(envelope[index - radius : index + radius + 1].max()):
            peaks.append({"time": round(index / envelope_rate, 4), "strength": round(value, 4)})
    return peaks


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("wav", type=Path)
    args = parser.parse_args()

    samples, sample_rate = read_pcm16_mono(args.wav)
    hop = 512
    envelope = spectral_flux(samples, sample_rate, hop=hop)
    envelope_rate = sample_rate / hop
    tempos = tempo_candidates(envelope, envelope_rate)
    for tempo in tempos:
        offset, phase_score = best_grid_offset(envelope, envelope_rate, tempo["bpm"])
        tempo["phase_offset_seconds"] = round(offset, 5)
        tempo["phase_score"] = round(phase_score, 6)

    result = {
        "file": str(args.wav),
        "duration_seconds": round(len(samples) / sample_rate, 6),
        "sample_rate": sample_rate,
        "tempo_candidates": tempos,
        "strong_onsets": strong_onsets(envelope, envelope_rate),
        "warning": "Automatic candidates require listening verification before chart authoring.",
    }
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
