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

## Web image and dataset collection safety

- Do not use the Codex in-app browser to bulk-download images or to emit full DOM snapshots, full inline scripts, page HTML, base64 data, or image bytes into the conversation.
- Use the browser only when a bounded metadata check cannot be completed with a small direct HTTP response. Prefer a local resumable script for collection and conversion.
- Discover a new source in stages:
  1. Make one bounded metadata request with a 15-second timeout and a 1-2 MB response limit.
  2. Confirm the expected item count and inspect no more than two names and image URLs.
  3. Download and validate one image before starting the full collection.
- For the full collection, default to one concurrent download, at least 750 ms between requests, a 60-second per-file timeout, and a 10 MB per-file limit. Do not exceed concurrency 2 without explicit user approval.
- Save at most 10 items per batch and atomically update a manifest and progress checkpoint after every batch. On restart, skip files whose existence, decode, dimensions, and SHA-256 already match the manifest.
- Validate every completed image: nonempty file, successful decode, positive width and height, intended output format, and SHA-256. Record failures without infinite retries.
- Stop the collection and report to the user on HTTP 429, access blocking, repeated network errors, unexpected response size or type, decoder failure, memory/tool errors, or a missing browser context. Do not bypass site protections or repeatedly restore a broken browser context.
- Keep original display names in the manifest. When filenames should use those names, remove only instructed annotations, replace Windows-invalid characters safely, detect collisions before renaming, and update manifest references atomically.
- Do not render or preview an entire collected dataset in the Codex conversation. Report counts, sizes, validation results, and a small sample only.
- For PIKU specifically, use the small server-side ranking JSON endpoint when available and keep image downloads outside the browser. Reuse the checkpointed local collection pattern demonstrated by `Scripts/CollectNarutoQuiz.ps1`.
- When the Chrome page-assets fallback is required, match every exported asset back to its ranking row by the exact image URL. Never pair names and exported files by array position because page-assets bundle order is not stable.
- If the PIKU ranking JSON endpoint is unavailable but the public ranking table renders normally in a user-connected Chrome session, use that visible table as the second-choice metadata source. Read only the expected row count plus the candidate names and image links; do not export full DOM, scripts, cookies, or session data.
- For the Chrome fallback, verify exactly two name/link pairs, then export and validate one observed image through the browser page-assets capability before continuing. Acquire images sequentially, never more than 10 per checkpoint batch, and immediately convert and record each image in the resumable manifest. Do not imitate browser headers, replay cookies, solve challenges, or call a blocked JSON endpoint again.
- If the connected Chrome page shows a CAPTCHA, Cloudflare challenge, missing rows, unexpected candidate count, or any asset export failure, stop and report it. A successful user-visible ranking table is required for this fallback; it is not permission to bypass access controls.
- These rules reduce crash risk but do not guarantee that the Codex app or a third-party site will never fail. Preserve resumability so an app crash never requires restarting a completed batch.

## Documentation maintenance

- `Docs/DevelopmentPlan.md` is the project source of truth for progress and design decisions.
- At the end of every user-verified completed stage, update that document without waiting for a separate reminder.
- Also update it whenever controls, architecture, public C++/Blueprint APIs, assets, test procedures, or roadmap decisions change materially.
- Add a dated entry to the work log describing what was added, changed, fixed, and verified.
- Keep `Current status`, `Next stage`, completed-stage notes, and known issues accurate.
- Do not mark a stage complete until implementation is verified and the user confirms the expected behavior when a manual PIE check is required.
