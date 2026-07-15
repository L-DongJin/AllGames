# Choom beat analysis

## Purpose

Establish a verified beat grid before authoring vocal and instrument patterns. Automatic analysis is used only to create a listening test; it is not treated as a finished chart.

## Source

- File: `Choom.wav`
- Duration: 176.053708 seconds
- Format: stereo, 48,000 Hz, 16-bit PCM

## Automatic analysis result

- Working tempo: **120 BPM**
- Beat duration: **0.5 seconds**
- Eighth-note duration: **0.25 seconds**
- Automatically detected acoustic grid offset: **0.0133 seconds**
- PIE listening calibration: **-0.1475 seconds**
- Calibrated gameplay grid offset: **-0.1342 seconds**

The strongest detected attacks repeatedly occur close to this grid, for example:

| Approximate time | Detected attack |
|---:|---:|
| 10.0133 | 10.0160 |
| 11.0133 | 11.0080 |
| 12.0133 | 12.0107 |
| 13.0133 | 13.0133 |
| 14.0133 | 14.0160 |
| 15.0133 | 15.0080 |

The whole-song tempo estimator also produced nearby 119.45 and 120.95 BPM candidates because vocals, subdivisions, and section changes affect autocorrelation. The repeating half-second attacks support using exactly 120 BPM as the first listening hypothesis.

## Verification chart

- Asset: `/Game/Data/DA_Choom_BeatSyncTest`
- Active range: approximately 10.0133 to 24.5133 seconds
- Notes: 30 quarter-note beats
- Lane: Space only
- Purpose: judge whether each note reaches the line at the audible drum/attack

The test intentionally avoids random lanes, vocals, eighth notes, and difficulty patterns. If the notes consistently feel early or late, adjust `MusicOffsetSeconds` and all target times by the same amount before authoring the real chart.

The spawner continues broadcasting timeline updates after its final note is spawned. This is required because the last visible notes still have two seconds of travel remaining.

## Next decision after PIE listening

- Consistently early: increase the offset.
- Consistently late: decrease the offset.
- Correct at the beginning but drifting later: re-check BPM rather than offset.
- Drum grid is stable: begin a short vocal phrase chart using syllable-onset timestamps, with drum hits filling non-vocal sections.

## First listening calibration

The first PIE test consistently felt visually late. Judgement logs confirmed that the player was pressing before the chart target, with later stable samples clustered near `-147.5 ms`:

```text
Good: target 20.513, error -147.8 ms
Good: target 22.513, error -147.5 ms
Good: target 24.513, error -147.4 ms
```

This is not addressed by widening the Perfect window. The verification chart is shifted earlier by 147.5 ms so an audible-beat input moves toward the center of the Perfect window. The resulting test offset is `0.0133 - 0.1475 = -0.1342` seconds.

## Verification result

- User completed a second PIE listening test after applying the `-0.1342`-second offset.
- Natural Space inputs on the audible attacks now produce Perfect judgements predominantly.
- The 120 BPM grid and calibrated gameplay offset are accepted for the current Choom test environment.
- Judgement windows were not widened to obtain this result.

This verifies the drum-grid foundation. The next chart-authoring task should use short vocal syllable-onset sections while retaining the verified drum grid for non-vocal passages.
