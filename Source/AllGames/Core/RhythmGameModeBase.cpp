// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmGameModeBase.h"

#include "RhythmPlayerController.h"
#include "../Judgement/RhythmJudgementManager.h"
#include "../Scoring/RhythmScoreManager.h"

ARhythmGameModeBase::ARhythmGameModeBase()
{
	PlayerControllerClass = ARhythmPlayerController::StaticClass();
}

void ARhythmGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("RhythmGameModeBase started."));

	JudgementManager = GetWorld()->SpawnActor<ARhythmJudgementManager>();
	ensureMsgf(JudgementManager, TEXT("Failed to create RhythmJudgementManager."));

	ScoreManager = GetWorld()->SpawnActor<ARhythmScoreManager>();
	ensureMsgf(ScoreManager, TEXT("Failed to create RhythmScoreManager."));
}
