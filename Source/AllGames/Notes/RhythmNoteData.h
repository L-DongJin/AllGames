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

	/** Zero-based lane index. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Note")
	int32 LaneIndex = 0;

	/** Music time at which this note should reach the judgement line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Note", meta = (ClampMin = "0.0"))
	float TargetTimeSeconds = 0.0f;
};
