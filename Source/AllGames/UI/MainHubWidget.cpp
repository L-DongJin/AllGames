// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainHubWidget.h"
#include "../Audio/UiSoundStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MiniGameEntryWidget.h"
#include "../Data/MiniGameCatalogDataAsset.h"
#include "../Data/MiniGameDefinitionDataAsset.h"

namespace
{
	UTextBlock* HubText(UWidgetTree* Tree, const TCHAR* Name, const FString& Value, int32 Size)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(FText::FromString(Value)); Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo Font = Text->GetFont(); Font.Size = Size; Text->SetFont(Font);
		return Text;
	}
}

UMainHubWidget::UMainHubWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsFocusable(true);
	GameCatalog = TSoftObjectPtr<UMiniGameCatalogDataAsset>(FSoftObjectPath(TEXT("/Game/Common/Data/DA_MiniGameCatalog.DA_MiniGameCatalog")));
	GameCardWidgetClass = TSoftClassPtr<UMiniGameEntryWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_GameCard.WBP_GameCard_C")));
}

TSharedRef<SWidget> UMainHubWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildLayout();
	return Super::RebuildWidget();
}

void UMainHubWidget::NativeConstruct()
{
	Super::NativeConstruct();
	GameGrid = Cast<UUniformGridPanel>(WidgetTree ? WidgetTree->FindWidget(TEXT("GameGrid")) : nullptr);
	RhythmGameButton = Cast<UButton>(WidgetTree ? WidgetTree->FindWidget(TEXT("btn_RhythmGame")) : nullptr);
	QuizGameButton = Cast<UButton>(WidgetTree ? WidgetTree->FindWidget(TEXT("btn_QuizGame")) : nullptr);
	ExitBackground = Cast<UBorder>(WidgetTree ? WidgetTree->FindWidget(TEXT("ExitBackground")) : nullptr);
	ExitConfirmButton = Cast<UButton>(WidgetTree ? WidgetTree->FindWidget(TEXT("ExitConfirmButton")) : nullptr);
	if (!ExitConfirmButton) ExitConfirmButton = Cast<UButton>(WidgetTree ? WidgetTree->FindWidget(TEXT("ExitconfirmButton")) : nullptr);
	ExitCancelButton = Cast<UButton>(WidgetTree ? WidgetTree->FindWidget(TEXT("ExitCancelButton")) : nullptr);
	if (RhythmGameButton) RhythmGameButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRhythmGameClicked);
	if (QuizGameButton) QuizGameButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQuizGameClicked);
	if (ExitConfirmButton) ExitConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleExitConfirmed);
	if (ExitCancelButton) ExitCancelButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleExitCanceled);
	if (!IsDesignTime())
	{
		// WBP_GameSelect owns two hand-authored buttons. The native/catalog grid remains
		// only as a safe fallback when that designer hierarchy is unavailable.
		if (!RhythmGameButton || !QuizGameButton) PopulateGames();
		SetExitVisible(false);
	}
	AllGamesUiSound::ApplyButtonClickSound(WidgetTree);
}

void UMainHubWidget::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MainHubRoot"));
	WidgetTree->RootWidget = Root;
	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
	if (BackgroundImage) Background->SetBrushFromTexture(BackgroundImage, true);
	else Background->SetColorAndOpacity(FLinearColor(0.006f, 0.012f, 0.035f, 1.0f));
	auto* BackgroundSlot = Root->AddChildToCanvas(Background); BackgroundSlot->SetAnchors(FAnchors(0,0,1,1)); BackgroundSlot->SetOffsets(FMargin(0));

	auto AddCentered = [Root](UWidget* Widget, float Y, FVector2D Size)
	{
		auto* Slot = Root->AddChildToCanvas(Widget); Slot->SetAnchors(FAnchors(0.5f,Y));
		Slot->SetAlignment(FVector2D(0.5f,0.5f)); Slot->SetSize(Size);
	};
	AddCentered(HubText(WidgetTree, TEXT("Title"), TEXT("ALL GAMES"), 68), 0.10f, FVector2D(900,100));
	AddCentered(HubText(WidgetTree, TEXT("Subtitle"), TEXT("플레이할 게임을 선택하세요"), 26), 0.17f, FVector2D(800,50));
	GameGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("GameGrid"));
	AddCentered(GameGrid, 0.53f, FVector2D(1240,700));

	ExitBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ExitBackground"));
	ExitBackground->SetBrushColor(FLinearColor(0.005f,0.01f,0.03f,0.95f));
	auto* ExitSlot = Root->AddChildToCanvas(ExitBackground); ExitSlot->SetAnchors(FAnchors(0,0,1,1)); ExitSlot->SetOffsets(FMargin(0));
	AddCentered(HubText(WidgetTree, TEXT("ExitTitle"), TEXT("게임을 종료하시겠습니까?"), 38), 0.43f, FVector2D(700,70));
	ExitConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ExitConfirmButton"));
	ExitConfirmButton->AddChild(HubText(WidgetTree, TEXT("ExitConfirmText"), TEXT("게임 종료"), 28));
	ExitConfirmButton->OnClicked.AddDynamic(this, &ThisClass::HandleExitConfirmed); AddCentered(ExitConfirmButton,0.53f,FVector2D(220,70));
	if (auto* ConfirmCanvasSlot = Cast<UCanvasPanelSlot>(ExitConfirmButton->Slot)) ConfirmCanvasSlot->SetPosition(FVector2D(-130,0));
	ExitCancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ExitCancelButton"));
	ExitCancelButton->AddChild(HubText(WidgetTree, TEXT("ExitCancelText"), TEXT("취소"), 28));
	ExitCancelButton->OnClicked.AddDynamic(this, &ThisClass::HandleExitCanceled); AddCentered(ExitCancelButton,0.53f,FVector2D(220,70));
	if (auto* CancelCanvasSlot = Cast<UCanvasPanelSlot>(ExitCancelButton->Slot)) CancelCanvasSlot->SetPosition(FVector2D(130,0));
	SetExitVisible(false);
}

void UMainHubWidget::PopulateGames()
{
	// Widget Blueprint designer instances have no gameplay world/player controller.
	// Creating catalog-entry user widgets there can dereference an invalid designer context.
	if (IsDesignTime()) return;

	UMiniGameCatalogDataAsset* Catalog = GameCatalog.LoadSynchronous();
	if (!Catalog || !GameGrid) return;
	GameGrid->ClearChildren();
	for (int32 Index=0; Index<Catalog->Games.Num(); ++Index)
	{
		if (!Catalog->Games[Index]) continue;
		UClass* CardClass = GameCardWidgetClass.LoadSynchronous();
		UMiniGameEntryWidget* Entry = CreateWidget<UMiniGameEntryWidget>(GetOwningPlayer(), CardClass ? CardClass : UMiniGameEntryWidget::StaticClass());
		if (!Entry) continue;
		if (GameCardSize.X > 0.0f && GameCardSize.Y > 0.0f) Entry->SetCardSize(GameCardSize);
		const bool bRhythmCard = Catalog->Games[Index]->GameId == TEXT("RHYTHM");
		UTexture2D* SpecificFrame = bRhythmCard ? RhythmGameCardFrameImage.Get() : QuizGameCardFrameImage.Get();
		Entry->SetCardFrameImage(SpecificFrame ? SpecificFrame : GameCardFrameImage.Get());
		Entry->SetTextBoxSizes(GameCardTitleTextBoxSize, GameCardDescriptionTextBoxSize);
		Entry->SetCoverOverride(bRhythmCard
			? RhythmGameCardImage.Get() : QuizGameCardImage.Get());
		Entry->SetDefinition(Catalog->Games[Index]); Entry->OnSelected.AddUObject(this, &ThisClass::HandleGameSelected);
		if (UUniformGridSlot* GridSlot = GameGrid->AddChildToUniformGrid(Entry, Index/3, Index%3))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

void UMainHubWidget::HandleGameSelected(UMiniGameDefinitionDataAsset* Definition)
{
	if (!Definition || !Definition->bEnabled || Definition->EntryMap.IsNull()) return;
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Definition->EntryMap);
}

FReply UMainHubWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape) { SetExitVisible(!bExitVisible); return FReply::Handled(); }
	if (bExitVisible) return FReply::Handled();
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UMainHubWidget::SetExitVisible(bool bVisible)
{
	bExitVisible=bVisible; const ESlateVisibility V=bVisible?ESlateVisibility::Visible:ESlateVisibility::Collapsed;
	if(ExitBackground)ExitBackground->SetVisibility(V); if(ExitConfirmButton)ExitConfirmButton->SetVisibility(V); if(ExitCancelButton)ExitCancelButton->SetVisibility(V);
	if(UWidget* Title=WidgetTree?WidgetTree->FindWidget(TEXT("ExitTitle")):nullptr)Title->SetVisibility(V);
}

void UMainHubWidget::HandleExitConfirmed(){ UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false); }
void UMainHubWidget::HandleExitCanceled(){ SetExitVisible(false); SetKeyboardFocus(); }
void UMainHubWidget::HandleRhythmGameClicked(){ UGameplayStatics::OpenLevel(this,TEXT("LobbyMap")); }
void UMainHubWidget::HandleQuizGameClicked(){ UGameplayStatics::OpenLevel(this,TEXT("IdolQuizRoomMap")); }
