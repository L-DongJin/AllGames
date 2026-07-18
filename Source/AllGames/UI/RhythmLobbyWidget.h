// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Online/RhythmLeaderboardTypes.h"
#include "RhythmLobbyWidget.generated.h"

class UButton;
class UBorder;
class UAudioComponent;
class UImage;
class UFont;
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

	/** Normalized screen anchor for the ONLINE TOP 10 heading. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Leaderboard")
	FVector2D LeaderboardTitlePosition = FVector2D(0.82f, 0.30f);

	/** Normalized screen anchor for leaderboard player names, ranks, and scores. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Leaderboard")
	FVector2D LeaderboardEntriesPosition = FVector2D(0.82f, 0.35f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Leaderboard", meta = (ClampMin = "1.0", UIMin = "1.0"))
	FVector2D LeaderboardAreaSize = FVector2D(330.0f, 520.0f);

	/** Optional font asset shared by the leaderboard heading and player entries. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Leaderboard")
	TObjectPtr<UFont> LeaderboardFont;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Leaderboard", meta = (ClampMin = "8", ClampMax = "96"))
	int32 LeaderboardTitleFontSize = 26;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Leaderboard", meta = (ClampMin = "8", ClampMax = "96"))
	int32 LeaderboardEntryFontSize = 22;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Leaderboard")
	FLinearColor LeaderboardTitleColor = FLinearColor(0.35f, 0.9f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Leaderboard")
	FLinearColor LeaderboardEntryColor = FLinearColor(0.7f, 0.9f, 1.0f, 1.0f);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
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
	void RefreshLeaderboard();
	void SetExitConfirmationVisible(bool bVisible);
	void HandleTopLeaderboardCompleted(bool bSuccess, const FRhythmLeaderboardResult& Result);

	UFUNCTION() void PreviousDifficulty();
	UFUNCTION() void NextDifficulty();
	UFUNCTION() void PreviousSong();
	UFUNCTION() void NextSong();
	UFUNCTION() void DecreaseSpeed();
	UFUNCTION() void IncreaseSpeed();
	UFUNCTION() void HandleStartClicked();
	UFUNCTION() void HandleReturnToMainHub();
	UFUNCTION() void HandleExitConfirmed();
	UFUNCTION() void HandleExitCanceled();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> SongValueText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DifficultyValueText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ChartLevelText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SpeedValueText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LeaderboardTitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LeaderboardText;
	UPROPERTY(Transient) TObjectPtr<UButton> MainHubButton;
	UPROPERTY(Transient) TObjectPtr<UBorder> ExitConfirmationBackground;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ExitConfirmationTitle;
	UPROPERTY(Transient) TObjectPtr<UButton> ExitConfirmButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ExitCancelButton;
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
	bool bLeaderboardDelegatesBound = false;
	bool bExitConfirmationVisible = false;
	FString RequestedLeaderboardStatistic;
	int32 SelectedRow = 0;
};
