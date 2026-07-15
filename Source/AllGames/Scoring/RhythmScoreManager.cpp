// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmScoreManager.h"

#include "EngineUtils.h"

ARhythmScoreManager::ARhythmScoreManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARhythmScoreManager::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ARhythmJudgementManager> It(GetWorld()); It; ++It)
	{
		JudgementManager = *It;
		break;
	}

	if (ensureMsgf(JudgementManager, TEXT("RhythmScoreManager requires a RhythmJudgementManager.")))
	{
		JudgementManager->OnNoteJudged.AddDynamic(this, &ThisClass::HandleNoteJudged);
		UE_LOG(LogTemp, Log, TEXT("Rhythm scoring ready."));
	}
}

void ARhythmScoreManager::HandleNoteJudged(
	const FRhythmNoteData NoteData, const ERhythmJudgement Judgement, const float TimingErrorSeconds)
{
	int32 AwardedScore = 0;
	switch (Judgement)
	{
	case ERhythmJudgement::Perfect:
		AwardedScore = PerfectScore;
		++ScoreState.PerfectCount;
		break;
	case ERhythmJudgement::Great:
		AwardedScore = GreatScore;
		++ScoreState.GreatCount;
		break;
	case ERhythmJudgement::Good:
		AwardedScore = GoodScore;
		++ScoreState.GoodCount;
		break;
	case ERhythmJudgement::Miss:
		++ScoreState.MissCount;
		break;
	}

	ScoreState.Score += AwardedScore;
	EarnedAccuracyPoints += AwardedScore;
	PossibleAccuracyPoints += PerfectScore;
	ScoreState.AccuracyPercent = PossibleAccuracyPoints > 0
		? static_cast<float>(EarnedAccuracyPoints) / static_cast<float>(PossibleAccuracyPoints) * 100.0f
		: 0.0f;

	if (Judgement == ERhythmJudgement::Miss)
	{
		ScoreState.Combo = 0;
	}
	else
	{
		++ScoreState.Combo;
		ScoreState.MaxCombo = FMath::Max(ScoreState.MaxCombo, ScoreState.Combo);
	}

	OnScoreChanged.Broadcast(ScoreState);
	UE_LOG(LogTemp, Log, TEXT("Score %lld | Combo %d | Max %d | Accuracy %.2f%%"),
		ScoreState.Score, ScoreState.Combo, ScoreState.MaxCombo, ScoreState.AccuracyPercent);
}
