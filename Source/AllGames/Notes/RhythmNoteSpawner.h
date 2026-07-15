// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RhythmNoteData.h"
#include "RhythmNoteSpawner.generated.h"

class ARhythmConductor;
class ARhythmNoteActor;
class URhythmSongDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRhythmNoteSpawned, FRhythmNoteData, NoteData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRhythmTimelineUpdated, float, MusicTimeSeconds);

/** Spawns a temporary hard-coded chart from the conductor's music timeline. */
UCLASS(Blueprintable)
class ALLGAMES_API ARhythmNoteSpawner : public AActor
{
	GENERATED_BODY()

public:
	ARhythmNoteSpawner();

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Notes")
	FOnRhythmNoteSpawned OnNoteSpawned;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Notes")
	FOnRhythmTimelineUpdated OnTimelineUpdated;

	float GetSpawnLeadTimeSeconds() const { return SpawnLeadTimeSeconds; }
	ARhythmConductor* GetConductor() const { return Conductor; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void SpawnNote(const FRhythmNoteData& NoteData);
	FVector GetLaneSpawnLocation(int32 LaneIndex) const;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Notes")
	TSubclassOf<ARhythmNoteActor> NoteClass;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Notes")
	bool bSpawnWorldDebugNotes = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Rhythm|Notes")
	float SpawnLeadTimeSeconds = 2.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Rhythm|Notes")
	int32 LaneCount = 9;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Notes")
	float LaneSpacing = 120.0f;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Notes")
	FVector SpawnCenter = FVector(800.0, 0.0, 100.0);

	UPROPERTY(Transient)
	TObjectPtr<ARhythmConductor> Conductor;

	UPROPERTY(Transient)
	TObjectPtr<URhythmSongDataAsset> SongData;

	UPROPERTY(Transient)
	TArray<FRhythmNoteData> ActiveNotes;

	int32 NextNoteIndex = 0;
	float LastTimelineBroadcastMusicTime = -1.0f;
};
