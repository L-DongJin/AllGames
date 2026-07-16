// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RhythmConductor.generated.h"

class UAudioComponent;
class USoundBase;
class USoundWave;
class URhythmSongDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRhythmMusicFinished);

/** Owns rhythm music playback and exposes its audio-derived timeline. */
UCLASS(Blueprintable)
class ALLGAMES_API ARhythmConductor : public AActor
{
	GENERATED_BODY()

public:
	ARhythmConductor();

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Audio")
	FOnRhythmMusicFinished OnMusicFinished;

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Audio")
	void PlayMusic();

	/** Starts the preparation countdown once the gameplay note UI is fully visible. */
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Timing")
	void StartGameplayCountdown();

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Audio")
	void StopMusic();

	/** Pauses or resumes both audible playback and the music-derived gameplay clock. */
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Audio")
	void SetMusicPaused(bool bPaused);

	/** Current audio playback position in seconds. */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Timing")
	float GetMusicTimeSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Rhythm|Timing")
	float GetMusicDurationSeconds() const;

	/** Normalized playback position in the inclusive range 0..1. */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Timing")
	float GetMusicPlaybackProgress() const;

	UFUNCTION(BlueprintPure, Category = "Rhythm|Audio")
	bool IsMusicPlaying() const;

	UFUNCTION(BlueprintPure, Category = "Rhythm|Timing")
	bool IsStartCountdownActive() const { return bStartCountdownActive; }

	UFUNCTION(BlueprintPure, Category = "Rhythm|Timing")
	float GetStartCountdownSecondsRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Rhythm|Data")
	URhythmSongDataAsset* GetSongData() const { return SongData; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rhythm|Audio")
	TObjectPtr<UAudioComponent> MusicComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rhythm|Audio")
	TObjectPtr<USoundBase> Music;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rhythm|Data")
	TObjectPtr<URhythmSongDataAsset> SongData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rhythm|Audio")
	bool bAutoPlayOnBeginPlay = true;

	/** Preparation time before music, note spawning, and judgement begin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rhythm|Timing", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float StartCountdownSeconds = 3.0f;

private:
	void BeginMusicAfterCountdown();

	UFUNCTION()
	void HandleAudioPlaybackPercent(const USoundWave* PlayingSoundWave, float PlaybackPercent);

	UFUNCTION()
	void HandleAudioFinished();

	UPROPERTY(VisibleInstanceOnly, Category = "Rhythm|Timing")
	float MusicTimeSeconds = 0.0f;

	/** Platform-clock instant at which MusicTimeSeconds was last synchronized to audio. */
	double LastTimelineSyncPlatformSeconds = 0.0;

	/** The first callback establishes the audio clock; later delayed callbacks may never rewind it. */
	bool bHasReceivedAudioTimelineSync = false;

	/** Platform-clock instant when the current playback was requested. */
	double PlaybackStartPlatformSeconds = 0.0;

	bool bLoggedInvalidInitialTimelineCallback = false;

	/** Final guard against float rounding or delayed callbacks returning a decreasing public time. */
	mutable float LastReturnedMusicTimeSeconds = 0.0f;

	FTimerHandle StartCountdownTimerHandle;
	double CountdownEndPlatformSeconds = 0.0;
	bool bStartCountdownActive = false;
	bool bWaitingForGameplayCountdown = false;
	bool bMusicPaused = false;
};
