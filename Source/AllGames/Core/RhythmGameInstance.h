// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "../Data/RhythmSongDataAsset.h"
#include "RhythmGameInstance.generated.h"

class URhythmSongCatalogDataAsset;

/** Keeps the lobby's play settings alive while moving between lobby and gameplay maps. */
UCLASS()
class ALLGAMES_API URhythmGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	URhythmGameInstance();

	ERhythmDifficulty GetSelectedDifficulty() const { return SelectedDifficulty; }
	float GetScrollSpeed() const { return ScrollSpeed; }
	float GetNoteDensity() const;
	float GetJudgementWindowScale() const;
	float GetTravelTimeSeconds(float ChartTravelTimeSeconds) const;
	FText GetDifficultyDisplayName() const;
	URhythmSongDataAsset* GetSelectedSong() const;
	FText GetSelectedSongDisplayName() const;

	void ChangeSong(int32 Direction);
	void ChangeDifficulty(int32 Direction);
	void ChangeScrollSpeed(int32 Direction);

private:
	UPROPERTY()
	TObjectPtr<URhythmSongCatalogDataAsset> SongCatalog;

	UPROPERTY()
	int32 SelectedSongIndex = 0;

	UPROPERTY()
	ERhythmDifficulty SelectedDifficulty = ERhythmDifficulty::Normal;

	/** Visual scroll multiplier. This never changes music time or note target time. */
	UPROPERTY()
	float ScrollSpeed = 1.0f;
};
