# AllGames Rhythm Game Development Plan

Last updated: 2026-07-16

## Goal

Build a small Unreal Engine 5.7 rhythm-game prototype in which one song can be played from start to finish. The first complete version must support 5-key and 9-key layouts, music-synchronized notes, judgement, score/combo, data-driven charts, and a result screen.

## Current status

- Prototype progress: stages 1-10 complete; stage 11 implemented and awaiting full-song PIE verification.
- Current stage: three-song motif-aware catalog awaiting PIE listening verification for Lemonade and It'sMe.
- Next stage after user verification: tune any remaining per-song alignment or motif-selection errors, then formalize the repeatable song-import workflow.
- Default map: `/Game/Maps/LobbyMap`; it starts `/Game/Maps/FiveKeyMap` after settings are confirmed.
- Preserved 9-key test map: `/Game/Maps/TestMap`.
- Playable songs: `/Game/Audio/Music/Choom`, `/Game/Audio/Music/Lemonade`, and `/Game/Audio/Music/It_sMe`.
- Test song format: stereo, 48 kHz, 16-bit PCM WAV, approximately 176.054 seconds.
- Default input mode follows SongData; FiveKeyMap selects 5-key.
- Latest successful full build: `AllGamesEditor Win64 Development` on 2026-07-16.
- Repository scope now includes the lobby, complete 5-key gameplay loop, MIDI authoring tools, and the three-song production catalog.
- Detailed chart-authoring history and the reusable current workflow are documented in `Docs/ChartAuthoringPipeline.md`.

## Implemented architecture

### Game framework

- `ARhythmGameModeBase` is the C++ gameplay GameMode base.
- `BP_RhythmGameMode` is the editor-configurable Blueprint child.
- `FiveKeyMap` is the editor startup map and game default map; `TestMap` remains the 9-key development map.
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
