// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmNoteSpawner.h"

#include "EngineUtils.h"
#include "RhythmNoteActor.h"
#include "../Data/RhythmSongDataAsset.h"
#include "../Rhythm/RhythmConductor.h"

ARhythmNoteSpawner::ARhythmNoteSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	NoteClass = ARhythmNoteActor::StaticClass();
}

void ARhythmNoteSpawner::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ARhythmConductor> It(GetWorld()); It; ++It)
	{
		Conductor = *It;
		break;
	}

	if (!ensureMsgf(Conductor, TEXT("RhythmNoteSpawner requires one RhythmConductor in the level.")))
	{
		SetActorTickEnabled(false);
		return;
	}

	SongData = Conductor->GetSongData();
	if (!ensureMsgf(SongData, TEXT("RhythmNoteSpawner requires SongData on the RhythmConductor.")))
	{
		SetActorTickEnabled(false);
		return;
	}

	ActiveNotes = SongData->Notes;
	ActiveNotes.Sort([](const FRhythmNoteData& Left, const FRhythmNoteData& Right)
	{
		return Left.TargetTimeSeconds < Right.TargetTimeSeconds;
	});
	LaneCount = SongData->GetLaneCount();
	SpawnLeadTimeSeconds = SongData->NoteTravelTimeSeconds;
	UE_LOG(LogTemp, Log, TEXT("Chart ready from %s: %d notes, %d lanes, %.2f second travel."),
		*SongData->GetName(), ActiveNotes.Num(), LaneCount, SpawnLeadTimeSeconds);
}

void ARhythmNoteSpawner::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Conductor || !Conductor->IsMusicPlaying())
	{
		return;
	}

	const float MusicTime = Conductor->GetMusicTimeSeconds();
	if (LastTimelineBroadcastMusicTime >= 0.0f)
	{
		const float TimelineStep = MusicTime - LastTimelineBroadcastMusicTime;
		if (TimelineStep < -KINDA_SMALL_NUMBER)
		{
			UE_LOG(LogTemp, Error, TEXT("Rhythm timeline moved backward: %.3f -> %.3f"),
				LastTimelineBroadcastMusicTime, MusicTime);
		}
		else if (TimelineStep > 0.05f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Rhythm frame gap: %.1f ms at music %.3f"),
				TimelineStep * 1000.0f, MusicTime);
		}
	}
	LastTimelineBroadcastMusicTime = MusicTime;
	OnTimelineUpdated.Broadcast(MusicTime);
	while (ActiveNotes.IsValidIndex(NextNoteIndex)
		&& MusicTime >= ActiveNotes[NextNoteIndex].TargetTimeSeconds - SpawnLeadTimeSeconds)
	{
		SpawnNote(ActiveNotes[NextNoteIndex]);
		++NextNoteIndex;
	}

	// Keep broadcasting the music timeline after the final spawn. UI notes still need to travel
	// through their target time, and judgement feedback still uses this presentation clock.
}

void ARhythmNoteSpawner::SpawnNote(const FRhythmNoteData& NoteData)
{
	if (bSpawnWorldDebugNotes && ensure(NoteClass))
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		if (ARhythmNoteActor* Note = GetWorld()->SpawnActor<ARhythmNoteActor>(
			NoteClass, GetLaneSpawnLocation(NoteData.LaneIndex), FRotator::ZeroRotator, SpawnParameters))
		{
			Note->InitializeNote(NoteData);
		}
	}

	OnNoteSpawned.Broadcast(NoteData);
	UE_LOG(LogTemp, Log, TEXT("Spawned note: lane %d, target %.2f, music %.3f"),
		NoteData.LaneIndex + 1, NoteData.TargetTimeSeconds, Conductor->GetMusicTimeSeconds());
}

FVector ARhythmNoteSpawner::GetLaneSpawnLocation(const int32 LaneIndex) const
{
	const float CenteredLane = static_cast<float>(LaneIndex) - (static_cast<float>(LaneCount) - 1.0f) * 0.5f;
	return SpawnCenter + FVector(0.0, CenteredLane * LaneSpacing, 0.0);
}
