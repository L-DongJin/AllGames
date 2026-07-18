# AllGames Rhythm Game Development Plan

Last updated: 2026-07-19

## Goal

Build a small Unreal Engine 5.7 rhythm-game prototype in which one song can be played from start to finish. The first complete version must support 5-key and 9-key layouts, music-synchronized notes, judgement, score/combo, data-driven charts, and a result screen.

## Current status

- Prototype progress: stages 1-10 complete; stage 11 implemented and awaiting full-song PIE verification.
- AllGames platform progress: the shared MainHub is implemented, and the second mini-game `Idol Quiz` now has a complete single-player prototype awaiting manual PIE verification.
- Idol Quiz content: 83 third-generation idol questions across 13 groups, generated from folder names (group) and image filenames (answer). Each round selects 10 questions without replacement.
- Current stage: sixteen-song, sixty-four-chart catalog with automatic QA reports, awaiting prioritized PIE listening verification for the five newest songs.
- Current online stage: PlayFab account login and score submission are PIE-verified; result/lobby leaderboard presentation is implemented and awaiting PIE verification.
- Next online stage after verification: assign permanent SongIds to production chart assets, confirm maximum-score aggregation, then prepare server-validated submission before public distribution.
- Default map: `/Game/Maps/MainHubMap`; it authenticates once, launches the selected mini-game entry map, and the Rhythm entry continues through `/Game/Maps/LobbyMap` to `/Game/Maps/FiveKeyMap`.
- Preserved 9-key test map: `/Game/Maps/TestMap`.
- Playable catalog: Choom, Lemonade, It'sMe, CHASE-ME, CANON-D, Drama, 만찬가, LoveAttack, 갑자기, HeavySerenade, and RUDE!; every song has Easy/Normal/Hard/Expert 5-key charts.
- Test song format: stereo, 48 kHz, 16-bit PCM WAV, approximately 176.054 seconds.
- Default input mode follows SongData; FiveKeyMap selects 5-key.
- Latest successful full build: `AllGamesEditor Win64 Development` on 2026-07-19.
- Repository scope now includes the lobby, complete 5-key gameplay loop, MIDI authoring tools, and the three-song production catalog.
- Detailed chart-authoring history and the reusable current workflow are documented in `Docs/ChartAuthoringPipeline.md`.
- Next Idol Quiz stage after prototype verification: add timed rounds and server-authoritative multiplayer rooms where the first correct chat answer wins. Do not begin this stage until requested.

## Implemented architecture

### Game framework

- `ARhythmGameModeBase` is the C++ gameplay GameMode base.
- `BP_RhythmGameMode` is the editor-configurable Blueprint child.
- `MainHubMap` is the editor startup and packaged-game default map. `LobbyTestMap` bypasses login for editor-only rhythm iteration, while `TestMap` remains the 9-key development map.
- FiveKeyMap is based on TestMap and gives its single Conductor a per-map 5-key SongData override.

### Input

- `ARhythmPlayerController` owns Enhanced Input setup.
- `IA_Lane1` through `IA_Lane9` are Boolean Input Actions.
- `OnLaneInput(int32 LaneIndex, bool bPressed)` broadcasts zero-based lane input for later judgement logic.
- `ERhythmKeyMode` supports `FiveKey` and `NineKey`.
- `SetKeyMode()` changes the active mode; `GetActiveLaneCount()` returns 5 or 9.
- 5-key mapping context: `IMC_Rhythm_5Key`.
  - Lane 1-5: `D`, `F`, `Space`, `J`, `K`.
- 9-key mapping context: `IMC_Rhythm_9Key`.
  - Lane 1-9: `A`, `S`, `D`, `F`, `Space`, `J`, `K`, `L`, `Semicolon`.
- The legacy `IMC_Rhythm` asset is no longer used and is retained temporarily until the replacement layouts are fully user-verified.
- The engine's `Semicolon -> ToggleDebugCamera` debug binding is removed in `DefaultInput.ini` so lane 9 does not open the debug camera help.

### Music and timing

- `ARhythmConductor` owns the `UAudioComponent`, selected music, automatic playback, and stopping.
- `BP_RhythmConductor` is placed once per gameplay map and references `Choom`; FiveKeyMap overrides SongData at actor-instance level.
- Public timing API:
  - `GetMusicTimeSeconds()`
  - `GetMusicDurationSeconds()`
  - `GetMusicPlaybackProgress()`
  - `IsMusicPlaying()`
- Music time is derived from `OnAudioPlaybackPercent` and SoundWave duration, not accumulated frame time.
- `PlayMusic()` resets time to zero. Manual stop resets it to zero. Natural completion sets it to song duration.
- Natural completion broadcasts `OnMusicFinished`, which the gameplay UI uses to present results.

### Song and chart data

- `FRhythmNoteData` stores a zero-based lane index and target music time.
- `ARhythmNoteActor` is the runtime visual note and exposes its lane and target time.
- `ARhythmNoteSpawner` reads `ARhythmConductor::GetMusicTimeSeconds()` and spawns notes at a configurable lead time.
- `BP_RhythmNote` and `BP_RhythmNoteSpawner` provide editor-tunable Blueprint children.
- TestMap contains exactly one RhythmNoteSpawner.
- `URhythmSongDataAsset` stores title, music, BPM, timing offset, 5/9-key mode, difficulty, note travel time, and note array.
- `/Game/Data/DA_Choom_5Key_Full` contains 456 single notes from 4.7408-173.6158 seconds, combining verified human-groove timing with strength-filtered WAV onsets.
- FiveKeyMap supplies that asset through its Conductor instance; TestMap retains the separate 9-key development setup.
- Choom uses the verified 120 BPM grid and a -0.1342-second gameplay offset on the current device.

### MIDI-first chart authoring

- Future charts use a supplied standard MIDI file as the primary musical source. WAV analysis remains a fallback and a tool for aligning MIDI events to the audible recording.
- MIDI import must read tempo events, ticks-per-quarter, note-on/off events, velocity, pitch, chords, and track/channel metadata rather than treating every note as an identical gameplay note.
- Each MIDI is aligned against its actual WAV with a per-song offset before generating gameplay timestamps; MIDI file duration alone is not sufficient alignment evidence.
- Difficulty generation selects musical events from the same aligned source: Easy keeps structural accents, Normal follows the main groove/melody, and Hard/Expert progressively include subdivisions, ornaments, and selected chords.
- When separated stem MIDIs are available, Normal charts prioritize reduced Vocal events, use coincident Drums as accents, add standalone Drums at uncovered attacks, and use Bass/Other only to fill genuine vocal gaps or mark structure. Stem events are never concatenated blindly.
- Before ordinary density reduction, the generator detects short repeated-hit and dense-onset motifs in Vocal/Bass/Other stems. Overlapping stem detections are merged and the clearest representative phrase is protected, so hooks such as vocal syllable runs, fills, claps, and repeated attacks are not reduced to a single isolated note.
- Motif detail scales by difficulty: Easy keeps a readable two-hit outline, Normal up to four representative hits, Hard up to six, and Expert up to eight while retaining valid surrounding attacks. Song-specific timestamps are not embedded in generation logic; known listening points are used only by validation tests.
- Generated chart candidates go to separate Song Data Assets and never overwrite the last verified playable chart until a PIE listening test is accepted.
- The original MIDI remains source material, while runtime gameplay continues to consume `URhythmSongDataAsset`; the Conductor and judgement systems do not depend directly on MIDI parsing.
- Authoring MIDI files live under `SourceAssets/MIDI/`, outside Unreal `Content/`, so they are versioned but never cooked. Small `.mid` files use normal Git binary handling rather than Git LFS.
- The multitrack difficulty generator is song-agnostic: it reads each WAV duration, measures a separate acoustic alignment, accepts a song name, writes four named CSVs plus alignment metadata, and does not reuse Choom's end time or MIDI/WAV offset.

### Gameplay UI and note movement

- `URhythmGameplayWidget` is the C++ presentation base and `WBP_RhythmGameplay` is its editable Widget Blueprint child.
- `ARhythmPlayerController` creates the gameplay WBP for the local player.
- `ARhythmNoteSpawner::OnNoteSpawned` sends note data to the gameplay widget; world-space cube notes are disabled by default and remain an optional debug view.
- The widget creates 5 or 9 lanes from the active key mode, draws the judgement line, and creates a UI note per spawn event.
- UI note position is recalculated every frame from conductor music time, spawn time, and target time. Frame `DeltaTime` is not accumulated for movement.
- `WBP_RhythmGameplay` exposes four optional texture slots under `Rhythm|Appearance`: Background Image, Lane Background Image, Note Image, and Judgement Line Image.
- Judgement-line thickness, judgement-feedback vertical position, and judgement-feedback maximum size are editable under the widget's appearance settings for resolution-specific tuning; judgement art preserves its source aspect ratio.
- An optional Lane Glow texture can be assigned in WBP Class Defaults. Its per-lane overlay is shown while the corresponding input is held in either 5-key or 9-key mode.
- When textures are unset, built-in colors provide a complete functional test layout.

### Scoring

- `ARhythmScoreManager` subscribes to judgement events and owns score, current/max combo, judgement counts, and accuracy.
- Initial score weights are Perfect 1000, Great 700, Good 400, and Miss 0.
- Perfect/Great/Good increase combo; Miss resets it. Accuracy is earned judgement points divided by the maximum possible points so far.
- The gameplay widget displays score, combo, and accuracy but does not calculate them.
- At natural song completion it displays score, maximum combo, total notes, accuracy, and all four judgement counts.
- Result judgement counts use distinct colors: Perfect cyan, Great yellow, Good green, and Miss red.

### Lobby and play settings

- `LobbyMap` uses its own lightweight lobby GameMode/PlayerController and contains no gameplay Conductor or NoteSpawner.
- The selected song plays a looping lobby preview. Each Song Data Asset exposes preview start time, duration, and volume; a negative start time automatically selects a centered section of the track.
- Changing songs immediately switches the preview, while changing difficulty or note speed keeps the current preview playing.
- `URhythmGameInstance` preserves difficulty and visual note-speed settings across map travel.
- `/Game/Data/DA_RhythmSongCatalog` is the editor-managed source for lobby song selection. Adding a future Song Data Asset to this catalog makes it selectable without changing lobby code.
- Note speed changes only travel duration; target music timestamps and judgement timing remain unchanged.
- Difficulty selection resolves a separately generated Song Data Asset for the selected music: Choom currently has Easy 239, Normal 380, Hard 496, and Expert 671 notes.
- Runtime note thinning is disabled. Normal/Hard/Expert share the same judgement windows so chart density and pattern complexity define difficulty; Easy alone keeps a modest 15% wider timing window.
- The catalog groups charts by their shared Music asset. SONG cycles unique music tracks, while DIFFICULTY selects the matching chart within that song group.

## Stage roadmap

1. **Test map and GameMode — complete**
   - Created TestMap, C++/Blueprint GameMode, minimal actors, and default-map configuration.
2. **Enhanced Input — complete**
   - Implemented lane input and later expanded it into selectable 5-key and 9-key layouts.
3. **Play one song — complete**
   - Imported Choom and implemented automatic playback through RhythmConductor.
4. **Read music time — complete**
   - Added the audio-derived playback timeline and Blueprint-accessible queries.
5. **Create temporary notes — complete**
   - Add a temporary note data structure, several hard-coded notes, a note actor, and time-aware spawning.
   - Success means notes appear in the correct lanes before their target times; no judgement yet.
6. **Move notes to the judgement line**
   - Derive position from music time, target time, and travel duration.
   - A frame hitch must not permanently desynchronize a note from the song.
	- Complete. User verified that the gameplay WBP, lanes, textures, and music-time-derived note movement render correctly in PIE.
7. **Compare input time with note time**
	- Find the closest eligible note in the pressed lane.
	- Evaluate four tiers: Perfect, Great, Good, and Miss.
	- Prevent duplicate judgement and auto-Miss overdue notes.
	- Complete. Four timing tiers, closest-note selection, automatic Miss, and duplicate prevention were exercised in PIE.
8. **Display judgement feedback**
	- Add gameplay UMG feedback while keeping judgement calculations outside the UI.
	- Complete. Four supplied judgement textures display in PIE and judged notes are removed; subsequent layout polish remains part of normal UI iteration.
9. **Calculate score and combo**
	- Track score, combo, maximum combo, judgement counts, and accuracy.
	- Complete. User requested progression after score/combo implementation and UI integration.
10. **Move song and chart data to Data Assets**
	- Store music, BPM, offset, key mode, difficulty, and note array without requiring C++ edits per song.
	- Complete and verified through multiple Data Asset chart workflows.
11. **Finish song and show results**
    - Stop gameplay, finalize remaining notes, show results, and provide retry/exit actions.
	- Music-finished event and result presentation implemented; full-song PIE verification remains.

## Prototype completion criteria

- A user can launch FiveKeyMap and play one full song.
- A song/chart can select either the 5-key or 9-key layout.
- Notes remain synchronized to the music timeline.
- Inputs produce deterministic judgement results.
- Score, combo, accuracy, and results are displayed.
- Adding a new chart does not require editing gameplay C++.

## Later backlog

- Additional cataloged songs and independently verified difficulty sets.
- Chart-authoring workflow or editor tool.
- Additional judgement-window tuning and per-player calibration.
- Long notes.
- User-remappable controls.
- Audio/input latency calibration offset.
- Visual effects, note skins, and presentation polish.
- Save data and packaged-build verification.

## Known checks and issues

- Restart Unreal Editor after reflected C++ or input config changes; stale Live Coding state previously caused the old 4-key mapping to appear active.
- After restarting, verify that 9-key mode logs lanes 1-9 for `A/S/D/F/Space/J/K/L/Semicolon`.
- Verify that Semicolon triggers lane 9 without opening Debug Camera help after the `DefaultInput.ini` change.
- The playback-percent timing callback is appropriate for the current Windows PC target. Device latency calibration remains a later task.
- `BUG-002` tracks CHASE-ME's initial 49-second timeline jump. The first audio-percent callback could report a stale non-zero position while audible playback started at zero, advancing the HUD/chart and causing an apparent 3:07 cutoff; the fix rejects implausible initial callbacks and awaits manual PIE verification.

## Work log

### 2026-07-16

- Began the complete 5-key song stage using a dedicated `/Game/Maps/FiveKeyMap` so the existing 9-key TestMap remains independent. The new map is the editor/game startup target and will use a per-map Conductor SongData override.
- Generated 456 full-song timing candidates for Choom: 212 verified human-groove notes through 80 seconds and 244 strength-filtered WAV-onset notes through 173.6158 seconds, with no simultaneous timestamps.
- Added a conductor music-finished event and runtime result overlay showing final score, maximum combo, accuracy, and Perfect/Great/Good/Miss counts.
- Added result-screen keyboard actions: Enter reloads the current 5-key map for retry and Escape exits/stops the game.
- Final automated validation passed: 456 ordered notes, no duplicate timestamps, lanes distributed `[98, 99, 62, 99, 98]`, and FiveKeyMap's sole Conductor points to the 5-key full-song asset. The final Editor target build also succeeded.
- Investigated a 5-key PIE input mismatch where D/F/Space behaved like 9-key lanes 3/4/5 and J/K fell outside the five visible lanes. The saved `IMC_Rhythm_5Key` asset is correct (`D/F/Space/J/K -> IA_Lane1-5`); input-mode application now explicitly removes both rhythm contexts before adding the selected one and logs the exact active context path for verification after restart.
- Added a dedicated empty `LobbyMap`, lobby-only GameMode/PlayerController, persistent play settings GameInstance, and a functional keyboard/mouse lobby UI. Difficulty and note-speed selections travel into FiveKeyMap; result Escape now returns to the lobby while Enter retries.
- Split the result judgement totals into separately colored lines: Perfect cyan, Great yellow, Good green, and Miss red.
- Added provisional Easy/Normal/Hard/Expert profiles and 0.75x-2.00x visual note speed. Note speed changes spawn lead/travel time only and never modifies the conductor timeline or note targets.
- Rebuilt `AllGamesEditor` successfully and passed automated LobbyMap validation for startup configuration, lobby GameMode, and absence of gameplay audio/note actors. Manual lobby-to-full-song PIE verification remains pending.
- Fixed gameplay input after lobby travel by explicitly restoring Game Only input mode, hiding the cursor, and flushing lobby key state in `ARhythmPlayerController::BeginPlay()`.
- Added the SONG selector and `/Game/Data/DA_RhythmSongCatalog`. The catalog initially contains `DA_Choom_5Key_Full`; future Song Data Assets can be added to the catalog in the editor and the shared Conductor will use the selected entry after map travel.
- Adopted a MIDI-first chart-authoring policy after inspecting `Choom.mid`: valid Format 1, 480 PPQ, constant 120 BPM, 777 notes, and approximately 174.82 seconds. MIDI will drive future musical event selection, with the WAV used for audible alignment and final verification; the existing generated Choom chart remains intact until a MIDI-derived replacement passes PIE testing.
- Archived `Choom.mid` under `SourceAssets/MIDI/` with a verified matching SHA-256 and added a reusable standard-library MIDI parser/WAV alignment generator. Cross-correlation found a stable acoustic alignment near -0.0460 seconds; the existing -0.1475-second listening calibration is then applied to gameplay targets.
- Generated `DA_Choom_MIDI_Normal_5Key` as a non-destructive comparison chart: 412 single notes from 2.5940-173.9638 seconds with lane counts `[96, 95, 30, 96, 95]`. The lobby catalog now contains both the previous full chart and `Choom - MIDI Normal` for A/B listening tests.
- Made song selection independent of actor BeginPlay order: the PlayerController and NoteSpawner resolve the selected catalog Song Data directly from the GameInstance, while the Conductor selects the matching music/chart source.
- Archived the separated `Choom` Vocal/Drums/Bass/Other MIDI sources under `SourceAssets/MIDI/Choom/Stems/`. Their raw onset counts are 654/88/385/600 respectively; the files remain authoring inputs outside Unreal Content.
- Added a role-aware multitrack generator. Because sparse/periodic tracks produced beat-shifted cross-correlation peaks, it wraps individual estimates to the 120 BPM beat period and uses the stable consensus alignment (`-0.0710s` acoustic, followed by the existing `-0.1475s` listening calibration).
- Generated and cataloged `DA_Choom_MultiTrack_Normal_5Key` without deleting either earlier chart: 379 notes from 2.5919-173.9732 seconds, roles `{vocal: 312, drums: 37, bass: 27, other: 3}`, lanes `[93, 79, 37, 79, 91]`, and no simultaneous timestamps. The lobby now exposes all three versions for A/B testing.
- Listening feedback found the first multitrack candidate variably late: some sections matched, while natural hits elsewhere landed around Good through Miss. This rules out using only one additional global offset because it would damage already-correct sections.
- Added per-event WAV attack refinement around each globally aligned MIDI onset. It searches -140ms/+80ms, prefers the nearest strong spectral-flux peak, and only moves notes earlier by at least 15ms; it never pushes an already-correct note later.
- Generated `DA_Choom_MultiTrack_Refined_Normal_5Key` as a fourth preserved comparison chart. It contains 374 notes, locally advances 169 retained events, removes five collisions created by refinement, and has lane counts `[93, 79, 32, 79, 91]`. Shifted vocal events have a median adjustment of about -70.8ms.
- Replaced the provisional difficulty modifier with four production candidate Data Assets derived from the separated MIDI sources: Easy 239, Normal 380, Hard 496, and Expert 671 notes. The older comparison Data Assets remain on disk but are no longer the lobby's production entries.
- Removed runtime 60% note filtering and Hard/Expert judgement-window penalties. The selected music plus difficulty now resolves the matching Data Asset from the catalog; SONG selection groups entries by their shared Music asset for future multi-song support.
- Responded to late Expert feedback with an additional global -45ms playtest advance on top of per-event audio refinement. Expert density after 140 seconds increased to 133 notes (10-second bins `[46, 38, 31, 18]`) instead of the prior sparse 75-note tail.
- Final generated lane counts are Easy `[58, 57, 11, 57, 56]`, Normal `[86, 86, 36, 86, 86]`, Hard `[115, 114, 40, 114, 113]`, and Expert `[157, 157, 45, 156, 156]`; all charts contain sorted unique single-note timestamps.
- Reworked multitrack difficulty generation around recognizable rhythmic motifs rather than isolated onset strength. The detector now finds close onset runs and repeated-note pairs before density reduction, merges cross-stem leakage into one representative phrase, and preserves a difficulty-dependent number of hits.
- Regenerated the four production Choom charts as Easy 322, Normal 488, Hard 576, and Expert 715 notes with lane counts `[76,75,21,75,75]`, `[113,112,39,112,112]`, `[133,133,46,133,131]`, and `[166,165,54,166,164]`. Expert retains 147 notes from 140 seconds onward.
- Added regression checks for the six listening-reference regions around 47-48, 53, 73-74, 89-90, 117, and 123 seconds. These timestamps exist only in the validator; the reusable generator discovers motifs from MIDI structure without song-specific rules.
- Recreated all four Data Assets and passed the Unreal commandlet validation with zero errors and warnings, including ordered unique times, valid lanes, difficulty metadata, motif coverage, LobbyMap setup, and the production catalog.
- Added Lemonade and It'sMe as complete four-difficulty 5-key songs. Their Vocal/Drums/Bass/Other source MIDIs are archived under `SourceAssets/MIDI/Lemonade/Stems` and `SourceAssets/MIDI/ItsMe/Stems`; the original WAVs remain represented by the existing Unreal music assets.
- Generalized the motif-aware generator to derive each song's WAV duration and shared MIDI/audio alignment and to emit per-song metadata. Lemonade measured a +0.1760-second acoustic alignment and generates 337/520/605/881 notes through 185.6 seconds; It'sMe measured -0.0780 seconds and generates 208/359/426/566 notes through 136.4 seconds.
- Created `DA_Lemonade_{Easy,Normal,Hard,Expert}_5Key` and `DA_ItsMe_{Easy,Normal,Hard,Expert}_5Key`, then expanded `DA_RhythmSongCatalog` to 12 charts grouped as Choom, Lemonade, and It'sMe. Lobby song selection can now cycle the three unique music assets while difficulty resolves the matching chart.
- Extended automated lobby validation to all 12 assets, exact note/lane counts, ordered unique times, increasing difficulty density, full-song tail coverage, Choom hook regressions, and map setup. The Unreal commandlet completed with zero errors and warnings.
- Added long-note gameplay support. `FRhythmNoteData` now stores duration; tap notes remain backward-compatible at zero. The judgement manager tracks head timing, held state, early release, tail completion, and an editable release grace, while scoring receives one final Perfect/Great/Good/Miss result per hold.
- Added runtime-generated long-note Head/Body/Tail presentation, held-key glow, complete/break effects, and Blueprint-assignable texture slots. Functional colored fallbacks remain available until final UI textures are imported.
- Extended MIDI parsing to pair Note On/Off events and preserve source duration. The difficulty generator now converts only clearly sustained Vocal/Other events into non-overlapping holds, using thresholds of 1.0/0.8/0.65/0.55 seconds for Easy/Normal/Hard/Expert and a three-second cap.
- Regenerated all 12 production charts with conservative hold counts: Choom `[1,1,1,4]`, Lemonade `[1,1,4,8]`, and It'sMe `[0,1,1,2]`. Full `AllGamesEditor` builds succeeded after the reflected data and UI changes; automated asset and runtime PIE verification remain.
- Rebased difficulty after playtesting: the former Normal is now Easy, former Hard is Normal, and former Expert is Hard. A new Expert profile uses tighter onset gaps, lower velocity thresholds, more accompaniment fills, and up to 12 preserved motif hits while retaining single-note timestamps.
- New chart totals are Choom `[488,576,715,815]`, Lemonade `[520,605,881,1104]`, and It'sMe `[359,426,566,669]` for Easy/Normal/Hard/Expert. Corresponding hold counts are `[1,1,4,9]`, `[1,4,8,13]`, and `[1,1,2,5]`.
- Replaced fractional note-speed selection with exactly four looping visual speeds: `1x`, `2x`, `3x`, and `4x`. Speed changes travel/spawn lead time only and never changes music or target timestamps.
- Simplified result navigation to mouse-only interaction. Song completion enables the cursor and UI-only input, removes Enter retry and Escape navigation, and displays one `LOBBY` button that returns to LobbyMap.
- Added song-selection preview playback to the lobby. Song Data Assets can select a highlight with `PreviewStartTimeSeconds`, `PreviewDurationSeconds`, and `PreviewVolume`; unconfigured songs automatically preview a 15-second middle section and loop it until selection changes or gameplay starts.
- Added an audio-stem chart-authoring alternative for songs without MIDI. CHASE-ME uses four exactly aligned 187.5012-second WAV stems (vocals, drums, bass, and remaining music); spectral-onset analysis produces Easy/Normal/Hard/Expert drafts while runtime uses only the imported CHASE-ME master SoundWave.
- Created and cataloged `DA_CHASEME_{Easy,Normal,Hard,Expert}_5Key` with 583/791/1146/1513 notes after initial-travel correction. The separated WAV files stay outside Content as authoring sources, preventing four redundant full-length runtime audio imports.
- Added a three-second gameplay preparation countdown. The Conductor delays the authoritative audio start while the gameplay UI displays `3`, `2`, `1`, and `GO!`; note spawning and judgement remain inactive until audio playback begins.
- Investigated a reported late-song CHASE-ME note gap. The runtime SoundWave and source stems both last 187.501 seconds, and generated charts retain notes through 186-187 seconds with no empty late section. Song completion now logs spawned/total note counts and reports an error if playback finishes before every chart note is spawned, allowing a future PIE reproduction to distinguish spawning from UI presentation.
- Added the current play time as `TIME  m:ss` beneath gameplay accuracy so late-song visual issues can be reported against an exact music timestamp. The display remains at `0:00` during the preparation countdown and follows the authoritative music timeline after `GO!`. CHASE-ME Hard analysis confirms continuous late-chart density (roughly 29-36 notes per five seconds after 90 seconds) and no data gap longer than 0.5 seconds before the ending tail.
- Corrected CHASE-ME's abrupt first-note appearance. Audio-stem charts now discard candidates before 2.25 seconds so every first note has at least the two-second base travel time to enter from the lane top after `GO!`; Hard and Expert no longer create partially progressed notes in the middle of the screen.
- Corrected the earlier CHASE-ME 3:07 diagnosis: the four supplied stems and Unreal master are all 187.501 seconds, so there is no source-length mismatch. Opened `BUG-002-ChaseMeEndOfSongCutoff.md`; the final Hard note is at 186.9787 seconds, just before the integer HUD first displays 3:07, leaving only a 0.522-second audio tail. Restored the duration-clamped Conductor timeline and retained the issue for manual end-of-song presentation verification.
- User identified the decisive `BUG-002` reproduction: audible CHASE-ME starts at the beginning while the gameplay clock starts around 0:49. The Conductor had accepted an invalid first `OnAudioPlaybackPercent` value as absolute time, explaining the mid-screen opening notes and why the chart ended with roughly 49 seconds of audio remaining. Added a playback-start clock and reject any initial callback more than 0.5 seconds from plausible elapsed time.
- Moved countdown ownership to the visible gameplay flow: the Conductor now waits after map BeginPlay, and `URhythmGameplayWidget::NativeConstruct()` starts the 3-second countdown only after the lane/note screen has been created and added to the viewport.
- Deferred countdown start by one game-thread tick after `WBP_RhythmGameplay` enters the viewport, ensuring Slate paints the lane/note UI before `3, 2, 1, GO!` begins instead of making the countdown appear to belong to the preceding screen.
- Rebalanced the production chart pipeline around vocal and drum attacks. Bass/Other/FX events now fill only clear gaps with difficulty-scaled caps, and only vocal phrases can become MIDI hooks. Long notes are capped at 2.25 seconds, spaced apart, and permit at most 0/1/2/3 extra taps on Easy/Normal/Hard/Expert; conflicting ornamental taps are removed. Regenerated and updated all production charts except the intentionally preserved CANON-D set.
- Added a catalog-wide `ChartLevel` rating from 1-24 to every Song Data Asset. The shared formula uses average notes per second, peak two-second density, and long-note occupancy, so Easy/Normal/Hard/Expert remain authoring categories while players can compare actual difficulty across different songs.
- Added per-song `TitleImage` artwork to the original eleven Song Data Assets. The lobby normalizes artwork into one 720x250 area at 72% opacity and crossfades/scales between the old and new image over 0.28 seconds when song selection changes. The five songs added on 2026-07-19 currently await matching `/Game/Title` artwork and intentionally retain an empty `TitleImage` field.
- Expanded the lobby song-art area to 720x480 and reflowed difficulty, shared level, speed, and start controls below it. Scroll-speed selection now cycles through `1x`, `2x`, `2.5x`, `3x`, `3.5x`, and `4x`; it continues to affect visual travel time only.
- Added `/Game/UI/WBP_RhythmLobby` as the editor-facing child of `URhythmLobbyWidget`. Its Class Defaults expose `Lobby Background Image` and `Lobby Background Tint` under `Rhythm|Appearance`; the lobby controller loads this WBP with a C++ fallback, while layout and behavior remain code-driven.
- Exposed `Song Image Width` and `Song Image Height` in `WBP_RhythmLobby` Class Defaults under `Rhythm|Appearance`. Both the current and outgoing crossfade images use the same editor-tuned dimensions, with the existing 720x480 layout retained as the default.
- Added an in-game Escape pause menu with mouse-driven `RESTART` and `LOBBY` actions. Pausing explicitly freezes both the audio component and the Conductor's music-derived clock; Escape resumes gameplay, restart reloads the active gameplay map, and lobby returns to `LobbyMap`.
- Added a result-screen count-up animation: score, max combo, accuracy, total notes, and all four judgement counts rapidly ease from zero to their exact final values over an editor-tunable duration. The lobby button appears after the count completes so the result reveal reads as one game-like sequence.
- Began the persistent-account and online-ranking stage by installing the official PlayFab Unreal Marketplace Plugin `1.190.260619` for Unreal Engine 5.7 as a source-controlled project plugin. Enabled `PlayFab`, `PlayFabCpp`, and `PlayFabCommon` for the AllGames runtime module; Title ID and account APIs remain intentionally unconfigured until the PlayFab Title connection step.
- Connected the AllGames client to PlayFab Title ID `1BD611` through `DefaultEngine.ini`. The public Title ID is packaged with the client as intended, while `DeveloperSecretKey` remains explicitly empty and must never be added to the game project.
- Added the first persistent-account implementation. `URhythmAccountSubsystem` owns PlayFab username/password registration and login across map travel without storing passwords; the LobbyMap controller now gates the song lobby behind a minimal ID/password screen and opens the existing lobby only after successful authentication. Registration uses the ID as the initial display name and does not require email yet.
- Localized the entire account entry flow to Korean and separated registration into its own full-screen panel. The registration panel explains cross-PC use, requests ID/password/password confirmation, validates matching passwords before PlayFab submission, warns about the current lack of email recovery, and provides a clear return path to the login panel.
- Simplified registration to a single centered `가입` button with no manual back button. Successful registration now returns to the login panel, pre-fills the new ID, and requires an explicit login; all account text-entry foreground colors are black for readability. Made `URhythmLoginWidget` focusable to remove the PIE `does not support focus` warning.
- Added the first online leaderboard service layer. Song charts now expose a stable `SongId` and `ChartVersion`; `URhythmLeaderboardSubsystem` generates independent PlayFab statistic keys for song/key-mode/difficulty/version, submits completed-song scores, and can retrieve both the global top list and the logged-in player's surrounding ranks. Existing assets without an assigned SongId temporarily fall back to a sanitized song title so PIE integration can be tested before bulk Data Asset editing.
- Completed scores are submitted once from the song-finished flow. PlayFab's signed 32-bit statistic range, active login session, null charts, duplicate in-flight requests, result conversion, one-based ranks, and network failures are handled in the subsystem; leaderboard UI and permanent SongId assignment remain the next ranking-stage work.
- User verified a successful live score write for `RUDE_5K_HARD_V1`. Connected the result screen to show submission progress followed by the logged-in player's online rank and personal best, and added a selection-aware online top-10 panel to the lobby. Stale asynchronous responses are discarded and the latest song/difficulty is retried after an in-flight request completes.
- Exposed the lobby leaderboard heading and player-entry presentation through `WBP_RhythmLobby` Class Defaults. Designers can independently tune the normalized title/entry positions, shared area size and font asset, separate title/entry font sizes, and colors without changing the C++ query logic.
- Fixed a countdown-only upper-left UI cluster. Lane backgrounds, key labels, the judgement line, and score HUD previously waited for the first music-timeline event before receiving Canvas positions, leaving them visible at `(0,0)` during the three-second countdown. Position-dependent widgets now remain collapsed until valid geometry is available, are laid out from the Conductor time during countdown, and then become visible together.
- Added the first Windows Development packaging profile. The UI-driven project now targets DX11/SM5 and disables unused Ray Tracing, Lumen GI/reflections, Virtual Shadow Maps, Mesh Distance Fields, and Substrate for lower shader/VRAM cost and improved laptop stability. Packaging uses compressed IoStore/Pak output, excludes editor/movie/crash-reporter content, and cooks from LobbyMap and FiveKeyMap so unreferenced TestMap/development assets are omitted.
- The first Game-target build exposed an editor-only `SetActorLabel()` call in `ARhythmNoteActor`; guarded the Outliner debug label with `WITH_EDITOR`. A subsequent BuildCookRun completed Build, Cook, Stage, Pak/IoStore, prerequisites, and Archive successfully to `Builds/WindowsDevelopment`.
- The first packaged archive contains 49 files and approximately 1.16 GB: 204.36 MB compressed game content plus large Development-only executable/symbol files (approximately 293 MB EXE and 460 MB PDB). A 12-second packaged smoke test initialized DX11, XAudio2, PlayFab, LobbyMap, and the login UI without a crash; engine initialization completed in approximately 2.9 seconds. Full interactive packaged login, lobby, gameplay, music, and leaderboard verification remains pending.
- Packaged interactive testing found a black FiveKeyMap after START. Runtime logs proved both `/Game/UI/WBP_RhythmLobby` and `/Game/UI/WBP_RhythmGameplay` were omitted because their native `TSoftClassPtr` paths are not hard cook dependencies; the lobby silently used its C++ fallback while gameplay had no widget and therefore no countdown. Added `/Game/UI` to `DirectoriesToAlwaysCook` so both Blueprint presentation classes and their dependencies are staged while keeping the rest of the cook reference-driven.
- Rebuilt the Windows Development archive successfully after the cook fix. Package count increased from 526 to 539 and compressed content from 204.36 MB to 215.99 MB; cooked output now explicitly contains both `WBP_RhythmLobby` and `WBP_RhythmGameplay`. The rebuilt executable remained alive through a 10-second startup smoke test. Manual START/countdown/gameplay verification is required before marking the packaged loop complete.
- Packaged gameplay input reached the judgement manager, but the presentation did not react because the gameplay widget could initialize before the runtime judgement and score managers. The widget now retries those bindings during tick and uses unique delegates once the managers appear, restoring judged-note removal, judgement feedback, and score display in packaged builds.
- Removed the redundant `RHYTHM SELECT`, subtitle, and `SONG` lobby headings, then rebalanced the artwork, settings, controls, ranking area, start button, and help row for the 16:10 packaged layout.
- Fixed leaderboard best-score regression: because the PlayFab statistic was configured with `Last` aggregation, every completed run previously overwrote the stored score. Submission now reads the player's current statistic first and only sends an update when the completed run is a new personal best; lookup failure safely leaves the existing record untouched.
- Added a lobby Escape confirmation overlay with mouse-driven `게임 종료` and `취소` actions. Escape toggles the overlay instead of requiring Alt+F4, and lobby selection input is suppressed while the confirmation is visible.
- Reduced the gameplay Perfect/Great/Good/Miss feedback and HIT counter to 86% scale and moved the group slightly upward. The new scale, normalized vertical offset, and HIT font size remain editable in `WBP_RhythmGameplay` Class Defaults under `Rhythm|Appearance|Judgement`.
- Added SHEESH and DRIP from four aligned WAV stems each, plus BANG BANG, 404 (New Era), and 캐치캐치 from four-role MIDI stems aligned against their master WAVs. Generated and cataloged twenty difficulty assets with increasing Easy-to-Expert density, stable online SongIds, shared 1-24 chart levels, and automatic onset-quality reports.
- Added `/Game/Maps/LobbyTestMap` as an editor-only rapid-testing entry point. `ARhythmLobbyPlayerController` bypasses PlayFab login only when this map is running in an editor build; normal LobbyMap and packaged builds continue to require authentication.
- Began the AllGames multi-game platform layer. Added `MainHubMap`, shared account gating, catalog-driven mini-game definition assets, reusable game-entry cards, and an Escape exit confirmation. The default startup now enters MainHub; Rhythm launches the existing LobbyMap, while Idol Quiz is visible as a disabled coming-soon entry. Adding future games requires a definition asset and catalog entry rather than another hardcoded hub branch.
- Added an `ALL GAMES` action to the rhythm lobby so players can return to MainHub without restarting. Packaging configuration now cooks MainHubMap alongside the existing rhythm maps; no package was generated during this stage.
- Packaged gameplay then exposed an initialization-order race: `URhythmGameplayWidget::NativeConstruct()` ran before the GameMode spawned `ARhythmJudgementManager` and `ARhythmScoreManager`. Input, judgement, and scoring worked internally, but the UI never subscribed, so no feedback, score update, or judged-note removal appeared. Added idempotent runtime-manager binding that retries from tick until both managers exist and uses unique delegates; PIE and packaged startup order are now both supported.
- Confirmed the PlayFab operating assumption for development: Development Mode does not accrue monthly metered charges but has account/usage limits; switching the title to Live and a paid plan is an explicit, effectively permanent release action. Client-side score posting remains acceptable for private testing and is scheduled to move behind server validation before public competitive distribution.
- Raised judgement feedback from normalized Y 0.20 to 0.16, reduced its maximum area from 720x320 to 620x270, and reduced/raised the accompanying HIT counter for a less dominant upper-screen presentation. Blueprint appearance properties remain editable for later tuning.
- Added CANON-D from separated 120 BPM Vocal/Drums/Bass/Other MIDI sources and the existing 198.856-second master SoundWave. The reusable multitrack alignment/motif pipeline generated Easy/Normal/Hard/Expert 5-key charts with 403/495/927/1182 notes, 2/3/7/15 long notes, and coverage through 196.925 seconds.
- Created `DA_CanonD_{Easy,Normal,Hard,Expert}_5Key`, configured a 55-second lobby preview, expanded the catalog to five songs and twenty charts, and added dedicated plus full-catalog validators. Existing songs and comparison assets remain intact.
- Relaxed long-note release timing after playtesting: releasing up to 180 ms before the tail now completes the hold instead of the previous 80 ms. Earlier releases still break the note, and the shared `LongNoteReleaseGraceSeconds` remains editor-adjustable up to 350 ms.
- Added Drama and 만찬가 as complete four-difficulty 5-key song groups. Drama uses aligned Vocal/Drums/Bass/FX WAV authoring stems and generates 543/864/1246/1834 notes through 213.3 seconds; 만찬가 uses Vocal/Drums/Bass/Other MIDI aligned to its master WAV and generates 565/655/1040/1253 notes through 216.6 seconds.
- Created `DA_Drama_{Easy,Normal,Hard,Expert}_5Key` and `DA_Bansanka_{Easy,Normal,Hard,Expert}_5Key`, linked the existing `에스파-Drama` and `tuki-만찬가` SoundWaves, configured lobby previews, and expanded `DA_RhythmSongCatalog` to seven songs and twenty-eight charts.
- Added dedicated Drama/만찬가 validation and expanded full lobby validation. Both Unreal commandlets completed with zero errors and warnings; all new Data Assets and the catalog resolve to Git LFS. Manual PIE listening verification remains for musical feel, sync, previews, and late-song coverage.
- Added LoveAttack and 갑자기 as complete four-difficulty 5-key song groups. LoveAttack uses aligned Vocal/Drums/Bass/FX WAV stems and generates 594/786/1095/1457 notes through 179.4 seconds; 갑자기 uses Vocal/Drums/Bass/Other MIDI aligned to its master WAV and generates 523/592/857/1052 notes through 193.6 seconds.
- Created `DA_LoveAttack_{Easy,Normal,Hard,Expert}_5Key` and `DA_Suddenly_{Easy,Normal,Hard,Expert}_5Key`, linked the existing `리센느-LoveAttack` and `아이오아이-갑자기` SoundWaves, configured lobby previews, and expanded the catalog to nine songs and thirty-six charts.
- Added dedicated LoveAttack/갑자기 validation and expanded the full lobby validator. Both Unreal commandlets completed with zero errors and warnings, and all eight new Data Assets resolve to Git LFS. Manual PIE listening verification remains.
- Added a reusable automatic chart quality analyzer to both WAV-stem and multitrack-MIDI generation. Each generation now emits JSON and Markdown reports covering audio-onset match, median timing distance, density outliers, active-but-sparse windows, longest gaps, lane balance, long-note overlap, and prioritized five-second listening windows while excluding the intentional opening preparation region.
- Added HeavySerenade and RUDE! from aligned Vocal/Drums/Bass/FX WAV stems. HeavySerenade generates 471/752/1063/1539 notes through 178.1 seconds; RUDE! generates 524/858/1177/1731 notes through 195.5 seconds.
- Created `DA_HeavySerenade_{Easy,Normal,Hard,Expert}_5Key` and `DA_Rude_{Easy,Normal,Hard,Expert}_5Key`, linked their existing SoundWaves, configured lobby previews, and expanded the catalog to eleven songs and forty-four charts.
- HeavySerenade automatic onset match is 86.9-98.9%; priority Expert listening windows are 0:15-0:20, 2:05-2:15, and 2:55-2:59. RUDE! onset match is 92.1-100%; its main priority window is 0:45-0:50.
- Dedicated HeavySerenade/RUDE and full 11-song Unreal validations completed with zero errors and warnings; all eight new Data Assets resolve to Git LFS. Manual testing can focus on the generated priority windows before any full-song release pass.
- Backfilled automatic quality reports for all nine earlier production songs without regenerating or modifying their chart assets. The catalog now has onset-match metrics and prioritized listening windows for all eleven songs and forty-four difficulty charts.
- Added `GenerateAllChartQualitySummary.py` and generated `Docs/Analysis/AllSongsChartQualitySummary.{json,md}`. Current minimum per-song match priority begins with HeavySerenade 86.9%, LoveAttack 87.1%, and Drama 88.3%; CHASE-ME and RUDE remain `GOOD` at a 92.1% minimum. `CAUTION` can also reflect lane imbalance, so it is treated as a review priority rather than an automatic failure.
- Began music-grounded chart authoring for `Choom.wav`: added a repeatable NumPy spectral-flux/tempo/onset analysis script and documented the source as 176.053708 seconds, stereo 48 kHz PCM.
- Automatic analysis supports a 120 BPM working grid with a 0.0133-second initial offset; created a separate Space-only 10-25 second beat-sync verification chart workflow so timing can be listening-tested before vocal chart authoring.
- Kept the note spawner ticking after its last spawn; its actor-driven timeline is still required to move the final visible notes and animate judgement feedback, especially in short verification charts.
- First beat-sync PIE test showed stable early input errors near -147.5 ms while notes felt visually late. Preserved judgement windows, moved the test chart grid earlier from +0.0133 to -0.1342 seconds, regenerated `DA_Choom_BeatSyncTest`, and verified the commandlet with zero errors or warnings.
- User verified the calibrated beat-sync chart in PIE: natural Space inputs on audible attacks now produce predominantly Perfect judgements. Accepted 120 BPM and -0.1342 seconds as the current Choom timing baseline without widening judgement windows.
- Added a temporary tap-chart recorder for the first vocal-authoring pass. It captures pressed lanes and conductor time from 25-40 seconds and writes `Saved/ChartRecordings/ChoomTapRecording.csv` when PIE ends, allowing human-heard syllable onsets to become the chart source.
- Captured 47 human vocal/rhythm taps from 25.2084-39.9775 seconds. The sample contains 13 quarter-note-like and 33 eighth-note-like intervals with a 0.2665-second median; prepared an unquantized Space-only vocal playback-test asset workflow and recorder shutdown to preserve the source capture.
- Generated and assigned `DA_Choom_VocalTapTest` with all 47 unquantized human-perceived pulse/groove timestamps, disabled the temporary recorder, and completed the commandlet with zero errors or warnings.
- Prepared a clean 1-80 second perceptual-pulse capture with an empty chart, a separate output CSV, and no visual note prompts so the player's natural musical reference can be analyzed across a longer section without overwriting the first recording.
- Analyzed the completed 1-80 second capture: 212 taps, dominated by 117 eighth-like and 76 quarter-like intervals. After the 147.5 ms listening calibration, median error to the 120 BPM sixteenth-note grid is 14.8 ms and 80.2% fall within 30 ms. Adopted selective sixteenth-grid groove/syncopation as the player's chart-authoring rule rather than percussion-only onset selection.
- Prepared a playable Normal 9-key 1-80 second draft transformation: selective 35 ms sixteenth-grid snapping, preservation of expressive outliers, sub-90 ms merge protection, no chords, alternating hand-friendly lanes, and Space phrase accents.
- Generated and assigned `DA_Choom_HumanGroove_1_80`: all 212 taps became single notes, 181 were snapped, 31 expressive timestamps were preserved, none required merging, tap recording was disabled, and Git LFS handling was verified.
- User verified that the stage 6 gameplay screen and moving notes display correctly in PIE; marked stage 6 complete.
- Expanded the intended judgement design from two tiers to four: Perfect, Great, Good, and Miss. Stage 7 remains in progress until Great/Good are implemented and verified.
- Implemented the four judgement tiers with initial editable windows of 45/90/150/180 ms for Perfect/Great/Good/Miss.
- Connected `T_Judgement_Perfect`, `T_Judgement_Great`, `T_Judgement_Good`, and `T_Judgement_Miss` to the gameplay widget, displayed the latest judgement for 0.6 seconds, and removed judged note visuals.
- Increased the default judgement-line thickness from 8 to 24 UI units for 2560x1600 fullscreen visibility, moved judgement feedback from the screen center to 20% from the top, and exposed both layout values plus feedback size in WBP Class Defaults.
- Fixed flattened judgement art by uniformly scaling each source texture into a 720x320 maximum area while preserving its aspect ratio.
- Added optional input-driven Lane Glow overlays; no `T_LaneGlow` asset is currently present, so the texture still needs to be imported and assigned.
- Detected and connected the newly imported `T_LaneGlow`, set its default opacity to 50%, and widened the gameplay lane area from a fixed 900 units to a responsive 60% of screen width.
- Marked stages 7-8 complete after the user exercised fullscreen judgement feedback and requested progression to scoring.
- Added `ARhythmScoreManager`, editable judgement weights, score/combo/max-combo/count/accuracy state, change events, logs, and runtime score/combo/accuracy UI.
- Aligned each Lane Glow overlay's lower edge to the top edge of the judgement line instead of extending it below the line.
- Exposed judgement-line vertical position, lane-area screen-width fraction, Lane Glow top position, bottom offset, and horizontal padding under `WBP_RhythmGameplay` Class Defaults so layout tuning no longer requires C++ edits.
- Marked stage 9 complete after the user requested progression to the Data Asset stage.
- Added `URhythmSongDataAsset` with song, timing, key-mode, difficulty, travel-time, and note-array fields.
- Created `/Game/Data/DA_Choom_Test`, migrated the nine 5-13 second test notes, assigned Choom, and connected the asset to `BP_RhythmConductor`.
- Removed hard-coded temporary chart construction from `ARhythmNoteSpawner`; it now sorts and spawns the selected Data Asset notes.
- Input mode now follows the selected chart's 5-key/9-key setting, and note travel time comes from the chart asset.
- Enabled the editor-only Python plugin and added a repeatable editor script for creating/updating the Choom test Data Asset.
- Replaced the nine-note chart with a fixed-seed, full-song random test chart; consecutive identical lanes are avoided and simultaneous notes were removed at the user's request.
- Smoothed note motion between potentially sparse `OnAudioPlaybackPercent` callbacks by extrapolating from the latest audio-derived synchronization point with a high-resolution platform clock; every new audio callback corrects the timeline and frame `DeltaTime` is still not accumulated.
- Added automatic lane key labels below the judgement line (`A/S/D/F/SPACE/J/K/L/;` or `D/F/SPACE/J/K`) with editable font size, vertical offset, and color in WBP Class Defaults.
- Diagnosed apparent note freezing as Unreal Editor background throttling: runtime logs showed chart spawning and judgement continuing while editor frames dropped to roughly 3 FPS after focus moved away from PIE.
- Opened critical bug report `Docs/Bugs/BUG-001-NoteVisualFreeze.md` after the issue reproduced with background throttling disabled; investigation now tracks actor-driven UI timeline updates and per-note removal diagnostics.
- Added `ARhythmJudgementManager`, spawned automatically by `ARhythmGameModeBase`, to keep timing judgement outside input and UI classes.
- Bound spawned notes and zero-based lane input into a shared judgement path that works unchanged for 5-key and 9-key modes.
- Added a configurable +/-120 ms Perfect window, automatic Miss after 180 ms, closest-note selection per lane, duplicate-judgement prevention, judgement events, and timing-error logs.
- Investigated a gameplay WBP that received note events but rendered no visible widgets.
- Fixed the runtime-generated WidgetTree lifecycle by constructing it in `RebuildWidget()` before Slate rendering instead of in `NativeConstruct()`.
- Preserved automatic 5-key/9-key lane selection by resolving the active mode before the WidgetTree is built.
- Verified that all four appearance properties are assigned; the current Judgement Line Image points to `IMG_LaneBackground` and should be replaced if that was not intentional.
- Rebuilt `AllGamesEditor` successfully after the UI lifecycle fix; manual PIE visual verification remains pending.
- Continued `BUG-001` investigation after isolated 66-121 ms frame-gap warnings; these can cause visible jumps but do not explain persistent intermediate-position visuals.
- Exported and inspected `T_LaneGlow`; it contains no horizontal note-shaped artwork, confirming the bars in the reproduction are runtime note widgets.
- Fixed the upper-left flicker by positioning each note immediately on creation instead of leaving it at the canvas default position until the next timeline event.
- Explicitly invalidate moving note widgets after render-translation changes, allow overdue notes to move below the judgement line instead of clamping in place, and treat canceled Enhanced Input actions as lane releases.
- Rebuilt `AllGamesEditor` successfully after the presentation fixes; `BUG-001` remains open until manual PIE verification.
- A new reproduction proved the conductor timeline was repeatedly rewound by delayed playback-percent callbacks and exposed repeated runtime note-widget names derived from the active count. Changed the clock to remain monotonic after its initial audio sync and assigned every note image a monotonically increasing unique ID; manual verification remains required.
- User verified that `BUG-001` no longer reproduces; marked it resolved and documented the repeated UObject-name collision as the primary cause, with callback-driven timeline rewinds as a compounding defect.
- Added retriggerable judgement pop/fade animation so repeated Perfect/Great/Good/Miss results remain visually distinct, plus an in-feedback consecutive successful-hit counter that resets on Miss.
- Corrected the Semicolon debug-camera override: `DebugExecBindings` belongs to `/Script/Engine.PlayerInput`, not `InputSettings`. This hidden PIE-only engine binding is not listed in Editor Preferences keyboard-shortcut search.
- Created the dedicated `/Game/Maps/FiveKeyMap` by duplicating the proven TestMap setup while keeping TestMap available for 9-key development.
- Generated `/Game/Data/DA_Choom_5Key_Full` with 456 single notes spanning 4.7408-173.6158 seconds. The first 80 seconds preserve the user-recorded groove and the remainder uses analyzed vocal, drum, and strong transient candidates without simultaneous notes.
- Configured FiveKeyMap's Conductor instance to use the full 5-key chart and made FiveKeyMap the editor and packaged-game startup map. The active mapping remains D/F/Space/J/K through the existing SongData-driven input-mode selection.
- Added a song-finished result overlay with final score, max combo, accuracy, and Perfect/Great/Good/Miss totals. Enter reloads the current map and Escape exits the session.
- Added repeatable creation and validation scripts for the full 5-key chart/map. Automated validation passed for note ordering, unique timestamps, lane range/counts, map Conductor override, and disabled tap recording.
- Rebuilt `AllGamesEditor` successfully after the complete 5-key mode and result-screen implementation. Full-song PIE verification remains pending.

### 2026-07-19 - Idol Quiz single-player prototype

- Added data-driven Idol Quiz question types and catalog assets. Runtime code reads image, canonical answer, accepted aliases, group, and generation without hardcoding individual idols.
- Added `AIdolQuizGameModeBase`, `AIdolQuizPlayerController`, and `UIdolQuizWidget`. A game randomly draws 10 unique questions, accepts answers through the chat-style input, ignores case/spaces/punctuation during comparison, awards 100 points for a correct answer, and advances after feedback.
- Added restart and `ALL GAMES` navigation. The prototype intentionally remains single-player; multiplayer rooms, a round timer, and server-authoritative first-correct-answer ownership are future work.
- Imported 83 third-generation idol images from 13 group folders into `/Game/IdolQuiz/Images/Generation3`, built `/Game/IdolQuiz/Data/DA_IdolQuiz_3rdGeneration`, created `/Game/Maps/IdolQuizMap`, and enabled the Idol Quiz card in the shared MainHub.
- Preserved the user's source folder convention: immediate folder name becomes group metadata and the image filename without extension becomes the correct answer. Future bulk additions can use the same convention and import script.
- Found one mislabeled source file: `레드벨벳/예리.jpg` contained WEBP data. The original download was left untouched; a project-side PNG repair copy is used by the repeatable import workflow.
- UE 5.7's headless Interchange import crashed while notifying Content Browser because Slate was unavailable. The import was completed safely through a full Editor process with rendering disabled; this engine automation limitation is recorded in the workflow.
- Automated content validation passed: 83 questions, 83 image references, unique question IDs, nonempty answers/groups, generation metadata, 13 groups, IdolQuizMap, and enabled MainHub entry.
- Manual PIE verification still required: launch Idol Quiz from MainHub, confirm images display, correct/wrong answers behave as expected, 10 questions finish, restart works, and `ALL GAMES` returns to MainHub.

### 2026-07-15

- Initialized the Unreal project as a Git repository on `main` and connected `origin` to the private GitHub repository.
- Added Unreal/Rider `.gitignore` rules and Git LFS tracking for `.uasset` and `.umap`; corrected attribute ordering so binary assets remain non-text.
- Created TestMap, `ARhythmGameModeBase`, `BP_RhythmGameMode`, and the minimal level setup.
- Added the source-area folder structure for Core, Audio, Rhythm, Notes, Judgement, Scoring, Data, and UI.
- Implemented `ARhythmPlayerController`, lane events, nine Input Actions, and selectable 5-key/9-key mapping contexts.
- Removed the Semicolon Debug Camera shortcut conflict at the project level.
- Imported `Choom.wav`, created `ARhythmConductor`/`BP_RhythmConductor`, and enabled TestMap auto-play.
- Added audio-derived current time, duration, progress, playing-state, and playback-finished handling.
- Verified full Editor builds, TestMap startup, GameMode activation, input asset mappings, music assignment, duration, automatic playback, and LFS attributes.
- Added temporary note data, note actor, and music-time-aware spawner classes plus Blueprint children.
- Added a nine-note temporary chart targeting 5-13 seconds, with notes spawning two seconds before target time; manual PIE visual verification remains pending.
- User verified all nine notes spawning in order at approximately 3.008-11.008 seconds, consistently about two seconds before their 5-13 second target times.
- Decided to move the lane, judgement-line, and note presentation into gameplay UMG widgets while keeping chart, timing, spawning, and future judgement logic in C++.
- Added `URhythmGameplayWidget` and the actual `WBP_RhythmGameplay` Widget Blueprint, automatic viewport creation, adaptive 5/9-lane rendering, a judgement line, and music-time-derived UI note movement.
- Exposed replaceable background, lane, note, and judgement-line texture slots while retaining functional fallback colors.
