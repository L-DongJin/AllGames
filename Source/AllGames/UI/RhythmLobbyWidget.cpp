// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmLobbyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundBase.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "TimerManager.h"
#include "../Core/RhythmGameInstance.h"
#include "../Data/RhythmSongDataAsset.h"
#include "../Online/RhythmLeaderboardSubsystem.h"

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree, const TCHAR* Name, const FString& Text, int32 Size, FLinearColor Color)
	{
		UTextBlock* Result = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Result->SetText(FText::FromString(Text));
		Result->SetJustification(ETextJustify::Center);
		Result->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo Font = Result->GetFont();
		Font.Size = Size;
		Result->SetFont(Font);
		return Result;
	}
}

URhythmLobbyWidget::URhythmLobbyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> URhythmLobbyWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildLayout();
	return Super::RebuildWidget();
}

void URhythmLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (URhythmLeaderboardSubsystem* Leaderboards = GetGameInstance()
		? GetGameInstance()->GetSubsystem<URhythmLeaderboardSubsystem>() : nullptr)
	{
		Leaderboards->OnTopLeaderboardCompleted.AddUObject(
			this, &ThisClass::HandleTopLeaderboardCompleted);
		bLeaderboardDelegatesBound = true;
		RefreshLeaderboard();
	}
}

void URhythmLobbyWidget::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LobbyRoot"));
	WidgetTree->RootWidget = Root;
	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LobbyBackground"));
	if (LobbyBackgroundImage)
	{
		Background->SetBrushFromTexture(LobbyBackgroundImage, true);
		Background->SetColorAndOpacity(LobbyBackgroundTint);
	}
	else
	{
		Background->SetColorAndOpacity(FLinearColor(0.008f, 0.015f, 0.045f, 1.0f));
	}
	auto* BackgroundSlot = Root->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	BackgroundSlot->SetOffsets(FMargin(0));

	auto AddCentered = [Root](UWidget* Widget, float Y, FVector2D Size)
	{
		auto* Slot = Root->AddChildToCanvas(Widget);
		Slot->SetAnchors(FAnchors(0.5f, Y));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetSize(Size);
	};

	const FVector2D SongImageSize(
		FMath::Max(1.0f, SongImageWidth),
		FMath::Max(1.0f, SongImageHeight));
	PreviousSongTitleImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PreviousSongTitleImage"));
	PreviousSongTitleImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
	AddCentered(PreviousSongTitleImage, 0.255f, SongImageSize);
	SongTitleImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SongTitleImage"));
	SongTitleImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.72f));
	AddCentered(SongTitleImage, 0.255f, SongImageSize);
	SongValueText = MakeText(WidgetTree, TEXT("SongValue"), TEXT("CHOOM"), 38, FLinearColor(0.25f, 0.85f, 1.0f));
	AddCentered(SongValueText, 0.465f, FVector2D(700, 60));
	AddCentered(MakeText(WidgetTree, TEXT("DifficultyLabel"), TEXT("DIFFICULTY"), 28, FLinearColor::White), 0.535f, FVector2D(420, 46));
	DifficultyValueText = MakeText(WidgetTree, TEXT("DifficultyValue"), TEXT("NORMAL"), 42, FLinearColor(1.0f, 0.8f, 0.2f));
	AddCentered(DifficultyValueText, 0.58f, FVector2D(420, 56));
	ChartLevelText = MakeText(WidgetTree, TEXT("ChartLevelValue"), TEXT("LEVEL 1"), 34, FLinearColor(1.0f, 0.35f, 0.55f));
	AddCentered(ChartLevelText, 0.625f, FVector2D(420, 48));
	AddCentered(MakeText(WidgetTree, TEXT("SpeedLabel"), TEXT("NOTE SPEED"), 28, FLinearColor::White), 0.69f, FVector2D(420, 46));
	SpeedValueText = MakeText(WidgetTree, TEXT("SpeedValue"), TEXT("1x"), 42, FLinearColor(0.35f, 1.0f, 0.65f));
	AddCentered(SpeedValueText, 0.735f, FVector2D(420, 56));

	auto AddArrowButton = [this, Root](const TCHAR* Name, const TCHAR* Label, float X, float Y)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->AddChild(MakeText(WidgetTree, *FString::Printf(TEXT("%sText"), Name), Label, 32, FLinearColor::White));
		auto* Slot = Root->AddChildToCanvas(Button);
		Slot->SetAnchors(FAnchors(X, Y));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetSize(FVector2D(74, 64));
		return Button;
	};
	AddArrowButton(TEXT("PreviousSongButton"), TEXT("<"), 0.25f, 0.255f)->OnClicked.AddDynamic(this, &ThisClass::PreviousSong);
	AddArrowButton(TEXT("NextSongButton"), TEXT(">"), 0.75f, 0.255f)->OnClicked.AddDynamic(this, &ThisClass::NextSong);
	AddArrowButton(TEXT("PreviousDifficultyButton"), TEXT("<"), 0.34f, 0.58f)->OnClicked.AddDynamic(this, &ThisClass::PreviousDifficulty);
	AddArrowButton(TEXT("NextDifficultyButton"), TEXT(">"), 0.66f, 0.58f)->OnClicked.AddDynamic(this, &ThisClass::NextDifficulty);
	AddArrowButton(TEXT("DecreaseSpeedButton"), TEXT("<"), 0.34f, 0.735f)->OnClicked.AddDynamic(this, &ThisClass::DecreaseSpeed);
	AddArrowButton(TEXT("IncreaseSpeedButton"), TEXT(">"), 0.66f, 0.735f)->OnClicked.AddDynamic(this, &ThisClass::IncreaseSpeed);

	UButton* StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
	StartButton->AddChild(MakeText(WidgetTree, TEXT("StartButtonText"), TEXT("START"), 42, FLinearColor::White));
	StartButton->OnClicked.AddDynamic(this, &ThisClass::HandleStartClicked);
	AddCentered(StartButton, 0.84f, FVector2D(440, 76));

	HelpText = MakeText(WidgetTree, TEXT("LobbyHelp"), TEXT("UP/DOWN: SELECT    LEFT/RIGHT: CHANGE    ENTER: START"), 22, FLinearColor(0.55f, 0.62f, 0.75f));
	AddCentered(HelpText, 0.94f, FVector2D(1250, 40));

	MainHubButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MainHubButton"));
	MainHubButton->AddChild(MakeText(WidgetTree, TEXT("MainHubButtonText"), TEXT("ALL GAMES"), 22, FLinearColor::White));
	MainHubButton->OnClicked.AddDynamic(this, &ThisClass::HandleReturnToMainHub);
	if (UCanvasPanelSlot* HubButtonSlot = Root->AddChildToCanvas(MainHubButton))
	{
		HubButtonSlot->SetAnchors(FAnchors(0.035f, 0.045f));
		HubButtonSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		HubButtonSlot->SetSize(FVector2D(180.0f, 52.0f));
	}

	LeaderboardTitleText = MakeText(WidgetTree, TEXT("LeaderboardTitleText"), TEXT("ONLINE TOP 10"),
		LeaderboardTitleFontSize, LeaderboardTitleColor);
	if (LeaderboardFont)
	{
		FSlateFontInfo Font = LeaderboardTitleText->GetFont();
		Font.FontObject = LeaderboardFont.Get();
		LeaderboardTitleText->SetFont(Font);
	}
	LeaderboardTitleText->SetJustification(ETextJustify::Left);
	if (UCanvasPanelSlot* TitleSlot = Root->AddChildToCanvas(LeaderboardTitleText))
	{
		TitleSlot->SetAnchors(FAnchors(LeaderboardTitlePosition.X, LeaderboardTitlePosition.Y));
		TitleSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		TitleSlot->SetSize(FVector2D(FMath::Max(1.0f, LeaderboardAreaSize.X), 55.0f));
	}

	LeaderboardText = MakeText(WidgetTree, TEXT("LeaderboardText"), TEXT("불러오는 중..."),
		LeaderboardEntryFontSize, LeaderboardEntryColor);
	if (LeaderboardFont)
	{
		FSlateFontInfo Font = LeaderboardText->GetFont();
		Font.FontObject = LeaderboardFont.Get();
		LeaderboardText->SetFont(Font);
	}
	LeaderboardText->SetJustification(ETextJustify::Left);
	if (UCanvasPanelSlot* LeaderboardSlot = Root->AddChildToCanvas(LeaderboardText))
	{
		LeaderboardSlot->SetAnchors(FAnchors(LeaderboardEntriesPosition.X, LeaderboardEntriesPosition.Y));
		LeaderboardSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		LeaderboardSlot->SetSize(FVector2D(
			FMath::Max(1.0f, LeaderboardAreaSize.X),
			FMath::Max(1.0f, LeaderboardAreaSize.Y)));
	}

	ExitConfirmationBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ExitConfirmationBackground"));
	ExitConfirmationBackground->SetBrushColor(FLinearColor(0.005f, 0.01f, 0.03f, 0.94f));
	if (UCanvasPanelSlot* OverlaySlot = Root->AddChildToCanvas(ExitConfirmationBackground))
	{
		OverlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		OverlaySlot->SetOffsets(FMargin(0.0f));
	}

	ExitConfirmationTitle = MakeText(WidgetTree, TEXT("ExitConfirmationTitle"), TEXT("게임을 종료하시겠습니까?"), 38, FLinearColor::White);
	AddCentered(ExitConfirmationTitle, 0.43f, FVector2D(650.0f, 70.0f));

	ExitConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ExitConfirmButton"));
	ExitConfirmButton->AddChild(MakeText(WidgetTree, TEXT("ExitConfirmButtonText"), TEXT("게임 종료"), 28, FLinearColor::White));
	ExitConfirmButton->OnClicked.AddDynamic(this, &ThisClass::HandleExitConfirmed);
	if (UCanvasPanelSlot* ConfirmSlot = Root->AddChildToCanvas(ExitConfirmButton))
	{
		ConfirmSlot->SetAnchors(FAnchors(0.43f, 0.53f));
		ConfirmSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ConfirmSlot->SetSize(FVector2D(210.0f, 68.0f));
	}

	ExitCancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ExitCancelButton"));
	ExitCancelButton->AddChild(MakeText(WidgetTree, TEXT("ExitCancelButtonText"), TEXT("취소"), 28, FLinearColor::White));
	ExitCancelButton->OnClicked.AddDynamic(this, &ThisClass::HandleExitCanceled);
	if (UCanvasPanelSlot* CancelSlot = Root->AddChildToCanvas(ExitCancelButton))
	{
		CancelSlot->SetAnchors(FAnchors(0.57f, 0.53f));
		CancelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CancelSlot->SetSize(FVector2D(210.0f, 68.0f));
	}
	SetExitConfirmationVisible(false);
	RefreshSettings();
}

void URhythmLobbyWidget::NativeDestruct()
{
	if (URhythmLeaderboardSubsystem* Leaderboards = GetGameInstance()
		? GetGameInstance()->GetSubsystem<URhythmLeaderboardSubsystem>() : nullptr)
	{
		Leaderboards->OnTopLeaderboardCompleted.RemoveAll(this);
	}
	bLeaderboardDelegatesBound = false;
	StopSongPreview();
	Super::NativeDestruct();
}

void URhythmLobbyWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bTitleTransitionActive || !SongTitleImage || !PreviousSongTitleImage)
	{
		return;
	}
	TitleTransitionElapsed += InDeltaTime;
	const float Alpha = FMath::Clamp(TitleTransitionElapsed / FMath::Max(TitleTransitionDuration, 0.01f), 0.0f, 1.0f);
	const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	SongTitleImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.72f * SmoothAlpha));
	PreviousSongTitleImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.72f * (1.0f - SmoothAlpha)));
	SongTitleImage->SetRenderScale(FVector2D(FMath::Lerp(0.96f, 1.0f, SmoothAlpha)));
	if (Alpha >= 1.0f)
	{
		bTitleTransitionActive = false;
		PreviousSongTitleImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
	}
}

FReply URhythmLobbyWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape)
	{
		SetExitConfirmationVisible(!bExitConfirmationVisible);
		return FReply::Handled();
	}
	if (bExitConfirmationVisible)
	{
		return FReply::Handled();
	}
	if (Key == EKeys::Up) { SelectedRow = FMath::Max(0, SelectedRow - 1); RefreshSettings(); return FReply::Handled(); }
	if (Key == EKeys::Down) { SelectedRow = FMath::Min(3, SelectedRow + 1); RefreshSettings(); return FReply::Handled(); }
	if (Key == EKeys::Left) { if (SelectedRow == 0) PreviousSong(); else if (SelectedRow == 1) PreviousDifficulty(); else if (SelectedRow == 2) DecreaseSpeed(); return FReply::Handled(); }
	if (Key == EKeys::Right) { if (SelectedRow == 0) NextSong(); else if (SelectedRow == 1) NextDifficulty(); else if (SelectedRow == 2) IncreaseSpeed(); return FReply::Handled(); }
	if (Key == EKeys::Enter) { if (SelectedRow == 3) StartGame(); else { SelectedRow = FMath::Min(3, SelectedRow + 1); RefreshSettings(); } return FReply::Handled(); }
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void URhythmLobbyWidget::RefreshSettings()
{
	if (URhythmGameInstance* Settings = Cast<URhythmGameInstance>(GetGameInstance()))
	{
		if (SongValueText) SongValueText->SetText(Settings->GetSelectedSongDisplayName());
		if (DifficultyValueText) DifficultyValueText->SetText(Settings->GetDifficultyDisplayName());
		if (const URhythmSongDataAsset* Song = Settings->GetSelectedSong())
		{
			if (ChartLevelText)
			{
				ChartLevelText->SetText(FText::FromString(FString::Printf(TEXT("LEVEL %02d"), Song->ChartLevel)));
			}
			if (SongTitleImage && Song->TitleImage != DisplayedTitleTexture)
			{
				if (DisplayedTitleTexture && PreviousSongTitleImage)
				{
					PreviousSongTitleImage->SetBrushFromTexture(DisplayedTitleTexture, true);
				}
				DisplayedTitleTexture = Song->TitleImage;
				SongTitleImage->SetBrushFromTexture(DisplayedTitleTexture, true);
				TitleTransitionElapsed = 0.0f;
				bTitleTransitionActive = DisplayedTitleTexture != nullptr;
				SongTitleImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, bTitleTransitionActive ? 0.0f : 0.72f));
			}
		}
		if (SpeedValueText)
		{
			const float Speed = Settings->GetScrollSpeed();
			SpeedValueText->SetText(FText::FromString(
				FMath::IsNearlyEqual(Speed, FMath::RoundToFloat(Speed))
					? FString::Printf(TEXT("%.0fx"), Speed)
					: FString::Printf(TEXT("%.1fx"), Speed)));
		}
	}
	if (HelpText)
	{
		const TCHAR* RowName = SelectedRow == 0 ? TEXT("SONG") : SelectedRow == 1 ? TEXT("DIFFICULTY") : SelectedRow == 2 ? TEXT("NOTE SPEED") : TEXT("START");
		HelpText->SetText(FText::FromString(FString::Printf(TEXT("SELECTED: %s    UP/DOWN: SELECT    LEFT/RIGHT: CHANGE    ENTER: CONFIRM"), RowName)));
	}
	RefreshSongPreview();
	RefreshLeaderboard();
}

void URhythmLobbyWidget::RefreshLeaderboard()
{
	if (!bLeaderboardDelegatesBound || !LeaderboardText)
	{
		return;
	}
	const URhythmGameInstance* Settings = Cast<URhythmGameInstance>(GetGameInstance());
	const URhythmSongDataAsset* Song = Settings ? Settings->GetSelectedSong() : nullptr;
	const FString StatisticName = URhythmLeaderboardSubsystem::BuildStatisticName(Song);
	if (StatisticName.IsEmpty() || StatisticName == RequestedLeaderboardStatistic)
	{
		return;
	}
	RequestedLeaderboardStatistic = StatisticName;
	LeaderboardText->SetText(FText::FromString(TEXT("불러오는 중...")));
	if (URhythmLeaderboardSubsystem* Leaderboards =
		GetGameInstance()->GetSubsystem<URhythmLeaderboardSubsystem>())
	{
		Leaderboards->RequestTopLeaderboard(Song, 10);
	}
}

void URhythmLobbyWidget::HandleTopLeaderboardCompleted(
	const bool bSuccess, const FRhythmLeaderboardResult& Result)
{
	if (!LeaderboardText || Result.StatisticName != RequestedLeaderboardStatistic)
	{
		// A song may have changed while the previous HTTP request was in flight.
		// Retry the latest selection now that the subsystem is available again.
		RequestedLeaderboardStatistic.Reset();
		RefreshLeaderboard();
		return;
	}
	if (!bSuccess)
	{
		LeaderboardText->SetText(FText::FromString(TEXT("랭킹 조회 실패")));
		return;
	}

	FString Display;
	if (Result.Entries.IsEmpty())
	{
		Display += TEXT("아직 등록된 기록이 없습니다.");
	}
	else
	{
		for (const FRhythmLeaderboardEntry& Entry : Result.Entries)
		{
			Display += FString::Printf(TEXT("%2d.  %-14s  %d\n"),
				Entry.Rank, *Entry.DisplayName.Left(14), Entry.Score);
		}
	}
	LeaderboardText->SetText(FText::FromString(Display));
}

void URhythmLobbyWidget::RefreshSongPreview()
{
	const URhythmGameInstance* Settings = Cast<URhythmGameInstance>(GetGameInstance());
	const URhythmSongDataAsset* Song = Settings ? Settings->GetSelectedSong() : nullptr;
	if (!Song || !Song->Music)
	{
		StopSongPreview();
		return;
	}

	if (PreviewMusic == Song->Music && PreviewAudioComponent && PreviewAudioComponent->IsPlaying())
	{
		return;
	}

	StopSongPreview();
	PreviewMusic = Song->Music;

	const float MusicDuration = FMath::Max(0.0f, Song->Music->GetDuration());
	PreviewDurationSeconds = FMath::Clamp(
		Song->PreviewDurationSeconds,
		3.0f,
		FMath::Max(3.0f, MusicDuration));
	const float LatestStartTime = FMath::Max(0.0f, MusicDuration - PreviewDurationSeconds);
	PreviewStartTimeSeconds = Song->PreviewStartTimeSeconds >= 0.0f
		? FMath::Clamp(Song->PreviewStartTimeSeconds, 0.0f, LatestStartTime)
		: LatestStartTime * 0.5f;

	PreviewAudioComponent = UGameplayStatics::CreateSound2D(
		this,
		Song->Music,
		Song->PreviewVolume,
		1.0f,
		PreviewStartTimeSeconds,
		nullptr,
		false,
		false);
	if (!PreviewAudioComponent)
	{
		PreviewMusic = nullptr;
		return;
	}

	PreviewAudioComponent->FadeIn(0.35f, Song->PreviewVolume, PreviewStartTimeSeconds);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PreviewLoopTimerHandle,
			this,
			&ThisClass::RestartSongPreview,
			PreviewDurationSeconds,
			false);
	}
}

void URhythmLobbyWidget::StopSongPreview()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreviewLoopTimerHandle);
	}
	if (PreviewAudioComponent)
	{
		PreviewAudioComponent->Stop();
		PreviewAudioComponent = nullptr;
	}
	PreviewMusic = nullptr;
}

void URhythmLobbyWidget::RestartSongPreview()
{
	if (!PreviewAudioComponent)
	{
		RefreshSongPreview();
		return;
	}

	PreviewAudioComponent->Play(PreviewStartTimeSeconds);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PreviewLoopTimerHandle,
			this,
			&ThisClass::RestartSongPreview,
			PreviewDurationSeconds,
			false);
	}
}

void URhythmLobbyWidget::StartGame()
{
	StopSongPreview();
	UGameplayStatics::OpenLevel(this, TEXT("FiveKeyMap"));
}

void URhythmLobbyWidget::SetExitConfirmationVisible(const bool bVisible)
{
	bExitConfirmationVisible = bVisible;
	const ESlateVisibility ExitVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (ExitConfirmationBackground) ExitConfirmationBackground->SetVisibility(ExitVisibility);
	if (ExitConfirmationTitle) ExitConfirmationTitle->SetVisibility(ExitVisibility);
	if (ExitConfirmButton) ExitConfirmButton->SetVisibility(ExitVisibility);
	if (ExitCancelButton) ExitCancelButton->SetVisibility(ExitVisibility);
	if (bVisible && ExitCancelButton)
	{
		ExitCancelButton->SetKeyboardFocus();
	}
}

void URhythmLobbyWidget::HandleExitConfirmed()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void URhythmLobbyWidget::HandleExitCanceled()
{
	SetExitConfirmationVisible(false);
	SetKeyboardFocus();
}

void URhythmLobbyWidget::PreviousDifficulty() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeDifficulty(-1); RefreshSettings(); }
void URhythmLobbyWidget::NextDifficulty() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeDifficulty(1); RefreshSettings(); }
void URhythmLobbyWidget::PreviousSong() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeSong(-1); RefreshSettings(); }
void URhythmLobbyWidget::NextSong() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeSong(1); RefreshSettings(); }
void URhythmLobbyWidget::DecreaseSpeed() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeScrollSpeed(-1); RefreshSettings(); }
void URhythmLobbyWidget::IncreaseSpeed() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeScrollSpeed(1); RefreshSettings(); }
void URhythmLobbyWidget::HandleStartClicked() { StartGame(); }

void URhythmLobbyWidget::HandleReturnToMainHub()
{
	StopSongPreview();
	UGameplayStatics::OpenLevel(this, TEXT("MainHubMap"));
}
