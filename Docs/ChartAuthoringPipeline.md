# Rhythm Chart Authoring Pipeline

Last updated: 2026-07-16

## Purpose

This document records how AllGames moved from hand-written test notes to the current reusable, motif-aware multitrack MIDI pipeline. It is the reference for adding songs and for understanding why a generated chart contains a note.

The runtime never parses MIDI. MIDI and WAV files are authoring inputs; the final game reads ordered `FRhythmNoteData` entries from `URhythmSongDataAsset` assets.

## Evolution of the chart baseline

### 1. Fixed temporary notes

The first prototype used nine notes written directly in code at one-second intervals. Its only purpose was to verify spawning, travel time, lane layout, input, judgement, score, and UI. It contained no musical interpretation.

### 2. Deterministic random full-song test

After Data Assets were introduced, a fixed-seed random chart replaced the nine notes. It verified full-song lifetime and avoided simultaneous notes and consecutive identical lanes, but it was deliberately not considered a real rhythm chart.

### 3. WAV onset and beat-grid experiments

Spectral-flux analysis extracted strong WAV attacks and compared them with a 120 BPM grid. A short Space-only beat test exposed device/listening latency. Natural player taps showed an approximately 147.5 ms early timing requirement, which became the current listening calibration instead of widening judgement windows.

### 4. Human perceptual tap reference

A temporary recorder captured the player's natural taps from 1-80 seconds. Analysis found that most taps followed selected sixteenth-note subdivisions rather than every drum attack. This established the design rule that a chart should follow perceived vocal/groove emphasis, not blindly convert all detected transients.

### 5. Single-file MIDI candidate

The first supplied `Choom.mid` became the primary musical event source. MIDI supplied exact onset structure, velocity, pitch, chord size, tempo, and PPQ; the WAV supplied acoustic alignment. This was more repeatable than hand taps but could not distinguish vocal, bass, percussion, and accompaniment roles reliably.

### 6. Separated multitrack MIDI

Vocal, Drums, Bass, and Other MIDI stems were introduced. The initial role-aware policy was:

- prefer Vocal for playable syllables and melody;
- treat coincident Drums as support and uncovered Drums as accents;
- use Bass to fill genuine vocal gaps;
- use Other for sparse structural attacks and chords;
- never concatenate every stem onset.

Because AI-separated stem MIDI can leak the same sound across tracks, individual stem alignment estimates are wrapped to the beat period and a shared inlier consensus is used.

### 7. Per-event audible refinement

A single global MIDI/WAV offset was insufficient: some sections matched while others felt late. Each selected event now searches a local WAV window from 140 ms before to 80 ms after its predicted attack. A clearly earlier spectral-flux peak may pull the note earlier; refinement never pushes an already-correct note later.

An additional 45 ms playtest advance was accepted after Expert testing. Current target time is therefore conceptually:

```text
MIDI onset
+ per-song acoustic alignment
+ optional earlier local WAV refinement
- 147.5 ms listening calibration
- 45 ms accepted playtest advance
```

### 8. True difficulty assets

Runtime random thinning was removed. Easy, Normal, Hard, and Expert are now four separately generated and stored charts. Difficulty affects event thresholds, role gaps, phrase detail, and final spacing; it does not change the conductor timeline or artificially punish Hard/Expert with narrower judgement windows.

### 9. Motif-aware generation

The first difficulty reducer still damaged memorable phrases by keeping only a representative isolated attack. Examples included clap pairs, rapid vocal syllables, bass fills, and repeated instrumental attacks.

The current generator detects motifs before ordinary density reduction:

- onset runs whose consecutive gaps are approximately 45-230 ms;
- strong repeated same-pitch pairs;
- vocal, bass, and other-instrument phrases with sufficient velocity and length;
- overlapping detections caused by cross-stem leakage.

Overlapping candidates are grouped and scored by peak velocity, average velocity, phrase length, and musical role. One clear representative phrase owns the local window for Easy through Hard, preventing unrelated leakage from muddying it. Expert retains the motif and may also keep valid surrounding attacks.

Difficulty preserves progressively more of the same detected phrase:

| Difficulty | Motif detail | General intent |
| --- | ---: | --- |
| Easy | up to 2 representative hits | readable outline and major accents |
| Normal | up to 4 hits | recognizable vocal/groove phrase |
| Hard | up to 6 hits | subdivisions and fills |
| Expert | up to 8 hits plus valid surrounding attacks | dense original articulation |

No lyric word, onomatopoeia, or production timestamp is embedded in the generator. Choom's known listening points around 47-48, 53, 73-74, 89-90, 117, and 123 seconds exist only as regression tests proving that general motif rules did not erase those phrases.

## Current reusable workflow

### Inputs

Each song needs:

- one PCM WAV already imported as an Unreal SoundWave;
- `Vocal.mid`;
- `Drums.mid`;
- `Bass.mid`;
- `Other.mid`.

Authoring MIDIs are stored under `SourceAssets/MIDI/<Song>/Stems/`. They are normal Git binary files and are not cooked. Runtime music remains under `Content/Audio/Music/` and uses Git LFS.

### Generation

`Scripts/Analysis/GenerateMultiTrackMidiDifficultyCharts.py`:

1. parses Standard MIDI tempo, PPQ, note-on, pitch, velocity, and chords;
2. collapses simultaneous MIDI notes into onset events;
3. measures a separate shared MIDI/WAV alignment for the song;
4. reads the real WAV duration so no Choom-specific endpoint is reused;
5. detects and scores motifs before density reduction;
6. applies role-aware difficulty profiles;
7. refines selected events toward clearly earlier audible WAV attacks;
8. removes accidental near-collisions and guarantees single-note timestamps;
9. assigns balanced 5-key lanes without requiring simultaneous input;
10. writes four CSV candidates and one metadata JSON containing alignment and counts.

### Unreal asset creation

Editor Python scripts convert CSV rows to four `URhythmSongDataAsset` assets. Each asset stores the same music but a different `ERhythmDifficulty`, ordered note array, 5-key mode, BPM, measured metadata offset, and base travel time.

`DA_RhythmSongCatalog` contains difficulty groups for each unique music asset. The lobby cycles unique songs, and the selected difficulty resolves the matching chart for that music. As of 2026-07-16 the production catalog contains:

- Choom: 322 / 488 / 576 / 715 notes;
- Lemonade: 337 / 520 / 605 / 881 notes;
- It'sMe: 208 / 359 / 426 / 566 notes.

### Verification

Automated checks require:

- four ordered difficulty assets per song;
- strictly increasing, unique target timestamps;
- lane indices in the 0-4 range;
- independently increasing density from Easy to Expert;
- coverage through the audible end of the source MIDI/WAV;
- correct song title, music reference, difficulty, and catalog order;
- Git LFS attributes for all Unreal binary assets;
- known regression hooks for a song when listening references exist.

Automated analysis cannot fully decide which phrase a human remembers most. Every newly added song still needs one PIE listening pass, but the expected human work is exception review—reporting an omitted or awkward region—not manually describing or tapping the whole song.

## Current limitations and future improvements

- AI stem transcription can omit an audible hit or create cross-stem leakage.
- The current motif detector understands onset shape, repetition, strength, and role, but not lyric meaning or musical form labels such as chorus and bridge.
- The 147.5 ms listening calibration and 45 ms advance were accepted on the current device; a future player calibration screen should make device latency user-specific.
- Long notes, chords, and simultaneous inputs are intentionally excluded from the current 5-key charts.
- A future editor tool should display generated motifs over a waveform and allow local accept/reject edits without regenerating the whole song.

