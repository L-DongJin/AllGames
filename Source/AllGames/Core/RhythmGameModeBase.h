// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RhythmGameModeBase.generated.h"

/**
 * Base game mode for rhythm gameplay maps.
 *
 * Gameplay systems will be added in later development steps. For now, this
 * class provides a stable C++ parent for map-specific Blueprint settings.
 */
UCLASS()
class ALLGAMES_API ARhythmGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARhythmGameModeBase();

protected:
	virtual void BeginPlay() override;

private:
	/** Temporary authoring mode; disable after the first vocal phrase has been captured. */
	UPROPERTY(EditDefaultsOnly, Category = "Rhythm|Chart Recording")
	bool bEnableTapChartRecording = true;

	UPROPERTY(Transient)
	TObjectPtr<class ARhythmJudgementManager> JudgementManager;

	UPROPERTY(Transient)
	TObjectPtr<class ARhythmScoreManager> ScoreManager;

	UPROPERTY(Transient)
	TObjectPtr<class ARhythmChartTapRecorder> ChartTapRecorder;
};
