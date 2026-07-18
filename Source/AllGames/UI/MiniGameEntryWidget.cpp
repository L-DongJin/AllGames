// Copyright Epic Games, Inc. All Rights Reserved.

#include "MiniGameEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "../Data/MiniGameDefinitionDataAsset.h"

TSharedRef<SWidget> UMiniGameEntryWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
		// Catalog data is assigned before the entry is inserted into the grid. Apply it
		// again after Slate controls exist so the first frame never shows an empty card.
		SetDefinition(Definition);
	}
	return Super::RebuildWidget();
}

void UMiniGameEntryWidget::BuildLayout()
{
	SelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SelectButton"));
	SelectButton->OnClicked.AddDynamic(this, &ThisClass::HandleClicked);
	WidgetTree->RootWidget = SelectButton;

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EntryContent"));
	SelectButton->AddChild(Content);
	CoverImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CoverImage"));
	if (UVerticalBoxSlot* CoverSlot = Content->AddChildToVerticalBox(CoverImage))
	{
		CoverSlot->SetPadding(FMargin(8.0f));
		CoverSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
	NameText->SetJustification(ETextJustify::Center);
	FSlateFontInfo NameFont = NameText->GetFont(); NameFont.Size = 30; NameText->SetFont(NameFont);
	Content->AddChildToVerticalBox(NameText);
	DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
	DescriptionText->SetJustification(ETextJustify::Center);
	DescriptionText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(DescriptionText);
	StateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StateText"));
	StateText->SetJustification(ETextJustify::Center);
	StateText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.7f, 0.2f)));
	Content->AddChildToVerticalBox(StateText);
}

void UMiniGameEntryWidget::SetDefinition(UMiniGameDefinitionDataAsset* InDefinition)
{
	Definition = InDefinition;
	if (!Definition) return;
	if (CoverImage && Definition->CoverImage) CoverImage->SetBrushFromTexture(Definition->CoverImage, true);
	if (NameText) NameText->SetText(Definition->DisplayName);
	if (DescriptionText) DescriptionText->SetText(Definition->Description);
	if (StateText) StateText->SetText(Definition->bEnabled ? FText::GetEmpty() : FText::FromString(TEXT("준비 중")));
	if (SelectButton) SelectButton->SetIsEnabled(Definition->bEnabled);
}

void UMiniGameEntryWidget::HandleClicked()
{
	if (Definition && Definition->bEnabled) OnSelected.Broadcast(Definition);
}
