// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RhythmLobbyWidget.generated.h"

class UButton;
class UAudioComponent;
class USoundBase;
class UTextBlock;

/** Functional pre-play lobby. Visuals can later move into a Blueprint child without changing settings logic. */
UCLASS()
class ALLGAMES_API URhythmLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URhythmLobbyWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void RefreshSettings();
	void RefreshSongPreview();
	void StopSongPreview();
	void RestartSongPreview();
	void StartGame();

	UFUNCTION() void PreviousDifficulty();
	UFUNCTION() void NextDifficulty();
	UFUNCTION() void PreviousSong();
	UFUNCTION() void NextSong();
	UFUNCTION() void DecreaseSpeed();
	UFUNCTION() void IncreaseSpeed();
	UFUNCTION() void HandleStartClicked();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> SongValueText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DifficultyValueText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SpeedValueText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpText;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> PreviewAudioComponent;
	UPROPERTY(Transient) TObjectPtr<USoundBase> PreviewMusic;
	FTimerHandle PreviewLoopTimerHandle;
	float PreviewStartTimeSeconds = 0.0f;
	float PreviewDurationSeconds = 15.0f;
	int32 SelectedRow = 0;
};
