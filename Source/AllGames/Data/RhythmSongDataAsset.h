// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "../Notes/RhythmNoteData.h"
#include "RhythmSongDataAsset.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class ERhythmChartKeyMode : uint8
{
	FiveKey UMETA(DisplayName = "5 Key"),
	NineKey UMETA(DisplayName = "9 Key"),
};

UENUM(BlueprintType)
enum class ERhythmDifficulty : uint8
{
	Easy,
	Normal,
	Hard,
	Expert,
};

/** Editable song metadata and note chart used by the runtime gameplay systems. */
UCLASS(BlueprintType)
class ALLGAMES_API URhythmSongDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song")
	FText SongTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song")
	TObjectPtr<USoundBase> Music;

	/**
	 * Lobby preview start time in seconds.
	 * A negative value automatically selects a representative section near the middle of the song.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song|Preview", meta = (ClampMin = "-1.0", UIMin = "-1.0"))
	float PreviewStartTimeSeconds = -1.0f;

	/** Length of the looping lobby preview. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song|Preview", meta = (ClampMin = "3.0", UIMin = "3.0"))
	float PreviewDurationSeconds = 15.0f;

	/** Preview-only volume multiplier. Gameplay music volume is unaffected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song|Preview", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PreviewVolume = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "1.0"))
	float BPM = 120.0f;

	/** Time in seconds at which the chart's first beat grid begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	float MusicOffsetSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chart")
	ERhythmChartKeyMode KeyMode = ERhythmChartKeyMode::NineKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chart")
	ERhythmDifficulty Difficulty = ERhythmDifficulty::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chart", meta = (ClampMin = "0.1"))
	float NoteTravelTimeSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chart")
	TArray<FRhythmNoteData> Notes;

	UFUNCTION(BlueprintPure, Category = "Chart")
	int32 GetLaneCount() const { return KeyMode == ERhythmChartKeyMode::FiveKey ? 5 : 9; }
};
