// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Judgement/RhythmJudgementManager.h"
#include "RhythmScoreManager.generated.h"

USTRUCT(BlueprintType)
struct ALLGAMES_API FRhythmScoreState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Scoring") int64 Score = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Scoring") int32 Combo = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Scoring") int32 MaxCombo = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Scoring") int32 PerfectCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Scoring") int32 GreatCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Scoring") int32 GoodCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Scoring") int32 MissCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Scoring") float AccuracyPercent = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRhythmScoreChanged, FRhythmScoreState, ScoreState);

/** Calculates score, combo, counts, and accuracy from judgement events. */
UCLASS(Blueprintable)
class ALLGAMES_API ARhythmScoreManager : public AActor
{
	GENERATED_BODY()

public:
	ARhythmScoreManager();

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Scoring")
	FOnRhythmScoreChanged OnScoreChanged;

	UFUNCTION(BlueprintPure, Category = "Rhythm|Scoring")
	FRhythmScoreState GetScoreState() const { return ScoreState; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleNoteJudged(FRhythmNoteData NoteData, ERhythmJudgement Judgement, float TimingErrorSeconds);

	UPROPERTY(EditAnywhere, Category = "Rhythm|Scoring", meta = (ClampMin = "0")) int32 PerfectScore = 1000;
	UPROPERTY(EditAnywhere, Category = "Rhythm|Scoring", meta = (ClampMin = "0")) int32 GreatScore = 700;
	UPROPERTY(EditAnywhere, Category = "Rhythm|Scoring", meta = (ClampMin = "0")) int32 GoodScore = 400;

	UPROPERTY(Transient)
	TObjectPtr<ARhythmJudgementManager> JudgementManager;

	UPROPERTY(VisibleInstanceOnly, Category = "Rhythm|Scoring")
	FRhythmScoreState ScoreState;

	int64 EarnedAccuracyPoints = 0;
	int64 PossibleAccuracyPoints = 0;
};
