// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmGameModeBase.h"

#include "RhythmPlayerController.h"

ARhythmGameModeBase::ARhythmGameModeBase()
{
	PlayerControllerClass = ARhythmPlayerController::StaticClass();
}

void ARhythmGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("RhythmGameModeBase started."));
}
