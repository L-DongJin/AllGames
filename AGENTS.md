# AllGames Repository Instructions

## Project

- This is a small PC rhythm game built with Unreal Engine 5.7 and JetBrains Rider.
- Use C++ for timing, input, judgement, scoring, and other game rules.
- Use Blueprint for asset assignment, visual presentation, animation, effects, and editor tuning.
- Keep the project suitable for solo/indie development. Do not introduce large frameworks before a concrete need appears.

## Required workflow

- Work on only the development stage requested by the user. Do not start the next stage until the user explicitly asks for it.
- Inspect existing files and assets before changing them. Preserve user changes and unrelated work.
- New reflected C++ types require a full `AllGamesEditor` build with Unreal Editor closed. Ask the user to save and close the editor when necessary.
- Verify each completed stage with an Editor target build and the narrowest relevant runtime or asset checks.
- Do not commit or push unless the user explicitly asks. Leave changes visible in GitHub Desktop for review.
- Keep Unreal binary assets (`.uasset`, `.umap`) under Git LFS and never treat them as text.

## Architecture rules

- `ARhythmConductor` owns music playback and the music-derived timeline.
- Gameplay timing must use `ARhythmConductor::GetMusicTimeSeconds()`. Do not drive notes or judgement from accumulated frame `DeltaTime`.
- `ARhythmPlayerController` owns Enhanced Input and broadcasts zero-based lane input through `OnLaneInput`.
- Support both input layouts:
  - 5-key: `D`, `F`, `Space`, `J`, `K`
  - 9-key: `A`, `S`, `D`, `F`, `Space`, `J`, `K`, `L`, `Semicolon`
- Use separate mapping contexts for 5-key and 9-key modes. Mode switching must not require changes to judgement logic.
- UI displays gameplay state but does not calculate timing, judgement, score, or combo.
- Song and chart values must move to Data Assets once the temporary chart prototype has been validated.

## Source organization

- `Source/AllGames/Core`: game framework and player controller classes.
- `Source/AllGames/Audio`: general audio helpers.
- `Source/AllGames/Rhythm`: conductor and rhythm timing.
- `Source/AllGames/Notes`: note actors, spawning, and movement.
- `Source/AllGames/Judgement`: timing-window evaluation.
- `Source/AllGames/Scoring`: score, combo, and accuracy.
- `Source/AllGames/Data`: song and chart data types.
- `Source/AllGames/UI`: C++ UI bases and view models.

## Build and verification

Use the Unreal Engine 5.7 Editor target for full C++ verification:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' AllGamesEditor Win64 Development 'C:\GitHub\AllGames\AllGames.uproject' -WaitMutex -NoHotReloadFromIDE
```

Before handing off a completed change:

- Confirm the Editor target builds successfully.
- Run `git diff --check`.
- Confirm newly created Unreal assets have the expected LFS attributes.
- Perform the relevant TestMap runtime check and report any manual PIE check still required.

## Documentation maintenance

- `Docs/DevelopmentPlan.md` is the project source of truth for progress and design decisions.
- At the end of every user-verified completed stage, update that document without waiting for a separate reminder.
- Also update it whenever controls, architecture, public C++/Blueprint APIs, assets, test procedures, or roadmap decisions change materially.
- Add a dated entry to the work log describing what was added, changed, fixed, and verified.
- Keep `Current status`, `Next stage`, completed-stage notes, and known issues accurate.
- Do not mark a stage complete until implementation is verified and the user confirms the expected behavior when a manual PIE check is required.
