// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RhythmLeaderboardTypes.generated.h"

USTRUCT(BlueprintType)
struct ALLGAMES_API FRhythmLeaderboardEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Leaderboard")
	int32 Rank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Leaderboard")
	FString PlayerId;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Leaderboard")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Leaderboard")
	int32 Score = 0;
};

USTRUCT(BlueprintType)
struct ALLGAMES_API FRhythmLeaderboardResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Leaderboard")
	FString StatisticName;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Leaderboard")
	TArray<FRhythmLeaderboardEntry> Entries;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRhythmScoreSubmissionCompleted, bool, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRhythmLeaderboardRequestCompleted, bool, const FRhythmLeaderboardResult&);
