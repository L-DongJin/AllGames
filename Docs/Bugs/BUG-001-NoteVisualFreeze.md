# BUG-001: Note visuals freeze, accumulate, or disappear unexpectedly

## Status

- Resolved and user-verified: 2026-07-16
- First recorded: 2026-07-16
- Affects: TestMap PIE gameplay, full-song deterministic test chart
- Severity: Critical gameplay blocker

## User-visible symptoms

- Some note visuals appear to stop at intermediate vertical positions.
- Multiple visuals accumulate at several heights even though note travel time is two seconds.
- Some notes appear to disappear before the player expects them to reach the judgement line.
- Simultaneous notes remained after the chart-generation script was changed because the Data Asset had not yet been regenerated while Unreal Editor was open.

## Expected behavior

- A note is spawned exactly `NoteTravelTimeSeconds` before its target time.
- Its position is derived continuously from conductor music time.
- It reaches the judgement line exactly at target time.
- It remains until a judgement event, then its matching visual is removed exactly once.
- The current single-note test chart never contains two notes at the same target time.

## Evidence collected

- Runtime logs show note spawning continuing at 0.5-second target intervals.
- Runtime logs show automatic Miss and scoring events continuing for the corresponding target times.
- A previous test showed editor frames dropping to roughly 3 FPS when focus moved away, but the user reproduced the issue again after disabling `Use Less CPU when in Background`.
- Screenshots show substantially more visuals than the expected four-to-six active notes for a two-second travel time and 0.5-second chart spacing.
- This indicates a presentation update/removal problem in addition to any editor throttling.

## Changes already attempted

1. Note position derived from conductor time instead of accumulated frame DeltaTime.
2. Conductor timeline interpolated between audio playback-percent callbacks using a high-resolution platform clock.
3. Unreal Editor background CPU throttling identified and disabled by the user.
4. Periodic chord generation removed from the chart script; the Data Asset still required regeneration afterward.

## Current hypotheses

1. `UUserWidget::NativeTick` is not running consistently for the runtime-generated WidgetTree.
2. Some judgement events reach the feedback image but fail to match/remove their corresponding `FNoteVisual`.
3. Slate layout invalidation may not be occurring reliably when canvas slot positions are updated from the widget tick.
4. Less likely: conductor time is non-monotonic or discontinuous despite spawning and judgement logs appearing correct.

## Investigation change set

- Regenerate `DA_Choom_Test` with one note per timestamp and no chords.
- Move UI position refresh from `UUserWidget::NativeTick` to a timeline event broadcast by the ticking `ARhythmNoteSpawner` actor.
- Log periodic music time and active visual count.
- Log whether each judgement successfully found and removed its matching visual.

## Verification checklist

- Startup log reports 339 notes, not 381.
- No two `Spawned note` logs share the same target time.
- `Rhythm UI timeline` logs advance monotonically about every five music seconds.
- Active visual count remains approximately four-to-six.
- Every `UI removed judged note` log reports success.
- No note remains stationary at an intermediate position.
- No note disappears before its judgement event.

## Investigation log

### 2026-07-16

- Opened the bug after a second user reproduction with background CPU throttling disabled.
- Preserved screenshot observations and runtime-log evidence.
- Began replacing widget-owned ticking with actor-driven timeline presentation updates.
- Regenerated `DA_Choom_Test` with 339 single notes and no duplicate target timestamps.
- Completed the actor-driven timeline update path and added five-second active-visual diagnostics plus per-judgement removal success/failure logs.
- Full `AllGamesEditor Win64 Development` build succeeded; manual PIE verification is pending and the bug remains open.
- User reported a major reduction in freezes/disappearances, but occasional intermediate-position freezes remain.
- New diagnostics showed the chart loading 339 notes, monotonic five-second timeline samples, stable active-visual counts of three-to-five, and successful visual removal for every observed judgement with zero removal failures.
- This rules out chart duplication, visual accumulation, and judgement matching as the remaining cause in the observed run.
- Remaining leading hypothesis: repeated `UCanvasPanelSlot::SetPosition` layout updates are not repainting reliably through Slate invalidation. Next change will keep slot layout static and animate notes through render translation.
- Replaced per-frame canvas-slot Y layout mutation with a static spawn slot plus per-frame `UWidget::SetRenderTranslation`, which uses Slate's animation/render-transform path and avoids repeated layout invalidation.
- Full user-provided log showed stable active-visual counts and no removal failures. It did expose a startup renderer/SBT hitch where music advanced from roughly 4.517 to 5.479 and the first Miss was delayed to +478.9 ms; subsequent logged gameplay was regular.
- Exported and visually inspected the active background, lane, and note textures. No fixed horizontal note-like bars exist in the background/lane art, ruling out texture decoration as the perceived freeze.
- Added warnings for every music-timeline step over 50 ms (and errors for backward time) so future reproductions identify exact frame hitches without the user needing to remember a timestamp.
- Strengthened judged-note cleanup by collapsing the image before removing it from its parent, preventing stale Slate visibility if removal and paint occur in the same frame.
- The latest reproduction reported isolated frame gaps of 66-121 ms. These explain brief jumps but not permanently stationary visuals; no backward timeline event was reported.
- Exported and inspected `T_LaneGlow`; it contains a vertical glow with no horizontal note-like bars, ruling out Lane Glow artwork as the accumulated bars in the screenshot.
- Identified the upper-left flicker: the spawner broadcasts its timeline update before broadcasting newly spawned notes, leaving a new image at the canvas default `(0,0)` until the next frame. New notes are now positioned immediately after creation.
- Explicitly invalidate each moving note's Slate layout/volatility state after changing its render translation, preventing reuse of stale cached draw elements.
- Removed the hard stop at progress `1.0`; an overdue note may now travel slightly below the judgement line while awaiting its automatic Miss event.
- Bound Enhanced Input `Canceled` events as releases to prevent a Lane Glow from remaining visible after focus/input cancellation.
- A subsequent full reproduction exposed repeated timeline reversal: one initial `0.170 -> 0.052` correction followed by many 0-3 ms callback rewinds. Playback-percent callbacks are delayed relative to the extrapolated platform clock, so subsequent stale callbacks now preserve the later extrapolated time and the public timeline is guarded as monotonically increasing.
- Found that UI note UObject names used `NoteVisuals.Num()`. Because the active count remains around four-to-five, names could repeat for the same lane while old constructed widgets still exist in the WidgetTree. Replaced this with a monotonically increasing visual ID so every chart note owns a uniquely named image.
- User completed another PIE run and confirmed that the freeze/disappearance bug no longer reproduces. The primary visual corruption cause was repeated UObject widget names; delayed audio callbacks repeatedly rewinding the extrapolated clock was a compounding timing defect. Both corrections are retained.
