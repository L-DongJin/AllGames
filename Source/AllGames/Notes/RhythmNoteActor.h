// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RhythmNoteData.h"
#include "RhythmNoteActor.generated.h"

class UStaticMeshComponent;

/** Visual runtime representation of a single rhythm note. */
UCLASS(Blueprintable)
class ALLGAMES_API ARhythmNoteActor : public AActor
{
	GENERATED_BODY()

public:
	ARhythmNoteActor();

	void InitializeNote(const FRhythmNoteData& InNoteData);

	UFUNCTION(BlueprintPure, Category = "Rhythm|Note")
	int32 GetLaneIndex() const { return NoteData.LaneIndex; }

	UFUNCTION(BlueprintPure, Category = "Rhythm|Note")
	float GetTargetTimeSeconds() const { return NoteData.TargetTimeSeconds; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rhythm|Note")
	TObjectPtr<UStaticMeshComponent> NoteMesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Rhythm|Note")
	FRhythmNoteData NoteData;
};
