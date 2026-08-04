#include "IdolQuizLobbyWidget.h"
#include "../Audio/UiSoundStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "../Core/IdolQuizLobbyPlayerController.h"
#include "../Core/IdolQuizPlayerState.h"
#include "../Online/IdolQuizSessionSubsystem.h"
#include "QuizPlayerColors.h"

namespace
{
	UTextBlock* LobbyText(UWidgetTree* Tree, const TCHAR* Name, const FString& Value, const int32 Size)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(FText::FromString(Value));
		Text->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		return Text;
	}
}

UIdolQuizLobbyWidget::UIdolQuizLobbyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> UIdolQuizLobbyWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UIdolQuizLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindDesignerWidgets();

	if (StartButton)
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleStart);
		const APlayerController* PlayerController = GetOwningPlayer();
		StartButton->SetVisibility(PlayerController && PlayerController->HasAuthority()
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}
	if (LeaveButton)
	{
		LeaveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleLeave);
	}

	RefreshPlayers();
	AllGamesUiSound::ApplyButtonClickSound(WidgetTree);
}

void UIdolQuizLobbyWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshPlayers();
}

void UIdolQuizLobbyWidget::BindDesignerWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	GameInfoText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("GameInfo")));
	TitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Title")));
	StatusText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Status")));
	StartButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Start")));
	LeaveButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Leave")));

	PlayerNames.Reset();
	for (int32 Index = 1; Index <= 6; ++Index)
	{
		if (UTextBlock* PlayerText = Cast<UTextBlock>(
			WidgetTree->FindWidget(*FString::Printf(TEXT("Player%d"), Index))))
		{
			PlayerNames.Add(PlayerText);
		}
	}
}

void UIdolQuizLobbyWidget::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LobbyRoot"));
	WidgetTree->RootWidget = Root;
	auto Add = [Root](UWidget* Widget, const float X, const float Y, const FVector2D Size)
	{
		UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Widget);
		Slot->SetAnchors(FAnchors(X, Y));
		Slot->SetAlignment(FVector2D(.5f, .5f));
		Slot->SetSize(Size);
	};

	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BG"));
	Background->SetColorAndOpacity(FLinearColor(.006f, .014f, .04f, 1));
	UCanvasPanelSlot* BackgroundSlot = Root->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	BackgroundSlot->SetOffsets(FMargin(0));
	Add(LobbyText(WidgetTree, TEXT("Title"), TEXT("공용 대기실"), 48), .5f, .075f, {600, 70});

	GameInfoText = LobbyText(WidgetTree, TEXT("GameInfo"), TEXT("게임 정보를 불러오는 중..."), 24);
	GameInfoText->SetColorAndOpacity(FSlateColor(FLinearColor(.25f, .9f, 1)));
	Add(GameInfoText, .5f, .145f, {900, 55});

	for (int32 Index = 0; Index < 6; ++Index)
	{
		UTextBlock* Player = LobbyText(
			WidgetTree, *FString::Printf(TEXT("Player%d"), Index + 1), TEXT("None"), 28);
		Player->SetColorAndOpacity(FSlateColor(QuizPlayerColors::Get(Index)));
		Add(Player, .28f + (Index % 3) * .22f, .3f + (Index / 3) * .2f, {300, 80});
		PlayerNames.Add(Player);
	}

	StatusText = LobbyText(WidgetTree, TEXT("Status"), TEXT("방장이 게임을 시작할 때까지 기다려 주세요."), 20);
	Add(StatusText, .5f, .68f, {900, 55});

	StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Start"));
	StartButton->AddChild(LobbyText(WidgetTree, TEXT("StartText"), TEXT("게임 시작"), 28));
	Add(StartButton, .62f, .82f, {280, 72});

	LeaveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Leave"));
	LeaveButton->AddChild(LobbyText(WidgetTree, TEXT("LeaveText"), TEXT("나가기"), 26));
	Add(LeaveButton, .38f, .82f, {240, 72});
}

void UIdolQuizLobbyWidget::RefreshPlayers()
{
	TArray<APlayerState*> Players;
	if (GetWorld() && GetWorld()->GetGameState())
	{
		Players = GetWorld()->GetGameState()->PlayerArray;
	}

	for (int32 Index = 0; Index < PlayerNames.Num(); ++Index)
	{
		if (PlayerNames[Index])
		{
			PlayerNames[Index]->SetText(FText::FromString(TEXT("None")));
			PlayerNames[Index]->SetColorAndOpacity(FSlateColor(QuizPlayerColors::Get(Index)));
		}
	}
	for (APlayerState* BasePlayerState : Players)
	{
		if (const AIdolQuizPlayerState* PlayerState = Cast<AIdolQuizPlayerState>(BasePlayerState))
		{
			const int32 ColorIndex = FMath::Clamp(PlayerState->GetPlayerColorIndex(), 0, 5);
			if (PlayerNames.IsValidIndex(ColorIndex) && PlayerNames[ColorIndex])
			{
				PlayerNames[ColorIndex]->SetText(FText::FromString(PlayerState->GetQuizPlayerName()));
			}
		}
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(FString::Printf(TEXT("%d / 6 참가"), Players.Num())));
	}

	if (GameInfoText)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UIdolQuizSessionSubsystem* Sessions = GameInstance->GetSubsystem<UIdolQuizSessionSubsystem>())
			{
				const EMiniGameRoomType Type = Sessions->GetActiveGameType();
				const FString Settings = Type == EMiniGameRoomType::DrawingQuiz
					? FString::Printf(TEXT("인당 %d회 · 제한시간 %d초"),
						Sessions->GetActiveDrawingRoundsPerPlayer(), Sessions->GetActiveDrawingRoundTime())
					: FString::Printf(TEXT("%s · %d문제"),
						*UIdolQuizSessionSubsystem::GetPoolTagsLabel(Sessions->GetActiveRoomPoolTags()),
						Sessions->GetActiveRoomQuestionCount());
				GameInfoText->SetText(FText::FromString(FString::Printf(
					TEXT("%s  |  %s"), *UIdolQuizSessionSubsystem::GetGameTypeLabel(Type), *Settings)));
			}
		}
	}

	if (TitleText)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (const UIdolQuizSessionSubsystem* Sessions = GameInstance->GetSubsystem<UIdolQuizSessionSubsystem>())
			{
				TitleText->SetText(FText::FromString(Sessions->GetActiveRoomName()));
			}
		}
	}
}

void UIdolQuizLobbyWidget::HandleStart()
{
	if (AIdolQuizLobbyPlayerController* PlayerController = GetOwningPlayer<AIdolQuizLobbyPlayerController>())
	{
		PlayerController->ServerStartSelectedGame();
	}
}

void UIdolQuizLobbyWidget::HandleLeave()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UIdolQuizSessionSubsystem* Sessions = GameInstance->GetSubsystem<UIdolQuizSessionSubsystem>())
		{
			Sessions->LeaveRoom(true);
		}
	}
}
