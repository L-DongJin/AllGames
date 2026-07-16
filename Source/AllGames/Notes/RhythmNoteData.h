// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RhythmNoteData.generated.h"

/** Minimal runtime note data used by the temporary C++ chart. */
USTRUCT(BlueprintType)
struct ALLGAMES_API FRhythmNoteData
{
	GENERATED_BODY()

	FRhythmNoteData() = default;
	FRhythmNoteData(const int32 InLaneIndex, const float InTargetTimeSeconds)
		: LaneIndex(InLaneIndex), TargetTimeSeconds(InTargetTimeSeconds)
	{
	}

	FRhythmNoteData(const int32 InLaneIndex, const float InTargetTimeSeconds, const float InDurationSeconds)
		: LaneIndex(InLaneIndex), TargetTimeSeconds(InTargetTimeSeconds), DurationSeconds(InDurationSeconds)
	{
	}

	/** Zero-based lane index. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Note")
	int32 LaneIndex = 0;

	/** Music time at which this note should reach the judgement line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Note", meta = (ClampMin = "0.0"))
	float TargetTimeSeconds = 0.0f;

	/** Zero creates a tap note. Positive values create a hold note ending at target + duration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Note", meta = (ClampMin = "0.0"))
	float DurationSeconds = 0.0f;

	bool IsLongNote() const { return DurationSeconds > KINDA_SMALL_NUMBER; }
	float GetEndTimeSeconds() const { return TargetTimeSeconds + FMath::Max(DurationSeconds, 0.0f); }
};
