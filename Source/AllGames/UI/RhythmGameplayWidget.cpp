// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmGameplayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "../Core/RhythmPlayerController.h"
#include "../Core/RhythmGameInstance.h"
#include "../Core/RhythmAccountSubsystem.h"
#include "../Judgement/RhythmJudgementManager.h"
#include "../Notes/RhythmNoteSpawner.h"
#include "../Online/RhythmLeaderboardSubsystem.h"
#include "../Rhythm/RhythmConductor.h"
#include "../Scoring/RhythmScoreManager.h"

URhythmGameplayWidget::URhythmGameplayWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
	static ConstructorHelpers::FObjectFinder<UTexture2D> PerfectFinder(TEXT("/Game/Textures/T_Judgement_Perfect.T_Judgement_Perfect"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> GreatFinder(TEXT("/Game/Textures/T_Judgement_Great.T_Judgement_Great"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> GoodFinder(TEXT("/Game/Textures/T_Judgement_Good.T_Judgement_Good"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MissFinder(TEXT("/Game/Textures/T_Judgement_Miss.T_Judgement_Miss"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> LaneGlowFinder(TEXT("/Game/Textures/T_LaneGlow.T_LaneGlow"));
	PerfectJudgementImage = PerfectFinder.Object;
	GreatJudgementImage = GreatFinder.Object;
	GoodJudgementImage = GoodFinder.Object;
	MissJudgementImage = MissFinder.Object;
	LaneGlowImage = LaneGlowFinder.Object;
}

TSharedRef<SWidget> URhythmGameplayWidget::RebuildWidget()
{
	if (const ARhythmPlayerController* Controller = Cast<ARhythmPlayerController>(GetOwningPlayer()))
	{
		LaneCount = Controller->GetActiveLaneCount();
	}
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetLayout();
	}

	return Super::RebuildWidget();
}

void URhythmGameplayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (URhythmLeaderboardSubsystem* Leaderboards = GetGameInstance()
		? GetGameInstance()->GetSubsystem<URhythmLeaderboardSubsystem>() : nullptr)
	{
		Leaderboards->OnScoreSubmissionCompleted.AddUObject(
			this, &ThisClass::HandleScoreSubmissionCompleted);
		Leaderboards->OnAroundPlayerLeaderboardCompleted.AddUObject(
			this, &ThisClass::HandleAroundPlayerLeaderboardCompleted);
	}

	if (const ARhythmPlayerController* Controller = Cast<ARhythmPlayerController>(GetOwningPlayer()))
	{
		LaneCount = Controller->GetActiveLaneCount();
	}
	if (ARhythmPlayerController* Controller = Cast<ARhythmPlayerController>(GetOwningPlayer()))
	{
		Controller->OnLaneInput.AddUniqueDynamic(this, &ThisClass::HandleLaneGlowInput);
	}

	for (TActorIterator<ARhythmNoteSpawner> It(GetWorld()); It; ++It)
	{
		Spawner = *It;
		break;
	}

	if (Spawner)
	{
		Conductor = Spawner->GetConductor();
		Spawner->OnNoteSpawned.AddDynamic(this, &ThisClass::HandleNoteSpawned);
		Spawner->OnTimelineUpdated.AddDynamic(this, &ThisClass::HandleTimelineUpdated);
		if (Conductor)
		{
			Conductor->OnMusicFinished.AddUniqueDynamic(this, &ThisClass::HandleMusicFinished);
		}
	}

	BindRuntimeManagers();

	// NativeConstruct runs as the widget is added to the viewport, before Slate is guaranteed to
	// have painted the lane screen once. Defer the countdown by one game-thread tick so the player
	// sees the actual gameplay layout before 3, 2, 1, GO begins.
	if (Conductor)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (Conductor)
				{
					Conductor->StartGameplayCountdown();
				}
			}));
	}

	UE_LOG(LogTemp, Log, TEXT("Rhythm gameplay WBP ready: %d lanes."), LaneCount);
}

void URhythmGameplayWidget::NativeDestruct()
{
	if (URhythmLeaderboardSubsystem* Leaderboards = GetGameInstance()
		? GetGameInstance()->GetSubsystem<URhythmLeaderboardSubsystem>() : nullptr)
	{
		Leaderboards->OnScoreSubmissionCompleted.RemoveAll(this);
		Leaderboards->OnAroundPlayerLeaderboardCompleted.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void URhythmGameplayWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateResultAnimation();
	if (!JudgementManager || !ScoreManager)
	{
		BindRuntimeManagers();
	}
	if (!bInitialLayoutReady || (Conductor && Conductor->IsStartCountdownActive()))
	{
		RefreshLayout(Conductor ? Conductor->GetMusicTimeSeconds() : 0.0f);
	}
	if (!StartCountdownText || !Conductor)
	{
		return;
	}

	if (Conductor->IsStartCountdownActive())
	{
		const int32 Count = FMath::Max(1, FMath::CeilToInt(Conductor->GetStartCountdownSecondsRemaining()));
		StartCountdownText->SetText(FText::AsNumber(Count));
		StartCountdownText->SetVisibility(ESlateVisibility::HitTestInvisible);
		bCountdownWasVisible = true;
	}
	else if (bCountdownWasVisible)
	{
		StartCountdownText->SetText(FText::FromString(TEXT("GO!")));
		StartCountdownText->SetVisibility(ESlateVisibility::HitTestInvisible);
		bCountdownWasVisible = false;
		FTimerHandle HideCountdownHandle;
		GetWorld()->GetTimerManager().SetTimer(
			HideCountdownHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (StartCountdownText)
				{
					StartCountdownText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}),
			0.45f,
			false);
	}
}

void URhythmGameplayWidget::BindRuntimeManagers()
{
	if (!GetWorld())
	{
		return;
	}

	if (!JudgementManager)
	{
		for (TActorIterator<ARhythmJudgementManager> It(GetWorld()); It; ++It)
		{
			JudgementManager = *It;
			JudgementManager->OnNoteJudged.AddUniqueDynamic(this, &ThisClass::HandleNoteJudged);
			JudgementManager->OnLongNoteStateChanged.AddUniqueDynamic(
				this, &ThisClass::HandleLongNoteStateChanged);
			UE_LOG(LogTemp, Log, TEXT("Rhythm gameplay UI bound to judgement manager."));
			break;
		}
	}

	if (!ScoreManager)
	{
		for (TActorIterator<ARhythmScoreManager> It(GetWorld()); It; ++It)
		{
			ScoreManager = *It;
			ScoreManager->OnScoreChanged.AddUniqueDynamic(this, &ThisClass::HandleScoreChanged);
			RefreshScoreText(ScoreManager->GetScoreState());
			UE_LOG(LogTemp, Log, TEXT("Rhythm gameplay UI bound to score manager."));
			break;
		}
	}
}

void URhythmGameplayWidget::HandleTimelineUpdated(const float MusicTimeSeconds)
{
	UpdateJudgementAnimation();
	if (JudgementFeedback && JudgementFeedback->GetVisibility() == ESlateVisibility::HitTestInvisible
		&& GetWorld() && GetWorld()->GetTimeSeconds() >= JudgementHideWorldTime)
	{
		JudgementFeedback->SetVisibility(ESlateVisibility::Collapsed);
		if (JudgementHitCountText)
		{
			JudgementHitCountText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	RefreshLayout(MusicTimeSeconds);
	if (PlayTimeText && Conductor)
	{
		const int32 CurrentTotalSeconds = FMath::Max(0, FMath::FloorToInt(MusicTimeSeconds));
		PlayTimeText->SetText(FText::FromString(FString::Printf(
			TEXT("TIME  %d:%02d"),
			CurrentTotalSeconds / 60,
			CurrentTotalSeconds % 60)));
	}
	if (GetWorld())
	{
		for (int32 Index = LongNoteEffects.Num() - 1; Index >= 0; --Index)
		{
			if (GetWorld()->GetTimeSeconds() >= LongNoteEffects[Index].HideWorldTime)
			{
				if (LongNoteEffects[Index].Image)
				{
					LongNoteEffects[Index].Image->RemoveFromParent();
				}
				LongNoteEffects.RemoveAt(Index);
			}
		}
	}

	if (MusicTimeSeconds >= NextTimelineDiagnosticTime)
	{
		UE_LOG(LogTemp, Log, TEXT("Rhythm UI timeline: music %.3f, active visuals %d"),
			MusicTimeSeconds, NoteVisuals.Num());
		NextTimelineDiagnosticTime = MusicTimeSeconds + 5.0f;
	}
}

void URhythmGameplayWidget::BuildWidgetLayout()
{
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
	Background->SetColorAndOpacity(FLinearColor(0.015f, 0.02f, 0.04f, 0.92f));
	if (BackgroundImage) Background->SetBrushFromTexture(BackgroundImage);
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	LaneCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LaneCanvas"));
	UCanvasPanelSlot* LaneCanvasSlot = RootCanvas->AddChildToCanvas(LaneCanvas);
	const float LaneAreaSideMargin = (1.0f - LaneAreaScreenWidthFraction) * 0.5f;
	LaneCanvasSlot->SetAnchors(FAnchors(LaneAreaSideMargin, 0.0f, 1.0f - LaneAreaSideMargin, 1.0f));
	LaneCanvasSlot->SetOffsets(FMargin(0.0f, 40.0f, 0.0f, -80.0f));

	LaneImages.Reset();
	LaneGlowImages.Reset();
	LaneKeyLabels.Reset();
	const TArray<FString> KeyNames = LaneCount == 5
		? TArray<FString>{ TEXT("D"), TEXT("F"), TEXT("SPACE"), TEXT("J"), TEXT("K") }
		: TArray<FString>{ TEXT("A"), TEXT("S"), TEXT("D"), TEXT("F"), TEXT("SPACE"), TEXT("J"), TEXT("K"), TEXT("L"), TEXT(";") };
	for (int32 LaneIndex = 0; LaneIndex < LaneCount; ++LaneIndex)
	{
		UImage* Lane = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("Lane%d"), LaneIndex + 1));
		Lane->SetColorAndOpacity(LaneIndex % 2 == 0 ? FLinearColor(0.08f, 0.1f, 0.16f, 0.92f) : FLinearColor(0.04f, 0.06f, 0.11f, 0.92f));
		if (LaneBackgroundImage) Lane->SetBrushFromTexture(LaneBackgroundImage);
		Lane->SetVisibility(ESlateVisibility::Collapsed);
		LaneCanvas->AddChildToCanvas(Lane);
		LaneImages.Add(Lane);

		UImage* LaneGlow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("LaneGlow%d"), LaneIndex + 1));
		LaneGlow->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, LaneGlowOpacity));
		if (LaneGlowImage)
		{
			LaneGlow->SetBrushFromTexture(LaneGlowImage);
		}
		LaneGlow->SetVisibility(ESlateVisibility::Collapsed);
		LaneCanvas->AddChildToCanvas(LaneGlow);
		LaneGlowImages.Add(LaneGlow);

		UTextBlock* KeyLabel = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), *FString::Printf(TEXT("LaneKeyLabel%d"), LaneIndex + 1));
		KeyLabel->SetText(FText::FromString(KeyNames[LaneIndex]));
		KeyLabel->SetJustification(ETextJustify::Center);
		KeyLabel->SetColorAndOpacity(FSlateColor(LaneKeyLabelColor));
		KeyLabel->SetShadowOffset(FVector2D(2.0f, 2.0f));
		KeyLabel->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		FSlateFontInfo KeyFont = KeyLabel->GetFont();
		KeyFont.Size = LaneKeyLabelFontSize;
		KeyLabel->SetFont(KeyFont);
		KeyLabel->SetVisibility(ESlateVisibility::Collapsed);
		LaneCanvas->AddChildToCanvas(KeyLabel);
		LaneKeyLabels.Add(KeyLabel);
	}

	JudgementLine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("JudgementLine"));
	JudgementLine->SetColorAndOpacity(FLinearColor::White);
	if (JudgementLineImage) JudgementLine->SetBrushFromTexture(JudgementLineImage);
	JudgementLine->SetVisibility(ESlateVisibility::Collapsed);
	LaneCanvas->AddChildToCanvas(JudgementLine);

	JudgementFeedback = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("JudgementFeedback"));
	JudgementFeedback->SetVisibility(ESlateVisibility::Collapsed);
	RootCanvas->AddChildToCanvas(JudgementFeedback);

	JudgementHitCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JudgementHitCountText"));
	JudgementHitCountText->SetJustification(ETextJustify::Center);
	JudgementHitCountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	JudgementHitCountText->SetShadowOffset(FVector2D(3.0f, 3.0f));
	JudgementHitCountText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	FSlateFontInfo HitCountFont = JudgementHitCountText->GetFont();
	HitCountFont.Size = JudgementHitCountFontSize;
	JudgementHitCountText->SetFont(HitCountFont);
	JudgementHitCountText->SetVisibility(ESlateVisibility::Collapsed);
	RootCanvas->AddChildToCanvas(JudgementHitCountText);

	ScoreText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ScoreText"));
	ComboText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ComboText"));
	AccuracyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AccuracyText"));
	PlayTimeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayTimeText"));
	for (UTextBlock* TextBlock : { ScoreText.Get(), ComboText.Get(), AccuracyText.Get(), PlayTimeText.Get() })
	{
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		TextBlock->SetShadowOffset(FVector2D(2.0f, 2.0f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
		TextBlock->SetVisibility(ESlateVisibility::Collapsed);
		RootCanvas->AddChildToCanvas(TextBlock);
	}
	FSlateFontInfo ScoreFont = ScoreText->GetFont();
	ScoreFont.Size = 30;
	ScoreText->SetFont(ScoreFont);
	AccuracyText->SetFont(ScoreFont);
	PlayTimeText->SetFont(ScoreFont);
	PlayTimeText->SetText(FText::FromString(TEXT("TIME  0:00")));
	FSlateFontInfo ComboFont = ComboText->GetFont();
	ComboFont.Size = 42;
	ComboText->SetFont(ComboFont);
	RefreshScoreText(FRhythmScoreState());

	StartCountdownText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("StartCountdownText"));
	StartCountdownText->SetJustification(ETextJustify::Center);
	StartCountdownText->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.9f, 1.0f)));
	StartCountdownText->SetShadowOffset(FVector2D(5.0f, 5.0f));
	StartCountdownText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
	FSlateFontInfo CountdownFont = StartCountdownText->GetFont();
	CountdownFont.Size = 128;
	StartCountdownText->SetFont(CountdownFont);
	StartCountdownText->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* CountdownSlot = RootCanvas->AddChildToCanvas(StartCountdownText))
	{
		CountdownSlot->SetAnchors(FAnchors(0.5f, 0.42f));
		CountdownSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CountdownSlot->SetSize(FVector2D(700.0f, 220.0f));
		CountdownSlot->SetZOrder(90);
	}

	ResultBackground = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ResultBackground"));
	ResultBackground->SetColorAndOpacity(FLinearColor(0.005f, 0.008f, 0.02f, 0.94f));
	ResultBackground->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* ResultBackgroundSlot = RootCanvas->AddChildToCanvas(ResultBackground))
	{
		ResultBackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		ResultBackgroundSlot->SetOffsets(FMargin(0.0f));
		ResultBackgroundSlot->SetZOrder(100);
	}

	ResultTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultTitleText"));
	ResultTitleText->SetText(FText::FromString(TEXT("RESULT")));
	ResultTitleText->SetJustification(ETextJustify::Center);
	ResultTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.9f, 1.0f)));
	ResultTitleText->SetShadowOffset(FVector2D(4.0f, 4.0f));
	FSlateFontInfo ResultTitleFont = ResultTitleText->GetFont();
	ResultTitleFont.Size = 72;
	ResultTitleText->SetFont(ResultTitleFont);
	ResultTitleText->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* ResultTitleSlot = RootCanvas->AddChildToCanvas(ResultTitleText))
	{
		ResultTitleSlot->SetAnchors(FAnchors(0.5f, 0.16f));
		ResultTitleSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		ResultTitleSlot->SetSize(FVector2D(900.0f, 110.0f));
		ResultTitleSlot->SetZOrder(101);
	}

	ResultSummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultSummaryText"));
	ResultSummaryText->SetJustification(ETextJustify::Center);
	ResultSummaryText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ResultSummaryText->SetShadowOffset(FVector2D(3.0f, 3.0f));
	FSlateFontInfo ResultSummaryFont = ResultSummaryText->GetFont();
	ResultSummaryFont.Size = 36;
	ResultSummaryText->SetFont(ResultSummaryFont);
	ResultSummaryText->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* ResultSummarySlot = RootCanvas->AddChildToCanvas(ResultSummaryText))
	{
		ResultSummarySlot->SetAnchors(FAnchors(0.5f, 0.30f));
		ResultSummarySlot->SetAlignment(FVector2D(0.5f, 0.0f));
		ResultSummarySlot->SetSize(FVector2D(1000.0f, 220.0f));
		ResultSummarySlot->SetZOrder(101);
	}

	auto AddResultJudgement = [this, RootCanvas](const TCHAR* Name, const FLinearColor& Color, const float AnchorY)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(3.0f, 3.0f));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = 38;
		Text->SetFont(Font);
		Text->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Text))
		{
			Slot->SetAnchors(FAnchors(0.5f, AnchorY));
			Slot->SetAlignment(FVector2D(0.5f, 0.0f));
			Slot->SetSize(FVector2D(800.0f, 65.0f));
			Slot->SetZOrder(101);
		}
		return Text;
	};
	ResultPerfectText = AddResultJudgement(TEXT("ResultPerfectText"), FLinearColor(0.25f, 0.85f, 1.0f), 0.52f);
	ResultGreatText = AddResultJudgement(TEXT("ResultGreatText"), FLinearColor(1.0f, 0.82f, 0.12f), 0.59f);
	ResultGoodText = AddResultJudgement(TEXT("ResultGoodText"), FLinearColor(0.25f, 1.0f, 0.38f), 0.66f);
	ResultMissText = AddResultJudgement(TEXT("ResultMissText"), FLinearColor(1.0f, 0.16f, 0.16f), 0.73f);

	ResultLeaderboardText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("ResultLeaderboardText"));
	ResultLeaderboardText->SetText(FText::FromString(TEXT("온라인 기록 대기 중")));
	ResultLeaderboardText->SetJustification(ETextJustify::Center);
	ResultLeaderboardText->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.9f, 1.0f)));
	FSlateFontInfo LeaderboardFont = ResultLeaderboardText->GetFont();
	LeaderboardFont.Size = 25;
	ResultLeaderboardText->SetFont(LeaderboardFont);
	ResultLeaderboardText->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* LeaderboardSlot = RootCanvas->AddChildToCanvas(ResultLeaderboardText))
	{
		LeaderboardSlot->SetAnchors(FAnchors(0.5f, 0.79f));
		LeaderboardSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		LeaderboardSlot->SetSize(FVector2D(1000.0f, 55.0f));
		LeaderboardSlot->SetZOrder(101);
	}

	ResultLobbyButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResultLobbyButton"));
	UTextBlock* ResultLobbyButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("ResultLobbyButtonText"));
	ResultLobbyButtonText->SetText(FText::FromString(TEXT("LOBBY")));
	ResultLobbyButtonText->SetJustification(ETextJustify::Center);
	ResultLobbyButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo ResultButtonFont = ResultLobbyButtonText->GetFont();
	ResultButtonFont.Size = 34;
	ResultLobbyButtonText->SetFont(ResultButtonFont);
	ResultLobbyButton->AddChild(ResultLobbyButtonText);
	ResultLobbyButton->OnClicked.AddDynamic(this, &ThisClass::HandleReturnToLobbyClicked);
	ResultLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* ResultButtonSlot = RootCanvas->AddChildToCanvas(ResultLobbyButton))
	{
		ResultButtonSlot->SetAnchors(FAnchors(0.5f, 0.86f));
		ResultButtonSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		ResultButtonSlot->SetSize(FVector2D(360.0f, 80.0f));
		ResultButtonSlot->SetZOrder(101);
	}

	PauseBackground = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PauseBackground"));
	PauseBackground->SetColorAndOpacity(FLinearColor(0.005f, 0.008f, 0.02f, 0.88f));
	PauseBackground->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* PauseBackgroundSlot = RootCanvas->AddChildToCanvas(PauseBackground))
	{
		PauseBackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		PauseBackgroundSlot->SetOffsets(FMargin(0.0f));
		PauseBackgroundSlot->SetZOrder(110);
	}

	PauseTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseTitleText"));
	PauseTitleText->SetText(FText::FromString(TEXT("PAUSED")));
	PauseTitleText->SetJustification(ETextJustify::Center);
	PauseTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.9f, 1.0f)));
	FSlateFontInfo PauseTitleFont = PauseTitleText->GetFont();
	PauseTitleFont.Size = 72;
	PauseTitleText->SetFont(PauseTitleFont);
	PauseTitleText->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* PauseTitleSlot = RootCanvas->AddChildToCanvas(PauseTitleText))
	{
		PauseTitleSlot->SetAnchors(FAnchors(0.5f, 0.30f));
		PauseTitleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PauseTitleSlot->SetSize(FVector2D(700.0f, 110.0f));
		PauseTitleSlot->SetZOrder(111);
	}

	auto AddPauseButton = [this, RootCanvas](const TCHAR* Name, const TCHAR* Label, const float AnchorY)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), *FString::Printf(TEXT("%sText"), Name));
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetJustification(ETextJustify::Center);
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo Font = LabelText->GetFont();
		Font.Size = 36;
		LabelText->SetFont(Font);
		Button->AddChild(LabelText);
		Button->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* PauseButtonSlot = RootCanvas->AddChildToCanvas(Button))
		{
			PauseButtonSlot->SetAnchors(FAnchors(0.5f, AnchorY));
			PauseButtonSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PauseButtonSlot->SetSize(FVector2D(420.0f, 82.0f));
			PauseButtonSlot->SetZOrder(111);
		}
		return Button;
	};

	PauseRestartButton = AddPauseButton(TEXT("PauseRestartButton"), TEXT("RESTART"), 0.48f);
	PauseRestartButton->OnClicked.AddDynamic(this, &ThisClass::HandleRestartClicked);
	PauseLobbyButton = AddPauseButton(TEXT("PauseLobbyButton"), TEXT("LOBBY"), 0.60f);
	PauseLobbyButton->OnClicked.AddDynamic(this, &ThisClass::HandlePauseLobbyClicked);
}

void URhythmGameplayWidget::HandleNoteSpawned(const FRhythmNoteData NoteData)
{
	if (!Conductor && Spawner)
	{
		Conductor = Spawner->GetConductor();
	}

	if (!LaneCanvas || NoteData.LaneIndex < 0 || NoteData.LaneIndex >= LaneCount)
	{
		return;
	}

	// Active-count-based names repeat throughout a song and can collide with previously constructed
	// UObject widgets. A monotonic serial guarantees one distinct image object per chart note.
	const int64 NoteVisualId = NextNoteVisualId++;
	UImage* Note = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), *FString::Printf(TEXT("Note_%lld_Lane_%d"), NoteVisualId, NoteData.LaneIndex));
	Note->SetColorAndOpacity(FLinearColor::MakeFromHSV8(static_cast<uint8>(NoteData.LaneIndex * 24), 190, 255));
	if (NoteData.IsLongNote() && LongNoteHeadImage) Note->SetBrushFromTexture(LongNoteHeadImage);
	else if (NoteImage) Note->SetBrushFromTexture(NoteImage);
	LaneCanvas->AddChildToCanvas(Note);

	FNoteVisual& Visual = NoteVisuals.AddDefaulted_GetRef();
	Visual.Data = NoteData;
	Visual.HeadImage = Note;
	if (NoteData.IsLongNote())
	{
		Visual.BodyImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), *FString::Printf(TEXT("LongBody_%lld"), NoteVisualId));
		Visual.BodyImage->SetColorAndOpacity(FLinearColor::MakeFromHSV8(
			static_cast<uint8>(NoteData.LaneIndex * 24), 150, 220));
		if (LongNoteBodyImage) Visual.BodyImage->SetBrushFromTexture(LongNoteBodyImage);
		LaneCanvas->AddChildToCanvas(Visual.BodyImage);

		Visual.TailImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), *FString::Printf(TEXT("LongTail_%lld"), NoteVisualId));
		Visual.TailImage->SetColorAndOpacity(FLinearColor::MakeFromHSV8(
			static_cast<uint8>(NoteData.LaneIndex * 24), 190, 255));
		if (LongNoteTailImage) Visual.TailImage->SetBrushFromTexture(LongNoteTailImage);
		LaneCanvas->AddChildToCanvas(Visual.TailImage);

		Visual.HoldGlowImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), *FString::Printf(TEXT("LongHoldGlow_%lld"), NoteVisualId));
		Visual.HoldGlowImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.85f));
		if (LongNoteHoldGlowImage) Visual.HoldGlowImage->SetBrushFromTexture(LongNoteHoldGlowImage);
		Visual.HoldGlowImage->SetVisibility(ESlateVisibility::Collapsed);
		LaneCanvas->AddChildToCanvas(Visual.HoldGlowImage);
	}
	Visual.SpawnTimeSeconds = NoteData.TargetTimeSeconds - (Spawner ? Spawner->GetSpawnLeadTimeSeconds() : 2.0f);
	// OnTimelineUpdated is broadcast before newly due notes are spawned. Position this note now so it
	// cannot flash for one frame at the canvas default position in the upper-left corner.
	const float CurrentMusicTime = Conductor ? Conductor->GetMusicTimeSeconds() : Visual.SpawnTimeSeconds;
	RefreshLayout(CurrentMusicTime);
	UE_LOG(LogTemp, Log, TEXT("Created UI %s note: lane %d, target %.2f, duration %.2f"),
		NoteData.IsLongNote() ? TEXT("long") : TEXT("tap"),
		NoteData.LaneIndex + 1, NoteData.TargetTimeSeconds, NoteData.DurationSeconds);
}

void URhythmGameplayWidget::HandleLongNoteStateChanged(
	const FRhythmNoteData NoteData, const ERhythmLongNoteState State)
{
	for (FNoteVisual& Visual : NoteVisuals)
	{
		if (Visual.Data.LaneIndex == NoteData.LaneIndex
			&& FMath::IsNearlyEqual(Visual.Data.TargetTimeSeconds, NoteData.TargetTimeSeconds))
		{
			Visual.bHolding = State == ERhythmLongNoteState::Started;
			if (Visual.HoldGlowImage)
			{
				Visual.HoldGlowImage->SetVisibility(
					State == ERhythmLongNoteState::Started
						? ESlateVisibility::HitTestInvisible
						: ESlateVisibility::Collapsed);
			}
			break;
		}
	}

	if (State == ERhythmLongNoteState::Started || !LaneCanvas || !GetWorld())
	{
		return;
	}

	UTexture2D* EffectTexture = State == ERhythmLongNoteState::Completed
		? LongNoteCompleteEffectImage
		: LongNoteBreakEffectImage;
	UImage* Effect = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), *FString::Printf(TEXT("LongEffect_%lld"), NextNoteVisualId++));
	Effect->SetColorAndOpacity(State == ERhythmLongNoteState::Completed
		? FLinearColor(0.25f, 0.95f, 1.0f, 0.95f)
		: FLinearColor(1.0f, 0.08f, 0.08f, 0.95f));
	if (EffectTexture) Effect->SetBrushFromTexture(EffectTexture);
	LaneCanvas->AddChildToCanvas(Effect);
	if (UCanvasPanelSlot* EffectSlot = Cast<UCanvasPanelSlot>(Effect->Slot))
	{
		const FVector2D Size = LaneCanvas->GetCachedGeometry().GetLocalSize();
		const float LaneWidth = Size.X / FMath::Max(LaneCount, 1);
		const float EffectSize = FMath::Min(LaneWidth * 1.25f, 180.0f);
		EffectSlot->SetPosition(FVector2D(
			NoteData.LaneIndex * LaneWidth + (LaneWidth - EffectSize) * 0.5f,
			Size.Y * JudgementLineVerticalPosition - EffectSize * 0.5f));
		EffectSlot->SetSize(FVector2D(EffectSize));
		EffectSlot->SetZOrder(8);
	}
	FTimedEffectVisual& TimedEffect = LongNoteEffects.AddDefaulted_GetRef();
	TimedEffect.Image = Effect;
	TimedEffect.HideWorldTime = GetWorld()->GetTimeSeconds() + LongNoteEffectDisplaySeconds;
}

void URhythmGameplayWidget::HandleNoteJudged(
	const FRhythmNoteData NoteData, const ERhythmJudgement Judgement, const float TimingErrorSeconds)
{
	UTexture2D* Texture = nullptr;
	switch (Judgement)
	{
	case ERhythmJudgement::Perfect: Texture = PerfectJudgementImage; break;
	case ERhythmJudgement::Great: Texture = GreatJudgementImage; break;
	case ERhythmJudgement::Good: Texture = GoodJudgementImage; break;
	case ERhythmJudgement::Miss: Texture = MissJudgementImage; break;
	}

	if (JudgementFeedback && Texture && GetWorld())
	{
		if (Judgement == ERhythmJudgement::Miss)
		{
			CurrentHitStreak = 0;
		}
		else
		{
			++CurrentHitStreak;
		}

		JudgementFeedback->SetBrushFromTexture(Texture, true);
		JudgementFeedback->SetColorAndOpacity(FLinearColor::White);
		JudgementFeedback->SetRenderScale(FVector2D(JudgementPopStartScale));
		JudgementFeedback->SetVisibility(ESlateVisibility::HitTestInvisible);
		JudgementAnimationStartWorldTime = GetWorld()->GetTimeSeconds();
		JudgementHideWorldTime = GetWorld()->GetTimeSeconds() + JudgementDisplaySeconds;

		if (JudgementHitCountText)
		{
			if (CurrentHitStreak > 0)
			{
				JudgementHitCountText->SetText(FText::FromString(FString::Printf(TEXT("%d HIT"), CurrentHitStreak)));
				JudgementHitCountText->SetRenderScale(FVector2D(JudgementPopStartScale));
				JudgementHitCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				JudgementHitCountText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	bool bRemovedVisual = false;
	for (int32 Index = NoteVisuals.Num() - 1; Index >= 0; --Index)
	{
		const FNoteVisual& Visual = NoteVisuals[Index];
		if (Visual.Data.LaneIndex == NoteData.LaneIndex
			&& FMath::IsNearlyEqual(Visual.Data.TargetTimeSeconds, NoteData.TargetTimeSeconds))
		{
			for (UImage* Image : {
				Visual.HeadImage.Get(), Visual.BodyImage.Get(), Visual.TailImage.Get(), Visual.HoldGlowImage.Get() })
			{
				if (Image)
				{
					Image->SetVisibility(ESlateVisibility::Collapsed);
					Image->RemoveFromParent();
				}
			}
			NoteVisuals.RemoveAt(Index);
			bRemovedVisual = true;
			break;
		}
	}
	if (bRemovedVisual)
	{
		UE_LOG(LogTemp, Log, TEXT("UI removed judged note: lane %d, target %.3f, remaining visuals %d"),
			NoteData.LaneIndex + 1, NoteData.TargetTimeSeconds, NoteVisuals.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UI failed to remove judged note: lane %d, target %.3f, remaining visuals %d"),
			NoteData.LaneIndex + 1, NoteData.TargetTimeSeconds, NoteVisuals.Num());
	}
}

void URhythmGameplayWidget::HandleLaneGlowInput(const int32 LaneIndex, const bool bPressed)
{
	if (!LaneGlowImage || !LaneGlowImages.IsValidIndex(LaneIndex) || !LaneGlowImages[LaneIndex])
	{
		return;
	}

	LaneGlowImages[LaneIndex]->SetVisibility(
		bPressed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void URhythmGameplayWidget::HandleScoreChanged(const FRhythmScoreState ScoreState)
{
	RefreshScoreText(ScoreState);
}

void URhythmGameplayWidget::HandleMusicFinished()
{
	if (!ResultBackground || !ResultTitleText || !ResultSummaryText || !ResultPerfectText
		|| !ResultGreatText || !ResultGoodText || !ResultMissText || !ResultLobbyButton)
	{
		return;
	}

	FinalResultState = ScoreManager ? ScoreManager->GetScoreState() : FRhythmScoreState();
	if (ResultLeaderboardText)
	{
		ResultLeaderboardText->SetText(FText::FromString(TEXT("온라인 기록 등록 중...")));
		ResultLeaderboardText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (const URhythmGameInstance* RhythmGameInstance = Cast<URhythmGameInstance>(GetGameInstance()))
	{
		if (URhythmLeaderboardSubsystem* Leaderboards =
			GetGameInstance()->GetSubsystem<URhythmLeaderboardSubsystem>())
		{
			Leaderboards->SubmitScore(RhythmGameInstance->GetSelectedSong(), FinalResultState);
		}
	}
	FinalResultTotalNotes = FinalResultState.PerfectCount + FinalResultState.GreatCount
		+ FinalResultState.GoodCount + FinalResultState.MissCount;
	ResultAnimationStartRealTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0f;
	bResultAnimationActive = true;
	ResultSummaryText->SetText(FText::FromString(TEXT("SCORE  0000000000\n\nMAX COMBO  0 / 0\nACCURACY  0.00%")));
	ResultPerfectText->SetText(FText::FromString(TEXT("PERFECT  0")));
	ResultGreatText->SetText(FText::FromString(TEXT("GREAT  0")));
	ResultGoodText->SetText(FText::FromString(TEXT("GOOD  0")));
	ResultMissText->SetText(FText::FromString(TEXT("MISS  0")));

	ResultBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultTitleText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultSummaryText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultPerfectText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultGreatText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultGoodText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultMissText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultLobbyButton->SetVisibility(ESlateVisibility::Collapsed);
	bShowingResults = true;
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
	}
	if (JudgementFeedback) JudgementFeedback->SetVisibility(ESlateVisibility::Collapsed);
	if (JudgementHitCountText) JudgementHitCountText->SetVisibility(ESlateVisibility::Collapsed);
	UE_LOG(LogTemp, Log, TEXT("Rhythm result displayed: score %lld, max combo %d, accuracy %.2f%%, notes %d"),
		FinalResultState.Score, FinalResultState.MaxCombo, FinalResultState.AccuracyPercent, FinalResultTotalNotes);
}

void URhythmGameplayWidget::HandleScoreSubmissionCompleted(const bool bSuccess, const FString& Message)
{
	if (!ResultLeaderboardText || !bShowingResults)
	{
		return;
	}
	if (!bSuccess)
	{
		ResultLeaderboardText->SetText(FText::FromString(Message));
		ResultLeaderboardText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.35f, 0.35f)));
		return;
	}

	ResultLeaderboardText->SetText(FText::FromString(TEXT("현재 순위 확인 중...")));
	if (const URhythmGameInstance* Settings = Cast<URhythmGameInstance>(GetGameInstance()))
	{
		if (URhythmLeaderboardSubsystem* Leaderboards =
			GetGameInstance()->GetSubsystem<URhythmLeaderboardSubsystem>())
		{
			Leaderboards->RequestLeaderboardAroundPlayer(Settings->GetSelectedSong(), 9);
		}
	}
}

void URhythmGameplayWidget::HandleAroundPlayerLeaderboardCompleted(
	const bool bSuccess, const FRhythmLeaderboardResult& Result)
{
	if (!ResultLeaderboardText || !bShowingResults)
	{
		return;
	}
	if (!bSuccess)
	{
		ResultLeaderboardText->SetText(FText::FromString(TEXT("순위 조회에 실패했습니다.")));
		ResultLeaderboardText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.35f, 0.35f)));
		return;
	}

	const URhythmAccountSubsystem* Accounts = GetGameInstance()
		? GetGameInstance()->GetSubsystem<URhythmAccountSubsystem>() : nullptr;
	const FString PlayerId = Accounts ? Accounts->GetPlayerId() : FString();
	const FRhythmLeaderboardEntry* PlayerEntry = Result.Entries.FindByPredicate(
		[&PlayerId](const FRhythmLeaderboardEntry& Entry) { return Entry.PlayerId == PlayerId; });
	ResultLeaderboardText->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.9f, 1.0f)));
	ResultLeaderboardText->SetText(FText::FromString(PlayerEntry
		? FString::Printf(TEXT("온라인 순위  %d위    개인 최고  %d"), PlayerEntry->Rank, PlayerEntry->Score)
		: TEXT("등록 완료 · 순위 집계 대기 중")));
}

void URhythmGameplayWidget::HandleReturnToLobbyClicked()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetShowMouseCursor(false);
	}
	UGameplayStatics::OpenLevel(this, TEXT("LobbyMap"));
}

void URhythmGameplayWidget::TogglePauseMenu()
{
	if (!bShowingResults)
	{
		SetPauseMenuVisible(!bShowingPauseMenu);
	}
}

void URhythmGameplayWidget::SetPauseMenuVisible(const bool bVisible)
{
	if (!PauseBackground || !PauseTitleText || !PauseRestartButton || !PauseLobbyButton)
	{
		return;
	}

	bShowingPauseMenu = bVisible;
	PauseBackground->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	PauseTitleText->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	PauseRestartButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	PauseLobbyButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (Conductor)
	{
		Conductor->SetMusicPaused(bVisible);
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetPause(bVisible);
		PlayerController->SetShowMouseCursor(bVisible);
		if (bVisible)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
		}
		else
		{
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
			PlayerController->FlushPressedKeys();
		}
	}
}

void URhythmGameplayWidget::HandleRestartClicked()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetPause(false);
		PlayerController->SetShowMouseCursor(false);
	}
	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
	UGameplayStatics::OpenLevel(this, CurrentLevelName);
}

void URhythmGameplayWidget::HandlePauseLobbyClicked()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetPause(false);
		PlayerController->SetShowMouseCursor(false);
	}
	UGameplayStatics::OpenLevel(this, TEXT("LobbyMap"));
}

void URhythmGameplayWidget::RefreshScoreText(const FRhythmScoreState& ScoreState)
{
	if (ScoreText)
	{
		ScoreText->SetText(FText::FromString(FString::Printf(TEXT("SCORE  %010lld"), ScoreState.Score)));
	}
	if (ComboText)
	{
		ComboText->SetText(FText::FromString(FString::Printf(TEXT("%d COMBO"), ScoreState.Combo)));
	}
	if (AccuracyText)
	{
		AccuracyText->SetText(FText::FromString(FString::Printf(TEXT("ACCURACY  %.2f%%"), ScoreState.AccuracyPercent)));
	}
}

void URhythmGameplayWidget::UpdateResultAnimation()
{
	if (!bResultAnimationActive || !GetWorld() || !ResultSummaryText || !ResultPerfectText
		|| !ResultGreatText || !ResultGoodText || !ResultMissText)
	{
		return;
	}

	const float LinearAlpha = FMath::Clamp(
		(GetWorld()->GetRealTimeSeconds() - ResultAnimationStartRealTime)
			/ FMath::Max(ResultCountUpDuration, 0.01f),
		0.0f,
		1.0f);
	// Fast initial climb followed by a satisfying settle on the exact final value.
	const float CountAlpha = 1.0f - FMath::Pow(1.0f - LinearAlpha, 3.0f);
	const int64 AnimatedScore = FMath::RoundToInt64(static_cast<double>(FinalResultState.Score) * CountAlpha);
	const int32 AnimatedMaxCombo = FMath::RoundToInt(static_cast<float>(FinalResultState.MaxCombo) * CountAlpha);
	const int32 AnimatedTotalNotes = FMath::RoundToInt(static_cast<float>(FinalResultTotalNotes) * CountAlpha);
	const float AnimatedAccuracy = FinalResultState.AccuracyPercent * CountAlpha;

	ResultSummaryText->SetText(FText::FromString(FString::Printf(
		TEXT("SCORE  %010lld\n\nMAX COMBO  %d / %d\nACCURACY  %.2f%%"),
		AnimatedScore,
		AnimatedMaxCombo,
		AnimatedTotalNotes,
		AnimatedAccuracy)));
	ResultPerfectText->SetText(FText::FromString(FString::Printf(
		TEXT("PERFECT  %d"), FMath::RoundToInt(FinalResultState.PerfectCount * CountAlpha))));
	ResultGreatText->SetText(FText::FromString(FString::Printf(
		TEXT("GREAT  %d"), FMath::RoundToInt(FinalResultState.GreatCount * CountAlpha))));
	ResultGoodText->SetText(FText::FromString(FString::Printf(
		TEXT("GOOD  %d"), FMath::RoundToInt(FinalResultState.GoodCount * CountAlpha))));
	ResultMissText->SetText(FText::FromString(FString::Printf(
		TEXT("MISS  %d"), FMath::RoundToInt(FinalResultState.MissCount * CountAlpha))));

	if (LinearAlpha >= 1.0f)
	{
		bResultAnimationActive = false;
		if (ResultLobbyButton)
		{
			ResultLobbyButton->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void URhythmGameplayWidget::UpdateJudgementAnimation()
{
	if (!GetWorld() || !JudgementFeedback
		|| JudgementFeedback->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float PopProgress = FMath::Clamp(
		(Now - JudgementAnimationStartWorldTime) / FMath::Max(JudgementPopAnimationSeconds, 0.01f),
		0.0f, 1.0f);
	const float EasedProgress = 1.0f - FMath::Pow(1.0f - PopProgress, 3.0f);
	const float Scale = FMath::Lerp(JudgementPopStartScale, 1.0f, EasedProgress);
	const float FadeDuration = FMath::Min(0.15f, JudgementDisplaySeconds * 0.5f);
	const float Opacity = FMath::Clamp((JudgementHideWorldTime - Now) / FMath::Max(FadeDuration, 0.01f), 0.0f, 1.0f);

	JudgementFeedback->SetRenderScale(FVector2D(Scale));
	JudgementFeedback->SetRenderOpacity(Opacity);
	if (JudgementHitCountText && JudgementHitCountText->GetVisibility() != ESlateVisibility::Collapsed)
	{
		JudgementHitCountText->SetRenderScale(FVector2D(Scale));
		JudgementHitCountText->SetRenderOpacity(Opacity);
	}
}

void URhythmGameplayWidget::RefreshLayout(const float MusicTime)
{
	if (!LaneCanvas || LaneCount <= 0)
	{
		return;
	}

	const FVector2D Size = LaneCanvas->GetCachedGeometry().GetLocalSize();
	if (Size.X <= 1.0f || Size.Y <= 1.0f)
	{
		return;
	}

	if (!bInitialLayoutReady)
	{
		for (UImage* Lane : LaneImages)
		{
			if (Lane) Lane->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		for (UTextBlock* Label : LaneKeyLabels)
		{
			if (Label) Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (JudgementLine) JudgementLine->SetVisibility(ESlateVisibility::HitTestInvisible);
		for (UTextBlock* TextBlock : { ScoreText.Get(), ComboText.Get(), AccuracyText.Get(), PlayTimeText.Get() })
		{
			if (TextBlock) TextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		bInitialLayoutReady = true;
	}

	const float LaneWidth = Size.X / static_cast<float>(LaneCount);
	const float SpawnY = Size.Y * 0.05f;
	const float JudgementY = Size.Y * JudgementLineVerticalPosition;
	const float JudgementLineTop = JudgementY - JudgementLineThickness * 0.5f;
	const float GlowTopY = Size.Y * LaneGlowTopPosition;
	const float GlowBottomY = JudgementLineTop - LaneGlowBottomOffset;
	for (int32 LaneIndex = 0; LaneIndex < LaneImages.Num(); ++LaneIndex)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(LaneImages[LaneIndex]->Slot))
		{
			CanvasSlot->SetPosition(FVector2D(LaneIndex * LaneWidth + 1.0f, 0.0f));
			CanvasSlot->SetSize(FVector2D(LaneWidth - 2.0f, Size.Y));
		}
		if (LaneGlowImages.IsValidIndex(LaneIndex))
		{
			if (UCanvasPanelSlot* GlowSlot = Cast<UCanvasPanelSlot>(LaneGlowImages[LaneIndex]->Slot))
			{
				const float GlowWidth = FMath::Max(1.0f, LaneWidth - LaneGlowHorizontalPadding * 2.0f);
				GlowSlot->SetPosition(FVector2D(
					LaneIndex * LaneWidth + LaneGlowHorizontalPadding,
					GlowTopY));
				GlowSlot->SetSize(FVector2D(GlowWidth, FMath::Max(1.0f, GlowBottomY - GlowTopY)));
				GlowSlot->SetZOrder(2);
			}
		}
		if (LaneKeyLabels.IsValidIndex(LaneIndex))
		{
			if (UCanvasPanelSlot* LabelSlot = Cast<UCanvasPanelSlot>(LaneKeyLabels[LaneIndex]->Slot))
			{
				LabelSlot->SetPosition(FVector2D(
					LaneIndex * LaneWidth,
					JudgementY + JudgementLineThickness * 0.5f + LaneKeyLabelVerticalOffset));
				LabelSlot->SetSize(FVector2D(LaneWidth, LaneKeyLabelFontSize + 16.0f));
				LabelSlot->SetZOrder(5);
			}
		}
	}

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(JudgementLine->Slot))
	{
		CanvasSlot->SetPosition(FVector2D(0.0f, JudgementLineTop));
		CanvasSlot->SetSize(FVector2D(Size.X, JudgementLineThickness));
	}

	if (JudgementFeedback)
	{
		if (UCanvasPanelSlot* FeedbackSlot = Cast<UCanvasPanelSlot>(JudgementFeedback->Slot))
		{
			const float FeedbackY = FMath::Clamp(
				JudgementFeedbackVerticalPosition + JudgementFeedbackVerticalOffset, 0.0f, 1.0f);
			const FVector2D ScaledFeedbackMaxSize = JudgementFeedbackMaxSize * JudgementFeedbackScale;
			FeedbackSlot->SetAnchors(FAnchors(0.5f, FeedbackY));
			FeedbackSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			FeedbackSlot->SetPosition(FVector2D::ZeroVector);
			FVector2D FeedbackSize = ScaledFeedbackMaxSize;
			if (const UTexture2D* FeedbackTexture = Cast<UTexture2D>(JudgementFeedback->GetBrush().GetResourceObject()))
			{
				const FVector2D TextureSize(FeedbackTexture->GetSizeX(), FeedbackTexture->GetSizeY());
				if (TextureSize.X > 0.0f && TextureSize.Y > 0.0f)
				{
					const float UniformScale = FMath::Min(
						ScaledFeedbackMaxSize.X / TextureSize.X,
						ScaledFeedbackMaxSize.Y / TextureSize.Y);
					FeedbackSize = TextureSize * UniformScale;
				}
			}
			FeedbackSlot->SetSize(FeedbackSize);
			FeedbackSlot->SetZOrder(10);
		}
	}
	if (JudgementHitCountText)
	{
		if (UCanvasPanelSlot* CountSlot = Cast<UCanvasPanelSlot>(JudgementHitCountText->Slot))
		{
			const float FeedbackY = FMath::Clamp(
				JudgementFeedbackVerticalPosition + JudgementFeedbackVerticalOffset, 0.0f, 1.0f);
			CountSlot->SetAnchors(FAnchors(0.5f, FeedbackY));
			CountSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			CountSlot->SetPosition(FVector2D(0.0f, JudgementFeedbackMaxSize.Y * JudgementFeedbackScale * 0.34f));
			CountSlot->SetSize(FVector2D(280.0f, 52.0f));
			CountSlot->SetZOrder(11);
		}
	}

	const struct FScoreLayout
	{
		UTextBlock* Widget;
		float Y;
		FVector2D WidgetSize;
	} ScoreLayouts[] = {
		{ ScoreText.Get(), 40.0f, FVector2D(520.0f, 50.0f) },
		{ ComboText.Get(), 90.0f, FVector2D(520.0f, 70.0f) },
		{ AccuracyText.Get(), 155.0f, FVector2D(520.0f, 50.0f) },
		{ PlayTimeText.Get(), 200.0f, FVector2D(520.0f, 50.0f) },
	};
	for (const FScoreLayout& Layout : ScoreLayouts)
	{
		if (Layout.Widget)
		{
			if (UCanvasPanelSlot* TextSlot = Cast<UCanvasPanelSlot>(Layout.Widget->Slot))
			{
				TextSlot->SetAnchors(FAnchors(1.0f, 0.0f));
				TextSlot->SetAlignment(FVector2D(1.0f, 0.0f));
				TextSlot->SetPosition(FVector2D(-40.0f, Layout.Y));
				TextSlot->SetSize(Layout.WidgetSize);
				TextSlot->SetZOrder(20);
			}
		}
	}

	for (FNoteVisual& Visual : NoteVisuals)
	{
		const float TravelDuration = Visual.Data.TargetTimeSeconds - Visual.SpawnTimeSeconds;
		const float PixelsPerSecond = TravelDuration > 0.0f
			? (JudgementY - SpawnY) / TravelDuration
			: 0.0f;
		const float UnclampedHeadY = JudgementY - (Visual.Data.TargetTimeSeconds - MusicTime) * PixelsPerSecond;
		const float HeadY = Visual.bHolding ? JudgementY : UnclampedHeadY;
		if (UCanvasPanelSlot* CanvasSlot = Visual.HeadImage
			? Cast<UCanvasPanelSlot>(Visual.HeadImage->Slot) : nullptr)
		{
			CanvasSlot->SetPosition(FVector2D(
				Visual.Data.LaneIndex * LaneWidth + 6.0f, HeadY - LongNoteHeadHeight * 0.5f));
			CanvasSlot->SetSize(FVector2D(LaneWidth - 12.0f, LongNoteHeadHeight));
			CanvasSlot->SetZOrder(6);
			Visual.HeadImage->InvalidateLayoutAndVolatility();
		}
		if (Visual.Data.IsLongNote())
		{
			const float TailY = JudgementY - (Visual.Data.GetEndTimeSeconds() - MusicTime) * PixelsPerSecond;
			const float TopY = FMath::Min(HeadY, TailY);
			const float BottomY = FMath::Max(HeadY, TailY);
			if (UCanvasPanelSlot* BodySlot = Visual.BodyImage
				? Cast<UCanvasPanelSlot>(Visual.BodyImage->Slot) : nullptr)
			{
				BodySlot->SetPosition(FVector2D(Visual.Data.LaneIndex * LaneWidth + LaneWidth * 0.2f, TopY));
				BodySlot->SetSize(FVector2D(LaneWidth * 0.6f, FMath::Max(BottomY - TopY, 2.0f)));
				BodySlot->SetZOrder(4);
			}
			if (UCanvasPanelSlot* TailSlot = Visual.TailImage
				? Cast<UCanvasPanelSlot>(Visual.TailImage->Slot) : nullptr)
			{
				TailSlot->SetPosition(FVector2D(
					Visual.Data.LaneIndex * LaneWidth + 6.0f, TailY - LongNoteHeadHeight * 0.5f));
				TailSlot->SetSize(FVector2D(LaneWidth - 12.0f, LongNoteHeadHeight));
				TailSlot->SetZOrder(5);
			}
			if (UCanvasPanelSlot* GlowSlot = Visual.HoldGlowImage
				? Cast<UCanvasPanelSlot>(Visual.HoldGlowImage->Slot) : nullptr)
			{
				GlowSlot->SetPosition(FVector2D(
					Visual.Data.LaneIndex * LaneWidth, JudgementY - LaneWidth * 0.5f));
				GlowSlot->SetSize(FVector2D(LaneWidth));
				GlowSlot->SetZOrder(7);
			}
			if (Visual.BodyImage) Visual.BodyImage->InvalidateLayoutAndVolatility();
			if (Visual.TailImage) Visual.TailImage->InvalidateLayoutAndVolatility();
		}
	}
}
