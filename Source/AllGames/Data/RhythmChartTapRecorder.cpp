// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmChartTapRecorder.h"

#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "../Core/RhythmPlayerController.h"
#include "../Rhythm/RhythmConductor.h"

ARhythmChartTapRecorder::ARhythmChartTapRecorder()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARhythmChartTapRecorder::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ARhythmConductor> It(GetWorld()); It; ++It)
	{
		Conductor = *It;
		break;
	}
	PlayerController = Cast<ARhythmPlayerController>(GetWorld()->GetFirstPlayerController());
	if (ensureMsgf(Conductor && PlayerController, TEXT("Chart tap recorder requires a Conductor and RhythmPlayerController.")))
	{
		PlayerController->OnLaneInput.AddDynamic(this, &ThisClass::HandleLaneInput);
		UE_LOG(LogTemp, Warning, TEXT("CHART RECORDING ACTIVE: tap audible vocal syllables from %.1f to %.1f seconds."),
			RecordStartSeconds, RecordEndSeconds);
	}
}

void ARhythmChartTapRecorder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SaveRecording();
	Super::EndPlay(EndPlayReason);
}

void ARhythmChartTapRecorder::HandleLaneInput(const int32 LaneIndex, const bool bPressed)
{
	if (!bPressed || !Conductor || !Conductor->IsMusicPlaying())
	{
		return;
	}

	const float MusicTime = Conductor->GetMusicTimeSeconds();
	if (MusicTime < RecordStartSeconds || MusicTime > RecordEndSeconds)
	{
		return;
	}

	FRecordedRhythmTap& Tap = RecordedTaps.AddDefaulted_GetRef();
	Tap.LaneIndex = LaneIndex;
	Tap.MusicTimeSeconds = MusicTime;
	UE_LOG(LogTemp, Log, TEXT("CHART TAP: lane %d, music %.4f"), LaneIndex + 1, MusicTime);
}

void ARhythmChartTapRecorder::SaveRecording() const
{
	if (RecordedTaps.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Chart recording ended with no taps between %.1f and %.1f seconds."),
			RecordStartSeconds, RecordEndSeconds);
		return;
	}

	FString Csv = TEXT("lane_index,target_time_seconds\n");
	for (const FRecordedRhythmTap& Tap : RecordedTaps)
	{
		Csv += FString::Printf(TEXT("%d,%.4f\n"), Tap.LaneIndex, Tap.MusicTimeSeconds);
	}

	const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ChartRecordings"));
	IFileManager::Get().MakeDirectory(*Directory, true);
	const FString OutputPath = FPaths::Combine(Directory, OutputFileName);
	if (FFileHelper::SaveStringToFile(Csv, *OutputPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Warning, TEXT("CHART RECORDING SAVED: %d taps -> %s"), RecordedTaps.Num(), *OutputPath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save chart recording: %s"), *OutputPath);
	}
}
