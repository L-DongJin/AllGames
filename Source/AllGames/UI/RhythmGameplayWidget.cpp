// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmGameplayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "../Core/RhythmPlayerController.h"
#include "../Judgement/RhythmJudgementManager.h"
#include "../Notes/RhythmNoteSpawner.h"
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

	for (TActorIterator<ARhythmJudgementManager> It(GetWorld()); It; ++It)
	{
		JudgementManager = *It;
		break;
	}
	if (JudgementManager)
	{
		JudgementManager->OnNoteJudged.AddDynamic(this, &ThisClass::HandleNoteJudged);
	}

	for (TActorIterator<ARhythmScoreManager> It(GetWorld()); It; ++It)
	{
		ScoreManager = *It;
		break;
	}
	if (ScoreManager)
	{
		ScoreManager->OnScoreChanged.AddDynamic(this, &ThisClass::HandleScoreChanged);
		RefreshScoreText(ScoreManager->GetScoreState());
	}

	UE_LOG(LogTemp, Log, TEXT("Rhythm gameplay WBP ready: %d lanes."), LaneCount);
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
		LaneCanvas->AddChildToCanvas(KeyLabel);
		LaneKeyLabels.Add(KeyLabel);
	}

	JudgementLine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("JudgementLine"));
	JudgementLine->SetColorAndOpacity(FLinearColor::White);
	if (JudgementLineImage) JudgementLine->SetBrushFromTexture(JudgementLineImage);
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
	HitCountFont.Size = 44;
	JudgementHitCountText->SetFont(HitCountFont);
	JudgementHitCountText->SetVisibility(ESlateVisibility::Collapsed);
	RootCanvas->AddChildToCanvas(JudgementHitCountText);

	ScoreText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ScoreText"));
	ComboText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ComboText"));
	AccuracyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AccuracyText"));
	for (UTextBlock* TextBlock : { ScoreText.Get(), ComboText.Get(), AccuracyText.Get() })
	{
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		TextBlock->SetShadowOffset(FVector2D(2.0f, 2.0f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
		RootCanvas->AddChildToCanvas(TextBlock);
	}
	FSlateFontInfo ScoreFont = ScoreText->GetFont();
	ScoreFont.Size = 30;
	ScoreText->SetFont(ScoreFont);
	AccuracyText->SetFont(ScoreFont);
	FSlateFontInfo ComboFont = ComboText->GetFont();
	ComboFont.Size = 42;
	ComboText->SetFont(ComboFont);
	RefreshScoreText(FRhythmScoreState());

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

	ResultHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultHintText"));
	ResultHintText->SetText(FText::FromString(TEXT("Enter: Retry    Esc: Lobby")));
	ResultHintText->SetJustification(ETextJustify::Center);
	ResultHintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.7f, 0.8f)));
	FSlateFontInfo ResultHintFont = ResultHintText->GetFont();
	ResultHintFont.Size = 24;
	ResultHintText->SetFont(ResultHintFont);
	ResultHintText->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* ResultHintSlot = RootCanvas->AddChildToCanvas(ResultHintText))
	{
		ResultHintSlot->SetAnchors(FAnchors(0.5f, 0.88f));
		ResultHintSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		ResultHintSlot->SetSize(FVector2D(900.0f, 60.0f));
		ResultHintSlot->SetZOrder(101);
	}
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
	if (NoteImage) Note->SetBrushFromTexture(NoteImage);
	LaneCanvas->AddChildToCanvas(Note);

	FNoteVisual& Visual = NoteVisuals.AddDefaulted_GetRef();
	Visual.Data = NoteData;
	Visual.Image = Note;
	Visual.SpawnTimeSeconds = NoteData.TargetTimeSeconds - (Spawner ? Spawner->GetSpawnLeadTimeSeconds() : 2.0f);
	// OnTimelineUpdated is broadcast before newly due notes are spawned. Position this note now so it
	// cannot flash for one frame at the canvas default position in the upper-left corner.
	const float CurrentMusicTime = Conductor ? Conductor->GetMusicTimeSeconds() : Visual.SpawnTimeSeconds;
	RefreshLayout(CurrentMusicTime);
	UE_LOG(LogTemp, Log, TEXT("Created UI note: lane %d, target %.2f"), NoteData.LaneIndex + 1, NoteData.TargetTimeSeconds);
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
			if (Visual.Image)
			{
				Visual.Image->SetVisibility(ESlateVisibility::Collapsed);
				Visual.Image->RemoveFromParent();
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
		|| !ResultGreatText || !ResultGoodText || !ResultMissText || !ResultHintText)
	{
		return;
	}

	const FRhythmScoreState FinalState = ScoreManager ? ScoreManager->GetScoreState() : FRhythmScoreState();
	const int32 TotalNotes = FinalState.PerfectCount + FinalState.GreatCount
		+ FinalState.GoodCount + FinalState.MissCount;
	ResultSummaryText->SetText(FText::FromString(FString::Printf(
		TEXT("SCORE  %010lld\n\nMAX COMBO  %d / %d\nACCURACY  %.2f%%"),
		FinalState.Score,
		FinalState.MaxCombo,
		TotalNotes,
		FinalState.AccuracyPercent)));
	ResultPerfectText->SetText(FText::FromString(FString::Printf(TEXT("PERFECT  %d"), FinalState.PerfectCount)));
	ResultGreatText->SetText(FText::FromString(FString::Printf(TEXT("GREAT  %d"), FinalState.GreatCount)));
	ResultGoodText->SetText(FText::FromString(FString::Printf(TEXT("GOOD  %d"), FinalState.GoodCount)));
	ResultMissText->SetText(FText::FromString(FString::Printf(TEXT("MISS  %d"), FinalState.MissCount)));

	ResultBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultTitleText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultSummaryText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultPerfectText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultGreatText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultGoodText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultMissText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultHintText->SetVisibility(ESlateVisibility::HitTestInvisible);
	bShowingResults = true;
	SetKeyboardFocus();
	if (JudgementFeedback) JudgementFeedback->SetVisibility(ESlateVisibility::Collapsed);
	if (JudgementHitCountText) JudgementHitCountText->SetVisibility(ESlateVisibility::Collapsed);
	UE_LOG(LogTemp, Log, TEXT("Rhythm result displayed: score %lld, max combo %d, accuracy %.2f%%, notes %d"),
		FinalState.Score, FinalState.MaxCombo, FinalState.AccuracyPercent, TotalNotes);
}

FReply URhythmGameplayWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bShowingResults)
	{
		if (InKeyEvent.GetKey() == EKeys::Enter)
		{
			const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
			UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));
			return FReply::Handled();
		}
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			UGameplayStatics::OpenLevel(this, TEXT("LobbyMap"));
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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
			FeedbackSlot->SetAnchors(FAnchors(0.5f, JudgementFeedbackVerticalPosition));
			FeedbackSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			FeedbackSlot->SetPosition(FVector2D::ZeroVector);
			FVector2D FeedbackSize = JudgementFeedbackMaxSize;
			if (const UTexture2D* FeedbackTexture = Cast<UTexture2D>(JudgementFeedback->GetBrush().GetResourceObject()))
			{
				const FVector2D TextureSize(FeedbackTexture->GetSizeX(), FeedbackTexture->GetSizeY());
				if (TextureSize.X > 0.0f && TextureSize.Y > 0.0f)
				{
					const float UniformScale = FMath::Min(
						JudgementFeedbackMaxSize.X / TextureSize.X,
						JudgementFeedbackMaxSize.Y / TextureSize.Y);
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
			CountSlot->SetAnchors(FAnchors(0.5f, JudgementFeedbackVerticalPosition));
			CountSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			CountSlot->SetPosition(FVector2D(0.0f, JudgementFeedbackMaxSize.Y * 0.38f));
			CountSlot->SetSize(FVector2D(360.0f, 70.0f));
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
		const float Progress = TravelDuration > 0.0f
			? FMath::Clamp((MusicTime - Visual.SpawnTimeSeconds) / TravelDuration, 0.0f, 1.25f)
			: 1.0f;
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Visual.Image->Slot))
		{
			CanvasSlot->SetPosition(FVector2D(Visual.Data.LaneIndex * LaneWidth + 6.0f, SpawnY - 12.0f));
			CanvasSlot->SetSize(FVector2D(LaneWidth - 12.0f, 24.0f));
			Visual.Image->SetRenderTranslation(FVector2D(
				0.0f,
				FMath::Lerp(SpawnY, JudgementY, Progress) - SpawnY));
			// Runtime-generated moving images must invalidate their cached paint/layout state.
			Visual.Image->InvalidateLayoutAndVolatility();
		}
	}
}
