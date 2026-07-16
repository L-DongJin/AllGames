# BUG-002: CHASE-ME appears to stop notes and play time at 3:07

## Status

Fix implemented; awaiting manual PIE verification.

## User report

- In CHASE-ME Hard, notes appear to stop when the HUD first displays `TIME 3:07`.
- The play-time display appears to stop at the same moment.
- Audio is perceived as continuing briefly afterward.
- Follow-up observation: audible music begins at the real song start, but the gameplay HUD
  immediately begins around 0:49 instead of 0:00.

## Verified facts

- The four supplied authoring stems are all 187.5012 seconds long.
- Unreal's `/Game/Audio/Music/CHASE-ME` SoundWave is 187.5011 seconds long.
- The SoundWave is not configured to loop.
- The runtime log reports music completion at 187.501 seconds.
- The complete Hard chart is spawned: `1149 / 1149` in the original reproduction.
- After the initial-travel correction, Hard contains 1146 notes and still reaches the song tail.
- Hard's final note targets are:
  - 186.1195 seconds
  - 186.2008 seconds
  - 186.9787 seconds
- The integer HUD changes from `3:06` to `3:07` at 187.0000 seconds.
- Therefore, the final note crosses the judgement line approximately 21 ms before the HUD first
  shows `3:07`, and the remaining approximately 0.5 seconds contains only the audio tail.

## Root cause

The earlier theories about mismatched source lengths and a simple outro presentation boundary
were incorrect and have been withdrawn.

`ARhythmConductor::HandleAudioPlaybackPercent()` trusted the first
`OnAudioPlaybackPercent` callback as an absolute music position. For CHASE-ME, the first
callback could report approximately 49 seconds even though audible playback had just begun
at zero. This shifted the authoritative gameplay timeline forward by about 49 seconds:

1. the HUD began around `0:49`;
2. all chart notes whose spawn windows were before 49 seconds spawned immediately;
3. those notes appeared partway down the lanes instead of entering from the top;
4. the chart timeline reached 187.5 seconds after only about 138.5 seconds of audible music;
5. notes and HUD then stopped while roughly 49 seconds of the song remained audible.

## Fix

- Record the platform-clock time when playback is requested.
- Compare the first audio-percent callback against elapsed playback time.
- Reject an initial callback that differs by more than 0.5 seconds.
- Keep the zero-based extrapolated timeline active until a plausible callback arrives.
- Emit `Rejected invalid initial audio timeline callback` with callback, expected time, and
  percentage for regression diagnosis.

## Follow-up verification

- Start CHASE-ME Hard and confirm the HUD begins at `TIME 0:00`, not around `0:49`.
- Confirm early notes enter from the lane top after `GO!`.
- Play through the full song and confirm notes remain synchronized until the real ending.
- Check whether the rejection log appears once near playback start.

## Regression guards

- Song completion must continue to log `spawned total / chart total`.
- CHASE-ME validation requires the final note to remain at or after 186 seconds.
- Do not synthesize notes after the final detected source attack merely to fill the last half-second.
