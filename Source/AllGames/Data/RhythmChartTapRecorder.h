// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RhythmChartTapRecorder.generated.h"

class ARhythmConductor;
class ARhythmPlayerController;

USTRUCT()
struct FRecordedRhythmTap
{
	GENERATED_BODY()

	int32 LaneIndex = 0;
	float MusicTimeSeconds = 0.0f;
};

/** Temporary in-PIE authoring helper that records audible-syllable taps to a CSV file. */
UCLASS()
class ALLGAMES_API ARhythmChartTapRecorder : public AActor
{
	GENERATED_BODY()

public:
	ARhythmChartTapRecorder();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleLaneInput(int32 LaneIndex, bool bPressed);

	void SaveRecording() const;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Chart Recording", meta = (ClampMin = "0.0"))
	float RecordStartSeconds = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Chart Recording", meta = (ClampMin = "0.0"))
	float RecordEndSeconds = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Chart Recording")
	FString OutputFileName = TEXT("ChoomTapRecording_1_80.csv");

	UPROPERTY(Transient)
	TObjectPtr<ARhythmConductor> Conductor;

	UPROPERTY(Transient)
	TObjectPtr<ARhythmPlayerController> PlayerController;

	TArray<FRecordedRhythmTap> RecordedTaps;
};
