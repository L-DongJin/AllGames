// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmLobbyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "../Core/RhythmGameInstance.h"
#include "../Data/RhythmSongDataAsset.h"

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

void URhythmLobbyWidget::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LobbyRoot"));
	WidgetTree->RootWidget = Root;
	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LobbyBackground"));
	Background->SetColorAndOpacity(FLinearColor(0.008f, 0.015f, 0.045f, 1.0f));
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

	AddCentered(MakeText(WidgetTree, TEXT("LobbyTitle"), TEXT("RHYTHM SELECT"), 74, FLinearColor(0.25f, 0.85f, 1.0f)), 0.10f, FVector2D(1000, 110));
	AddCentered(MakeText(WidgetTree, TEXT("LobbySubtitle"), TEXT("SELECT SONG AND PLAY SETTINGS"), 24, FLinearColor(0.65f, 0.7f, 0.85f)), 0.18f, FVector2D(900, 50));
	AddCentered(MakeText(WidgetTree, TEXT("SongLabel"), TEXT("SONG"), 30, FLinearColor::White), 0.27f, FVector2D(420, 50));
	SongValueText = MakeText(WidgetTree, TEXT("SongValue"), TEXT("CHOOM"), 38, FLinearColor(0.25f, 0.85f, 1.0f));
	AddCentered(SongValueText, 0.32f, FVector2D(600, 60));
	AddCentered(MakeText(WidgetTree, TEXT("DifficultyLabel"), TEXT("DIFFICULTY"), 30, FLinearColor::White), 0.40f, FVector2D(420, 50));
	DifficultyValueText = MakeText(WidgetTree, TEXT("DifficultyValue"), TEXT("NORMAL"), 42, FLinearColor(1.0f, 0.8f, 0.2f));
	AddCentered(DifficultyValueText, 0.45f, FVector2D(420, 60));
	AddCentered(MakeText(WidgetTree, TEXT("SpeedLabel"), TEXT("NOTE SPEED"), 30, FLinearColor::White), 0.53f, FVector2D(420, 50));
	SpeedValueText = MakeText(WidgetTree, TEXT("SpeedValue"), TEXT("1x"), 42, FLinearColor(0.35f, 1.0f, 0.65f));
	AddCentered(SpeedValueText, 0.58f, FVector2D(420, 60));

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
	AddArrowButton(TEXT("PreviousSongButton"), TEXT("<"), 0.30f, 0.32f)->OnClicked.AddDynamic(this, &ThisClass::PreviousSong);
	AddArrowButton(TEXT("NextSongButton"), TEXT(">"), 0.70f, 0.32f)->OnClicked.AddDynamic(this, &ThisClass::NextSong);
	AddArrowButton(TEXT("PreviousDifficultyButton"), TEXT("<"), 0.34f, 0.45f)->OnClicked.AddDynamic(this, &ThisClass::PreviousDifficulty);
	AddArrowButton(TEXT("NextDifficultyButton"), TEXT(">"), 0.66f, 0.45f)->OnClicked.AddDynamic(this, &ThisClass::NextDifficulty);
	AddArrowButton(TEXT("DecreaseSpeedButton"), TEXT("<"), 0.34f, 0.58f)->OnClicked.AddDynamic(this, &ThisClass::DecreaseSpeed);
	AddArrowButton(TEXT("IncreaseSpeedButton"), TEXT(">"), 0.66f, 0.58f)->OnClicked.AddDynamic(this, &ThisClass::IncreaseSpeed);

	UButton* StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
	StartButton->AddChild(MakeText(WidgetTree, TEXT("StartButtonText"), TEXT("START"), 42, FLinearColor::White));
	StartButton->OnClicked.AddDynamic(this, &ThisClass::HandleStartClicked);
	AddCentered(StartButton, 0.72f, FVector2D(440, 84));

	HelpText = MakeText(WidgetTree, TEXT("LobbyHelp"), TEXT("UP/DOWN: SELECT    LEFT/RIGHT: CHANGE    ENTER: START"), 22, FLinearColor(0.55f, 0.62f, 0.75f));
	AddCentered(HelpText, 0.87f, FVector2D(1250, 50));
	RefreshSettings();
}

void URhythmLobbyWidget::NativeDestruct()
{
	StopSongPreview();
	Super::NativeDestruct();
}

FReply URhythmLobbyWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
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
		if (SpeedValueText) SpeedValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%dx"), FMath::RoundToInt(Settings->GetScrollSpeed()))));
	}
	if (HelpText)
	{
		const TCHAR* RowName = SelectedRow == 0 ? TEXT("SONG") : SelectedRow == 1 ? TEXT("DIFFICULTY") : SelectedRow == 2 ? TEXT("NOTE SPEED") : TEXT("START");
		HelpText->SetText(FText::FromString(FString::Printf(TEXT("SELECTED: %s    UP/DOWN: SELECT    LEFT/RIGHT: CHANGE    ENTER: CONFIRM"), RowName)));
	}
	RefreshSongPreview();
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

void URhythmLobbyWidget::PreviousDifficulty() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeDifficulty(-1); RefreshSettings(); }
void URhythmLobbyWidget::NextDifficulty() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeDifficulty(1); RefreshSettings(); }
void URhythmLobbyWidget::PreviousSong() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeSong(-1); RefreshSettings(); }
void URhythmLobbyWidget::NextSong() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeSong(1); RefreshSettings(); }
void URhythmLobbyWidget::DecreaseSpeed() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeScrollSpeed(-1); RefreshSettings(); }
void URhythmLobbyWidget::IncreaseSpeed() { if (auto* S = Cast<URhythmGameInstance>(GetGameInstance())) S->ChangeScrollSpeed(1); RefreshSettings(); }
void URhythmLobbyWidget::HandleStartClicked() { StartGame(); }
