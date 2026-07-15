// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Notes/RhythmNoteData.h"
#include "RhythmJudgementManager.generated.h"

class ARhythmConductor;
class ARhythmNoteSpawner;
class ARhythmPlayerController;

UENUM(BlueprintType)
enum class ERhythmJudgement : uint8
{
	Perfect,
	Great,
	Good,
	Miss,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnRhythmNoteJudged, FRhythmNoteData, NoteData, ERhythmJudgement, Judgement, float, TimingErrorSeconds);

struct FPendingRhythmNote
{
	FRhythmNoteData Data;
	bool bJudged = false;
};

/** Owns note eligibility and evaluates lane input against the conductor timeline. */
UCLASS(Blueprintable)
class ALLGAMES_API ARhythmJudgementManager : public AActor
{
	GENERATED_BODY()

public:
	ARhythmJudgementManager();

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Judgement")
	FOnRhythmNoteJudged OnNoteJudged;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UFUNCTION()
	void HandleNoteSpawned(FRhythmNoteData NoteData);

	UFUNCTION()
	void HandleLaneInput(int32 LaneIndex, bool bPressed);

	void JudgeNote(int32 PendingNoteIndex, ERhythmJudgement Judgement, float TimingErrorSeconds);

	UPROPERTY(EditAnywhere, Category = "Rhythm|Judgement", meta = (ClampMin = "0.0"))
	float PerfectWindowSeconds = 0.045f;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Judgement", meta = (ClampMin = "0.0"))
	float GreatWindowSeconds = 0.09f;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Judgement", meta = (ClampMin = "0.0"))
	float GoodWindowSeconds = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Judgement", meta = (ClampMin = "0.0"))
	float MissWindowSeconds = 0.18f;

	UPROPERTY(Transient)
	TObjectPtr<ARhythmConductor> Conductor;

	UPROPERTY(Transient)
	TObjectPtr<ARhythmNoteSpawner> Spawner;

	UPROPERTY(Transient)
	TObjectPtr<ARhythmPlayerController> PlayerController;

	TArray<FPendingRhythmNote> PendingNotes;
};
