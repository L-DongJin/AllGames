// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RhythmLobbyWidget.generated.h"

class UButton;
class UAudioComponent;
class UImage;
class USoundBase;
class UTextBlock;
class UTexture2D;

/** Functional pre-play lobby. Visuals can later move into a Blueprint child without changing settings logic. */
UCLASS()
class ALLGAMES_API URhythmLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URhythmLobbyWidget(const FObjectInitializer& ObjectInitializer);

	/** Full-screen lobby background assigned from WBP_RhythmLobby Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance")
	TObjectPtr<UTexture2D> LobbyBackgroundImage;

	/** Tint and opacity applied over the assigned lobby background texture. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance")
	FLinearColor LobbyBackgroundTint = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	/** Width of the selected song artwork in the lobby. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SongImageWidth = 720.0f;

	/** Height of the selected song artwork in the lobby. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SongImageHeight = 480.0f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
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
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ChartLevelText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SpeedValueText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpText;
	UPROPERTY(Transient) TObjectPtr<UImage> SongTitleImage;
	UPROPERTY(Transient) TObjectPtr<UImage> PreviousSongTitleImage;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> DisplayedTitleTexture;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> PreviewAudioComponent;
	UPROPERTY(Transient) TObjectPtr<USoundBase> PreviewMusic;
	FTimerHandle PreviewLoopTimerHandle;
	float PreviewStartTimeSeconds = 0.0f;
	float PreviewDurationSeconds = 15.0f;
	float TitleTransitionElapsed = 0.0f;
	float TitleTransitionDuration = 0.28f;
	bool bTitleTransitionActive = false;
	int32 SelectedRow = 0;
};
