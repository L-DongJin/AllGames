// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmJudgementManager.h"

#include "EngineUtils.h"
#include "../Core/RhythmGameInstance.h"
#include "../Core/RhythmPlayerController.h"
#include "../Notes/RhythmNoteSpawner.h"
#include "../Rhythm/RhythmConductor.h"

ARhythmJudgementManager::ARhythmJudgementManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARhythmJudgementManager::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ARhythmConductor> It(GetWorld()); It; ++It)
	{
		Conductor = *It;
		break;
	}

	for (TActorIterator<ARhythmNoteSpawner> It(GetWorld()); It; ++It)
	{
		Spawner = *It;
		break;
	}

	PlayerController = Cast<ARhythmPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!ensureMsgf(Conductor && Spawner && PlayerController,
		TEXT("RhythmJudgementManager requires a conductor, note spawner, and rhythm player controller.")))
	{
		SetActorTickEnabled(false);
		return;
	}

	Spawner->OnNoteSpawned.AddDynamic(this, &ThisClass::HandleNoteSpawned);
	PlayerController->OnLaneInput.AddDynamic(this, &ThisClass::HandleLaneInput);
	if (const URhythmGameInstance* Settings = Cast<URhythmGameInstance>(GetGameInstance()))
	{
		const float WindowScale = Settings->GetJudgementWindowScale();
		PerfectWindowSeconds *= WindowScale;
		GreatWindowSeconds *= WindowScale;
		GoodWindowSeconds *= WindowScale;
		MissWindowSeconds *= WindowScale;
	}
	ensureMsgf(PerfectWindowSeconds <= GreatWindowSeconds
		&& GreatWindowSeconds <= GoodWindowSeconds
		&& GoodWindowSeconds <= MissWindowSeconds,
		TEXT("Judgement windows must be ordered Perfect <= Great <= Good <= Miss."));
	UE_LOG(LogTemp, Log, TEXT("Rhythm judgement ready: Perfect %.0f / Great %.0f / Good %.0f / Miss %.0f ms."),
		PerfectWindowSeconds * 1000.0f, GreatWindowSeconds * 1000.0f,
		GoodWindowSeconds * 1000.0f, MissWindowSeconds * 1000.0f);
}

void ARhythmJudgementManager::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Conductor || !Conductor->IsMusicPlaying())
	{
		return;
	}

	const float MusicTime = Conductor->GetMusicTimeSeconds();
	for (int32 Index = 0; Index < PendingNotes.Num(); ++Index)
	{
		FPendingRhythmNote& Note = PendingNotes[Index];
		if (!Note.bJudged && MusicTime > Note.Data.TargetTimeSeconds + MissWindowSeconds)
		{
			JudgeNote(Index, ERhythmJudgement::Miss, MusicTime - Note.Data.TargetTimeSeconds);
		}
	}
}

void ARhythmJudgementManager::HandleNoteSpawned(const FRhythmNoteData NoteData)
{
	FPendingRhythmNote& PendingNote = PendingNotes.AddDefaulted_GetRef();
	PendingNote.Data = NoteData;
}

void ARhythmJudgementManager::HandleLaneInput(const int32 LaneIndex, const bool bPressed)
{
	if (!bPressed || !Conductor || !Conductor->IsMusicPlaying())
	{
		return;
	}

	const float MusicTime = Conductor->GetMusicTimeSeconds();
	int32 ClosestIndex = INDEX_NONE;
	float ClosestAbsoluteError = TNumericLimits<float>::Max();
	float ClosestSignedError = 0.0f;

	for (int32 Index = 0; Index < PendingNotes.Num(); ++Index)
	{
		const FPendingRhythmNote& Note = PendingNotes[Index];
		if (Note.bJudged || Note.Data.LaneIndex != LaneIndex)
		{
			continue;
		}

		const float SignedError = MusicTime - Note.Data.TargetTimeSeconds;
		const float AbsoluteError = FMath::Abs(SignedError);
		if (AbsoluteError <= GoodWindowSeconds && AbsoluteError < ClosestAbsoluteError)
		{
			ClosestIndex = Index;
			ClosestAbsoluteError = AbsoluteError;
			ClosestSignedError = SignedError;
		}
	}

	if (ClosestIndex != INDEX_NONE)
	{
		ERhythmJudgement Judgement = ERhythmJudgement::Good;
		if (ClosestAbsoluteError <= PerfectWindowSeconds)
		{
			Judgement = ERhythmJudgement::Perfect;
		}
		else if (ClosestAbsoluteError <= GreatWindowSeconds)
		{
			Judgement = ERhythmJudgement::Great;
		}
		JudgeNote(ClosestIndex, Judgement, ClosestSignedError);
	}
}

void ARhythmJudgementManager::JudgeNote(
	const int32 PendingNoteIndex, const ERhythmJudgement Judgement, const float TimingErrorSeconds)
{
	if (!PendingNotes.IsValidIndex(PendingNoteIndex) || PendingNotes[PendingNoteIndex].bJudged)
	{
		return;
	}

	FPendingRhythmNote& Note = PendingNotes[PendingNoteIndex];
	Note.bJudged = true;
	OnNoteJudged.Broadcast(Note.Data, Judgement, TimingErrorSeconds);

	const TCHAR* JudgementName = TEXT("Miss");
	switch (Judgement)
	{
	case ERhythmJudgement::Perfect: JudgementName = TEXT("Perfect"); break;
	case ERhythmJudgement::Great: JudgementName = TEXT("Great"); break;
	case ERhythmJudgement::Good: JudgementName = TEXT("Good"); break;
	case ERhythmJudgement::Miss: break;
	}

	UE_LOG(LogTemp, Log, TEXT("%s: lane %d, target %.3f, error %+.1f ms"),
		JudgementName,
		Note.Data.LaneIndex + 1,
		Note.Data.TargetTimeSeconds,
		TimingErrorSeconds * 1000.0f);
}
