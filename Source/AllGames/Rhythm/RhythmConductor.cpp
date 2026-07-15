// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmConductor.h"

#include "Components/AudioComponent.h"
#include "HAL/PlatformTime.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "../Core/RhythmGameInstance.h"
#include "../Data/RhythmSongDataAsset.h"

ARhythmConductor::ARhythmConductor()
{
	PrimaryActorTick.bCanEverTick = false;

	MusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicComponent"));
	SetRootComponent(MusicComponent);
	MusicComponent->bAutoActivate = false;
	MusicComponent->OnAudioPlaybackPercent.AddDynamic(this, &ThisClass::HandleAudioPlaybackPercent);
	MusicComponent->OnAudioFinished.AddDynamic(this, &ThisClass::HandleAudioFinished);
}

void ARhythmConductor::BeginPlay()
{
	Super::BeginPlay();
	if (const URhythmGameInstance* Settings = Cast<URhythmGameInstance>(GetGameInstance()))
	{
		if (URhythmSongDataAsset* SelectedSong = Settings->GetSelectedSong())
		{
			SongData = SelectedSong;
		}
	}
	if (SongData && SongData->Music)
	{
		Music = SongData->Music;
	}

	if (bAutoPlayOnBeginPlay)
	{
		PlayMusic();
	}
}

void ARhythmConductor::PlayMusic()
{
	if (!ensureMsgf(Music, TEXT("RhythmConductor requires a Music asset before playback.")))
	{
		return;
	}

	MusicComponent->SetSound(Music);
	MusicTimeSeconds = 0.0f;
	bHasReceivedAudioTimelineSync = false;
	LastReturnedMusicTimeSeconds = 0.0f;
	LastTimelineSyncPlatformSeconds = FPlatformTime::Seconds();
	MusicComponent->Play();
	UE_LOG(LogTemp, Log, TEXT("RhythmConductor started music: %s"), *Music->GetName());
}

void ARhythmConductor::StopMusic()
{
	MusicComponent->Stop();
	MusicTimeSeconds = 0.0f;
	bHasReceivedAudioTimelineSync = false;
	LastReturnedMusicTimeSeconds = 0.0f;
	LastTimelineSyncPlatformSeconds = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Log, TEXT("RhythmConductor stopped music."));
}

float ARhythmConductor::GetMusicTimeSeconds() const
{
	if (!IsMusicPlaying())
	{
		return MusicTimeSeconds;
	}

	const double SecondsSinceAudioSync = FMath::Max(
		0.0, FPlatformTime::Seconds() - LastTimelineSyncPlatformSeconds);
	const float CandidateTime = FMath::Clamp(
		MusicTimeSeconds + static_cast<float>(SecondsSinceAudioSync),
		0.0f,
		GetMusicDurationSeconds());
	LastReturnedMusicTimeSeconds = FMath::Max(LastReturnedMusicTimeSeconds, CandidateTime);
	return LastReturnedMusicTimeSeconds;
}

float ARhythmConductor::GetMusicDurationSeconds() const
{
	return Music ? Music->GetDuration() : 0.0f;
}

float ARhythmConductor::GetMusicPlaybackProgress() const
{
	const float Duration = GetMusicDurationSeconds();
	return Duration > 0.0f ? FMath::Clamp(GetMusicTimeSeconds() / Duration, 0.0f, 1.0f) : 0.0f;
}

bool ARhythmConductor::IsMusicPlaying() const
{
	return MusicComponent && MusicComponent->IsPlaying();
}

void ARhythmConductor::HandleAudioPlaybackPercent(const USoundWave* PlayingSoundWave, const float PlaybackPercent)
{
	if (!PlayingSoundWave)
	{
		return;
	}

	const float CallbackMusicTime = FMath::Clamp(PlaybackPercent, 0.0f, 1.0f) * PlayingSoundWave->Duration;
	if (!bHasReceivedAudioTimelineSync)
	{
		// Establish the real audio position once. This occurs before the playable chart begins.
		MusicTimeSeconds = CallbackMusicTime;
		LastReturnedMusicTimeSeconds = CallbackMusicTime;
		bHasReceivedAudioTimelineSync = true;
	}
	else
	{
		// Playback-percent delegates arrive on the game thread after the audio position they report.
		// Preserve the extrapolated clock when a callback is stale instead of repeatedly rewinding it.
		MusicTimeSeconds = FMath::Max(GetMusicTimeSeconds(), CallbackMusicTime);
		LastReturnedMusicTimeSeconds = MusicTimeSeconds;
	}
	LastTimelineSyncPlatformSeconds = FPlatformTime::Seconds();
}

void ARhythmConductor::HandleAudioFinished()
{
	MusicTimeSeconds = GetMusicDurationSeconds();
	LastReturnedMusicTimeSeconds = MusicTimeSeconds;
	LastTimelineSyncPlatformSeconds = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Log, TEXT("RhythmConductor music finished at %.3f seconds."), MusicTimeSeconds);
	OnMusicFinished.Broadcast();
}
