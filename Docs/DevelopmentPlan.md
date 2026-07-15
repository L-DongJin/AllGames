# AllGames Rhythm Game Development Plan

Last updated: 2026-07-16

## Goal

Build a small Unreal Engine 5.7 rhythm-game prototype in which one song can be played from start to finish. The first complete version must support 5-key and 9-key layouts, music-synchronized notes, judgement, score/combo, data-driven charts, and a result screen.

## Current status

- Prototype progress: stages 1-9 of 11 completed.
- Current stage: stage 10 implemented; manual PIE and editor asset verification pending.
- Next stage after user verification: stage 11, finish song and show results.
- Test map: `/Game/Maps/TestMap`.
- Test song: `/Game/Audio/Music/Choom`.
- Test song format: stereo, 48 kHz, 16-bit PCM WAV, approximately 176.054 seconds.
- Default input mode: 9-key.
- Latest successful full build: `AllGamesEditor Win64 Development` on 2026-07-16.
- Git state: current stage 3-4, 5/9-key input, and documentation changes are not yet committed or pushed.

## Implemented architecture

### Game framework

- `ARhythmGameModeBase` is the C++ gameplay GameMode base.
- `BP_RhythmGameMode` is the editor-configurable Blueprint child.
- `TestMap` is both the editor startup map and game default map.
- TestMap contains a floor, directional light, sky light, Player Start, and one RhythmConductor actor.

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
- `BP_RhythmConductor` is placed once in TestMap and references `Choom`.
- Public timing API:
  - `GetMusicTimeSeconds()`
  - `GetMusicDurationSeconds()`
  - `GetMusicPlaybackProgress()`
  - `IsMusicPlaying()`
- Music time is derived from `OnAudioPlaybackPercent` and SoundWave duration, not accumulated frame time.
- `PlayMusic()` resets time to zero. Manual stop resets it to zero. Natural completion sets it to song duration.

### Song and chart data

- `FRhythmNoteData` stores a zero-based lane index and target music time.
- `ARhythmNoteActor` is the runtime visual note and exposes its lane and target time.
- `ARhythmNoteSpawner` reads `ARhythmConductor::GetMusicTimeSeconds()` and spawns notes at a configurable lead time.
- `BP_RhythmNote` and `BP_RhythmNoteSpawner` provide editor-tunable Blueprint children.
- TestMap contains exactly one RhythmNoteSpawner.
- `URhythmSongDataAsset` stores title, music, BPM, timing offset, 5/9-key mode, difficulty, note travel time, and note array.
- `/Game/Data/DA_Choom_Test` contains a deterministic full-song test chart from 5-174 seconds at 0.5-second intervals with one random 9-key lane per timestamp and no simultaneous notes.
- `BP_RhythmConductor` references `DA_Choom_Test`; the conductor supplies the song data to input selection and note spawning.
- Choom BPM 120 and offset 0 are placeholders until the song's real beat grid is measured.

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
	- Implementation complete; manual editor/PIE verification pending.
11. **Finish song and show results**
    - Stop gameplay, finalize remaining notes, show results, and provide retry/exit actions.

## Prototype completion criteria

- A user can launch TestMap and play one full song.
- A song/chart can select either the 5-key or 9-key layout.
- Notes remain synchronized to the music timeline.
- Inputs produce deterministic judgement results.
- Score, combo, accuracy, and results are displayed.
- Adding a new chart does not require editing gameplay C++.

## Later backlog

- Song selection screen and difficulty selection.
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
