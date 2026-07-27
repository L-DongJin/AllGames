# Rhythm Chart Authoring Pipeline

Last updated: 2026-07-17

## Purpose

This document records how AllGames moved from hand-written test notes to the current reusable, motif-aware multitrack MIDI pipeline. It is the reference for adding songs and for understanding why a generated chart contains a note.

The runtime never parses MIDI. MIDI and WAV files are authoring inputs; the final game reads ordered `FRhythmNoteData` entries from `URhythmSongDataAsset` assets.

### Aligned audio-stem alternative

Songs without usable MIDI can be authored from aligned separated WAV stems. Vocal, drum,
bass, and remaining-music stems must have the same sample rate, start point, and duration.
`GenerateAudioStemDifficultyCharts.py` detects per-stem spectral attacks, merges competing
events by time and strength, assigns readable 5-key lane patterns, and emits four difficulty
CSVs. The separated stems remain external authoring files; Unreal imports only the master mix
for runtime playback.

This method follows the actual rendered sound more directly than MIDI, but source separation
can leak instruments between stems. Generated charts therefore remain a first-pass draft and
require listening tests, especially for vocal sustains and dense Expert passages.

CANON-D uses the established multitrack MIDI path rather than audio-stem onset generation.
Its Vocal/Drums/Bass/Other MIDI sources share 120 BPM and extend to approximately 197 seconds;
the 198.856-second master WAV supplies acoustic alignment. The dense Other transcription is
filtered by the reusable difficulty profiles instead of being copied directly into gameplay.

Drama, LoveAttack, 만찬가, and 갑자기 verify that both production paths can coexist in the same catalog. Drama uses
four aligned Vocal/Drums/Bass/FX WAV stems; `FX` supplies the remaining-music role and only the
existing `/Game/Audio/Music/에스파-Drama` master is used at runtime. LoveAttack follows the same
aligned-stem process. 만찬가 and 갑자기 use separated Vocal/Drums/Bass/Other MIDI plus their master
WAV files for acoustic alignment. Neither the source stems
nor authoring MIDI files need to be imported into Unreal or cooked into the distributed game.

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

Difficulty preserves progressively more of the same detected phrase. After playtesting, the former Normal/Hard/Expert profiles became the new Easy/Normal/Hard baseline; Expert now uses a separate denser profile:

| Difficulty | Motif detail | General intent |
| --- | ---: | --- |
| Easy | up to 4 hits | former Normal; recognizable vocal/groove phrase |
| Normal | up to 6 hits | former Hard; subdivisions and fills |
| Hard | up to 8 hits plus surrounding attacks | former Expert |
| Expert | up to 12 hits with tighter event filters | fastest available articulation and fills |

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

The MIDI parser also pairs Note On and Note Off events. Clearly sustained Vocal/Other notes may become hold notes. The duration threshold is conservative and difficulty-dependent (Easy 1.0 s, Normal 0.8 s, Hard 0.65 s, Expert 0.55 s), durations are capped at 3 seconds, and a hold is shortened or rejected if it would overlap the next note on the same lane.

### Unreal asset creation

Editor Python scripts convert CSV rows to four `URhythmSongDataAsset` assets. Each asset stores the same music but a different `ERhythmDifficulty`, ordered note array, 5-key mode, BPM, measured metadata offset, and base travel time.

`DA_RhythmSongCatalog` contains difficulty groups for each unique music asset. The lobby cycles unique songs, and the selected difficulty resolves the matching chart for that music. As of 2026-07-19 the production catalog contains sixteen songs and sixty-four charts:

- Choom: 488 / 576 / 715 / 815 notes;
- Lemonade: 520 / 605 / 881 / 1104 notes;
- It'sMe: 359 / 426 / 566 / 669 notes.
- CHASE-ME: 583 / 791 / 1146 / 1513 notes;
- CANON-D: 403 / 495 / 927 / 1182 notes;
- Drama: 543 / 864 / 1246 / 1834 notes from aligned audio stems;
- 만찬가: 565 / 655 / 1040 / 1253 notes from multitrack MIDI.
- LoveAttack: 594 / 786 / 1095 / 1457 notes from aligned audio stems;
- 갑자기: 523 / 592 / 857 / 1052 notes from multitrack MIDI.
- HeavySerenade: 471 / 752 / 1063 / 1539 notes from aligned audio stems;
- RUDE!: 524 / 858 / 1177 / 1731 notes from aligned audio stems.
- SHEESH: 455 / 652 / 884 / 1292 notes from aligned audio stems.
- DRIP: 519 / 685 / 1002 / 1402 notes from aligned audio stems.
- BANG BANG: 424 / 489 / 559 / 607 notes from multitrack MIDI.
- 404 (New Era): 444 / 518 / 628 / 684 notes from multitrack MIDI.
- 캐치캐치: 510 / 567 / 679 / 765 notes from multitrack MIDI.

### Automatic quality report

Both production generators now call `GenerateChartQualityReport.py` after writing their four
CSVs. The report compares every note against spectral attacks in the master WAV or combined
aligned stems and writes `<Song>_quality_report.json` plus `<Song>_quality_report.md`.

The report records note density, longest gaps, five-lane balance, long-note overlaps, audio-onset
match percentage, median onset distance, and up to six prioritized five-second listening windows
per difficulty. A window is raised when active audio is under-charted, density is an outlier, or
notes have weak proximity to detected attacks. The intentional countdown/initial-travel region is
excluded from sparse-section warnings.

`GOOD`, `CAUTION`, and `REVIEW` are risk levels, not claims that a chart is fun or release-ready.

## Vocal/drum priority and hold playability

- Production charts now treat vocal articulation and drum attacks as the primary playable rhythm.
- Bass and accompaniment/FX may only fill clear gaps and are capped to a small difficulty-scaled
  share; they no longer create independent hook streams.
- MIDI hooks are derived from the vocal lead. Bass and Other tracks can support a selected attack
  but cannot become a competing phrase by themselves.
- Long notes are created only from sustained vocal evidence. Their duration is capped at 2.25
  seconds, successive holds are separated by difficulty-scaled minimum intervals, and a hold may
  contain at most 0/1/2/3 additional taps on Easy/Normal/Hard/Expert.
- When a clear hold conflicts with excess ornamental taps, the ornamental taps are removed instead
  of forcing the player to hold one key while executing an unreadable stream.
- CANON-D is intentionally exempt from this rebalance because its accompaniment-led pattern is part
  of the accepted chart identity.

## Shared 1-20 chart level

Easy, Normal, Hard, and Expert describe the chart-authoring profile, but they are not assumed to
have equal difficulty across songs. Every production Song Data Asset therefore also stores a
catalog-wide `ChartLevel` from 1 to 20. The automatic baseline combines:

- average notes per second over the playable chart;
- the highest notes-per-second density found in any two-second window;
- long-note occupied time as an additional hand-load factor.

The result is clamped to 1-20 and is shown separately in the lobby. This allows, for example, a
lighter Expert chart to carry a similar numeric level to a denser Normal chart. Manual playtesting
may later adjust an individual level without renaming or regenerating its difficulty profile.
Within one song, adjacent Easy/Normal/Hard/Expert ratings must differ by at least two levels so
the selection communicates a meaningful step even when the raw density formula rounds nearby
charts to the same integer. This ladder normalization changes the displayed rating only; actual
note density remains independently authored and validated.
They reduce full-song manual testing to short exception review; representative release songs still
need at least one complete human playtest.

`GenerateAllChartQualitySummary.py` discovers every per-song JSON report and produces
`Docs/Analysis/AllSongsChartQualitySummary.{json,md}`. It ranks listening priority by worst status,
minimum difficulty match, and suspicious-window count. A `CAUTION` result may come from intentional
lane emphasis as well as timing risk, so the ranked five-second windows and underlying reasons
should be reviewed before changing a chart.

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
- automatic quality reports with zero long-note overlaps and prioritized listening windows.

Automated analysis cannot fully decide which phrase a human remembers most. Every newly added song still needs one PIE listening pass, but the expected human work is exception review—reporting an omitted or awkward region—not manually describing or tapping the whole song.

## Current limitations and future improvements

- AI stem transcription can omit an audible hit or create cross-stem leakage.
- The current motif detector understands onset shape, repetition, strength, and role, but not lyric meaning or musical form labels such as chorus and bridge.
- The 147.5 ms listening calibration and 45 ms advance were accepted on the current device; a future player calibration screen should make device latency user-specific.
- Chords and simultaneous inputs remain excluded. Long notes are supported and generated conservatively from real MIDI duration, but their musical selection still requires PIE listening verification.
- A future editor tool should display generated motifs over a waveform and allow local accept/reject edits without regenerating the whole song.
