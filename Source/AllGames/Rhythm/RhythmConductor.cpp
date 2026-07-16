// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmConductor.h"

#include "Components/AudioComponent.h"
#include "HAL/PlatformTime.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"
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
		bWaitingForGameplayCountdown = true;
	}
}

void ARhythmConductor::StartGameplayCountdown()
{
	if (!bWaitingForGameplayCountdown || bStartCountdownActive || IsMusicPlaying())
	{
		return;
	}

	bWaitingForGameplayCountdown = false;
	if (StartCountdownSeconds <= 0.0f)
	{
		PlayMusic();
		return;
	}

	bStartCountdownActive = true;
	CountdownEndPlatformSeconds = FPlatformTime::Seconds() + StartCountdownSeconds;
	GetWorldTimerManager().SetTimer(
		StartCountdownTimerHandle,
		this,
		&ThisClass::BeginMusicAfterCountdown,
		StartCountdownSeconds,
		false);
	UE_LOG(LogTemp, Log, TEXT("Rhythm start countdown after gameplay UI ready: %.1f seconds."),
		StartCountdownSeconds);
}

void ARhythmConductor::PlayMusic()
{
	if (!ensureMsgf(Music, TEXT("RhythmConductor requires a Music asset before playback.")))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(StartCountdownTimerHandle);
	bWaitingForGameplayCountdown = false;
	bStartCountdownActive = false;
	MusicComponent->SetSound(Music);
	MusicTimeSeconds = 0.0f;
	bHasReceivedAudioTimelineSync = false;
	bLoggedInvalidInitialTimelineCallback = false;
	LastReturnedMusicTimeSeconds = 0.0f;
	PlaybackStartPlatformSeconds = FPlatformTime::Seconds();
	LastTimelineSyncPlatformSeconds = PlaybackStartPlatformSeconds;
	MusicComponent->Play();
	UE_LOG(LogTemp, Log, TEXT("RhythmConductor started music: %s"), *Music->GetName());
}

void ARhythmConductor::StopMusic()
{
	GetWorldTimerManager().ClearTimer(StartCountdownTimerHandle);
	bWaitingForGameplayCountdown = false;
	bStartCountdownActive = false;
	MusicComponent->Stop();
	MusicTimeSeconds = 0.0f;
	bHasReceivedAudioTimelineSync = false;
	bLoggedInvalidInitialTimelineCallback = false;
	LastReturnedMusicTimeSeconds = 0.0f;
	PlaybackStartPlatformSeconds = FPlatformTime::Seconds();
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

float ARhythmConductor::GetStartCountdownSecondsRemaining() const
{
	return bStartCountdownActive
		? FMath::Max(0.0, CountdownEndPlatformSeconds - FPlatformTime::Seconds())
		: 0.0f;
}

void ARhythmConductor::BeginMusicAfterCountdown()
{
	bStartCountdownActive = false;
	PlayMusic();
}

void ARhythmConductor::HandleAudioPlaybackPercent(const USoundWave* PlayingSoundWave, const float PlaybackPercent)
{
	if (!PlayingSoundWave)
	{
		return;
	}

	const float CallbackMusicTime = FMath::Clamp(PlaybackPercent, 0.0f, 1.0f) * PlayingSoundWave->Duration;
	const float ExpectedPlaybackTime = static_cast<float>(FMath::Max(
		0.0,
		FPlatformTime::Seconds() - PlaybackStartPlatformSeconds));
	if (!bHasReceivedAudioTimelineSync)
	{
		// Some compressed SoundWaves can report a stale/non-zero playback percentage on their
		// first callback even though audible playback began at zero. Never let that callback
		// jump the gameplay timeline tens of seconds ahead.
		constexpr float InitialCallbackToleranceSeconds = 0.5f;
		if (FMath::Abs(CallbackMusicTime - ExpectedPlaybackTime) > InitialCallbackToleranceSeconds)
		{
			if (!bLoggedInvalidInitialTimelineCallback)
			{
				UE_LOG(LogTemp, Error,
					TEXT("Rejected invalid initial audio timeline callback: callback %.3f, expected %.3f, percent %.4f."),
					CallbackMusicTime, ExpectedPlaybackTime, PlaybackPercent);
				bLoggedInvalidInitialTimelineCallback = true;
			}
			return;
		}

		MusicTimeSeconds = CallbackMusicTime;
		LastReturnedMusicTimeSeconds = FMath::Max(LastReturnedMusicTimeSeconds, CallbackMusicTime);
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
