# BUG-004: Room game combo Slate crash

## Symptom

- In the create-room popup, selecting `그림 퀴즈` could immediately terminate Unreal Editor.
- The crash report recorded `EXCEPTION_ACCESS_VIOLATION writing address 0x0000000400000008`.
- The call stack repeatedly cycled through Slate and SlateCore rather than EOS or room-session code.

## Cause

The room combo boxes used `OnGenerateWidget` to construct new `UTextBlock` objects through the parent widget's `WidgetTree` while Slate was generating combo rows. That runtime construction path could re-enter widget reconstruction and leave Slate with invalid row ownership.

## Fix

- Removed the dynamic row-generation callback.
- Added `URoomComboBoxString`, which applies the white foreground and font before `RebuildWidget` creates the underlying Slate combo.
- Both the Blueprint-backed room browser popup and the native fallback now use the safe combo class.
- Limited Person Quiz question-count options to 50-300 in 50-question increments.

## Verification

- Full `AllGamesEditor Win64 Development` build is required.
- Manual PIE check: open the room browser, open Create Room, switch repeatedly between Person Quiz and Drawing Quiz, expand every combo, and create one Drawing Quiz room.
