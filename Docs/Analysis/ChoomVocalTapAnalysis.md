# Choom vocal tap analysis

## Captured sample

- Recording window: 25-40 seconds
- Human taps: 47
- First tap: 25.2084 seconds
- Last tap: 39.9775 seconds
- Median interval: 0.2665 seconds
- Quarter-note-like intervals (0.42-0.58 seconds): 13
- Eighth-note-like intervals (0.17-0.33 seconds): 33

The sample changes from mostly half-second spacing near the beginning to mostly quarter-second spacing after roughly 32 seconds. This provides a useful human-authored timing reference instead of treating every automatically detected attack as a vocal syllable.

## First playback test

`DA_Choom_VocalTapTest` preserves the recorded timestamps without quantization and places every note on Space. This isolates vocal timing from lane-pattern difficulty. The tap recorder is disabled when this asset is activated so replaying the test cannot overwrite the source CSV.

The Data Asset was generated and assigned to `BP_RhythmConductor` successfully with zero commandlet errors or warnings.

The recording identifies perceived rhythmic events, not their isolated audio source. Without stem separation it cannot prove whether each tap followed a vocal consonant, drum transient, melody change, or their combination. The stable half-second-to-quarter-second subdivision change shows that the player followed a coherent perceived pulse/groove rather than random sounds; this musical interpretation is valid chart-authoring input.

If this replay feels aligned with the vocal syllables, the next transformation is lane-pattern authoring and selective quantization of only near-grid taps. If it feels systematically early or late, adjust all 47 timestamps together before changing individual notes.

## Extended perceptual-pulse capture

A second clean recording is configured from 1-80 seconds. It uses an empty chart so existing falling notes cannot prompt the player's timing, records Space taps to `Saved/ChartRecordings/ChoomTapRecording_1_80.csv`, and preserves the earlier 25-40 second CSV. The result will be compared by section against the 120 BPM grid and automatically detected acoustic onsets.

## Extended capture result

- Captured taps: 212
- Active tap range: 4.7214-79.7318 seconds
- Median interval: 0.2832 seconds
- Sixteenth-like short intervals: 10
- Eighth-like intervals: 117
- Quarter-like intervals: 76
- Half-note-like intervals: 6
- Phrase gaps: 2

After applying the previously measured 147.5 ms listening calibration, the taps align strongly to the song's 120 BPM sixteenth-note grid:

- Median distance to a sixteenth-note grid point: **14.8 ms**
- Taps within 30 ms of that grid: **80.2%**
- Median distance to the eighth-note-only grid: 110.1 ms
- Median distance to the quarter-note-only grid: 123.5 ms

This means the player was not simply pressing every quarter beat. The perceived rule was: follow a sixteenth-note subdivision, select salient slots, and skip the rest. Most selected events are separated by an eighth note or quarter note, while off-beat sixteenth positions express vocal/instrument syncopation. This is coherent rhythm-game chart interpretation, not random or unusably inexperienced input.

The capture does not identify the exact source instrument for every event. Its relatively weaker direct match to only the strongest spectral onsets is consistent with following a combination of vocal rhythm, melody articulation, and groove rather than percussion alone.

## Playable 1-80 second draft transformation

The first playable draft uses these rules:

- Snap only taps within 35 ms of the calibrated 120 BPM sixteenth-note grid.
- Preserve farther taps as possible expressive vocal/groove timing.
- Merge events closer than 90 ms after transformation.
- Keep single notes only; no chords are introduced.
- Alternate center-first left (`A/S/D/F`) and right (`J/K/L/;`) hand cycles.
- Use Space after long phrase gaps and for selected slower phrase accents.
- Disable the tap recorder before playback so the source CSV remains unchanged.

Asset: `/Game/Data/DA_Choom_HumanGroove_1_80`

Generation result:

- Input taps: 212
- Final notes: 212
- Snapped within the 35 ms window: 181
- Expressive raw timestamps preserved: 31
- Sub-90 ms events merged: 0
- Git LFS handling verified for the generated Data Asset
