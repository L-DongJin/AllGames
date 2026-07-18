// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Notes/RhythmNoteData.h"
#include "../Scoring/RhythmScoreManager.h"
#include "../Online/RhythmLeaderboardTypes.h"
#include "RhythmGameplayWidget.generated.h"

class ARhythmConductor;
class ARhythmJudgementManager;
class ARhythmNoteSpawner;
class ARhythmScoreManager;
enum class ERhythmJudgement : uint8;
class UCanvasPanel;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
enum class ERhythmLongNoteState : uint8;

UCLASS(Abstract, Blueprintable)
class ALLGAMES_API URhythmGameplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URhythmGameplayWidget(const FObjectInitializer& ObjectInitializer);

	/** Opens or closes the in-game restart/lobby pause menu. */
	void TogglePauseMenu();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance")
	TObjectPtr<UTexture2D> BackgroundImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance")
	TObjectPtr<UTexture2D> LaneBackgroundImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance")
	TObjectPtr<UTexture2D> LaneGlowImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LaneGlowOpacity = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance")
	TObjectPtr<UTexture2D> NoteImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Long Note")
	TObjectPtr<UTexture2D> LongNoteHeadImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Long Note")
	TObjectPtr<UTexture2D> LongNoteBodyImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Long Note")
	TObjectPtr<UTexture2D> LongNoteTailImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Long Note")
	TObjectPtr<UTexture2D> LongNoteHoldGlowImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Long Note")
	TObjectPtr<UTexture2D> LongNoteCompleteEffectImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Long Note")
	TObjectPtr<UTexture2D> LongNoteBreakEffectImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Long Note", meta = (ClampMin = "4.0", ClampMax = "100.0"))
	float LongNoteHeadHeight = 28.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Long Note", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float LongNoteEffectDisplaySeconds = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance")
	TObjectPtr<UTexture2D> JudgementLineImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Layout", meta = (ClampMin = "4.0", ClampMax = "100.0"))
	float JudgementLineThickness = 24.0f;

	/** Normalized vertical position within the lane area: 0 is top, 1 is bottom. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Layout", meta = (ClampMin = "0.1", ClampMax = "0.98"))
	float JudgementLineVerticalPosition = 0.85f;

	/** Fraction of the screen width occupied by the complete lane area. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Layout", meta = (ClampMin = "0.2", ClampMax = "1.0"))
	float LaneAreaScreenWidthFraction = 0.6f;

	/** Normalized point where Lane Glow starts within the lane area. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Layout", meta = (ClampMin = "0.0", ClampMax = "0.95"))
	float LaneGlowTopPosition = 0.0f;

	/** Positive values leave a gap above the line; negative values overlap it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Layout", meta = (ClampMin = "-100.0", ClampMax = "300.0"))
	float LaneGlowBottomOffset = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Layout", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float LaneGlowHorizontalPadding = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Key Labels", meta = (ClampMin = "8", ClampMax = "96"))
	int32 LaneKeyLabelFontSize = 28;

	/** Distance from the bottom edge of the judgement line to the key labels. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Key Labels", meta = (ClampMin = "0.0", ClampMax = "200.0"))
	float LaneKeyLabelVerticalOffset = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Key Labels")
	FLinearColor LaneKeyLabelColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement")
	TObjectPtr<UTexture2D> PerfectJudgementImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement")
	TObjectPtr<UTexture2D> GreatJudgementImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement")
	TObjectPtr<UTexture2D> GoodJudgementImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement")
	TObjectPtr<UTexture2D> MissJudgementImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement", meta = (ClampMin = "0.05"))
	float JudgementDisplaySeconds = 0.6f;

	/** Time used by the pop-in animation whenever a new judgement is received. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float JudgementPopAnimationSeconds = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement", meta = (ClampMin = "1.0", ClampMax = "3.0"))
	float JudgementPopStartScale = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float JudgementFeedbackVerticalPosition = 0.16f;

	/** Maximum area for judgement art; the source texture aspect ratio is preserved. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement")
	FVector2D JudgementFeedbackMaxSize = FVector2D(620.0f, 270.0f);

	/** Final size multiplier shared by Perfect/Great/Good/Miss art and the HIT counter. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement", meta = (ClampMin = "0.25", ClampMax = "2.0"))
	float JudgementFeedbackScale = 0.86f;

	/** Additional normalized vertical offset; negative values move feedback upward. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement", meta = (ClampMin = "-0.5", ClampMax = "0.5"))
	float JudgementFeedbackVerticalOffset = -0.025f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Judgement", meta = (ClampMin = "8", ClampMax = "96"))
	int32 JudgementHitCountFontSize = 30;

	/** Duration of the animated score count-up on the result screen. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Appearance|Result", meta = (ClampMin = "0.2", ClampMax = "5.0"))
	float ResultCountUpDuration = 1.8f;

private:
	UFUNCTION()
	void HandleNoteSpawned(FRhythmNoteData NoteData);

	UFUNCTION()
	void HandleTimelineUpdated(float MusicTimeSeconds);

	UFUNCTION()
	void HandleNoteJudged(FRhythmNoteData NoteData, ERhythmJudgement Judgement, float TimingErrorSeconds);

	UFUNCTION()
	void HandleLongNoteStateChanged(FRhythmNoteData NoteData, ERhythmLongNoteState State);

	UFUNCTION()
	void HandleLaneGlowInput(int32 LaneIndex, bool bPressed);

	UFUNCTION()
	void HandleScoreChanged(FRhythmScoreState ScoreState);

	UFUNCTION()
	void HandleMusicFinished();
	void HandleScoreSubmissionCompleted(bool bSuccess, const FString& Message);
	void HandleAroundPlayerLeaderboardCompleted(bool bSuccess, const FRhythmLeaderboardResult& Result);

	UFUNCTION()
	void HandleReturnToLobbyClicked();

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandlePauseLobbyClicked();

	void SetPauseMenuVisible(bool bVisible);
	void BindRuntimeManagers();
	void RefreshScoreText(const FRhythmScoreState& ScoreState);
	void UpdateJudgementAnimation();
	void UpdateResultAnimation();

	void BuildWidgetLayout();
	void RefreshLayout(float MusicTimeSeconds);

	struct FNoteVisual
	{
		FRhythmNoteData Data;
		TObjectPtr<UImage> HeadImage;
		TObjectPtr<UImage> BodyImage;
		TObjectPtr<UImage> TailImage;
		TObjectPtr<UImage> HoldGlowImage;
		float SpawnTimeSeconds = 0.0f;
		bool bHolding = false;
	};

	struct FTimedEffectVisual
	{
		TObjectPtr<UImage> Image;
		float HideWorldTime = 0.0f;
	};

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> LaneCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UImage> JudgementLine;

	UPROPERTY(Transient)
	TObjectPtr<UImage> JudgementFeedback;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> JudgementHitCountText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> LaneImages;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> LaneGlowImages;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> LaneKeyLabels;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ComboText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AccuracyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayTimeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StartCountdownText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ResultBackground;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultSummaryText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultPerfectText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultGreatText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultGoodText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultMissText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultLeaderboardText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResultLobbyButton;

	UPROPERTY(Transient)
	TObjectPtr<UImage> PauseBackground;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PauseTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> PauseRestartButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> PauseLobbyButton;

	UPROPERTY(Transient)
	TObjectPtr<ARhythmNoteSpawner> Spawner;

	UPROPERTY(Transient)
	TObjectPtr<ARhythmConductor> Conductor;

	UPROPERTY(Transient)
	TObjectPtr<ARhythmJudgementManager> JudgementManager;

	UPROPERTY(Transient)
	TObjectPtr<ARhythmScoreManager> ScoreManager;

	TArray<FNoteVisual> NoteVisuals;
	TArray<FTimedEffectVisual> LongNoteEffects;
	float JudgementHideWorldTime = 0.0f;
	float JudgementAnimationStartWorldTime = 0.0f;
	float ResultAnimationStartRealTime = 0.0f;
	float NextTimelineDiagnosticTime = 0.0f;
	FRhythmScoreState FinalResultState;
	int32 FinalResultTotalNotes = 0;
	int32 CurrentHitStreak = 0;
	int64 NextNoteVisualId = 0;
	int32 LaneCount = 9;
	bool bShowingResults = false;
	bool bResultAnimationActive = false;
	bool bShowingPauseMenu = false;
	bool bCountdownWasVisible = false;
	bool bInitialLayoutReady = false;
};
