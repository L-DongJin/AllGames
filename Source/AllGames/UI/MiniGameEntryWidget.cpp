// Copyright Epic Games, Inc. All Rights Reserved.

#include "MiniGameEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
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
		ApplyCardFrameStyle();
	}
	return Super::RebuildWidget();
}

void UMiniGameEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (SelectButton)
	{
		SelectButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
	}
	SetCardSize(CardSize);
	SetCardFrameImage(CardFrameTexture);
	SetDefinition(Definition);
}

void UMiniGameEntryWidget::BuildLayout()
{
	const float TextWrapWidth = FMath::Max(120.0f, CardSize.X - 40.0f);
	CardSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CardSizeBox"));
	CardSizeBox->SetWidthOverride(FMath::Max(1.0f, CardSize.X));
	CardSizeBox->SetHeightOverride(FMath::Max(1.0f, CardSize.Y));
	WidgetTree->RootWidget = CardSizeBox;

	SelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SelectButton"));
	SelectButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
	CardSizeBox->AddChild(SelectButton);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EntryContent"));
	SelectButton->AddChild(Content);
	CoverImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CoverImage"));
	if (UVerticalBoxSlot* CoverSlot = Content->AddChildToVerticalBox(CoverImage))
	{
		CoverSlot->SetPadding(FMargin(18.0f, 18.0f, 18.0f, 8.0f));
		CoverSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	USizeBox* NameBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NameBox"));
	NameBox->SetWidthOverride(TextWrapWidth);
	NameBox->SetHeightOverride(52.0f);
	NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
	NameText->SetJustification(ETextJustify::Center);
	NameText->SetAutoWrapText(true); NameText->SetWrapTextAt(TextWrapWidth);
	FSlateFontInfo NameFont = NameText->GetFont(); NameFont.Size = 24; NameText->SetFont(NameFont);
	NameBox->AddChild(NameText);
	if (UVerticalBoxSlot* NameSlot = Content->AddChildToVerticalBox(NameBox)) NameSlot->SetPadding(FMargin(14.0f, 0.0f));
	USizeBox* DescriptionBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DescriptionBox"));
	DescriptionBox->SetWidthOverride(TextWrapWidth);
	DescriptionBox->SetHeightOverride(66.0f);
	DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
	DescriptionText->SetJustification(ETextJustify::Center);
	DescriptionText->SetAutoWrapText(true); DescriptionText->SetWrapTextAt(TextWrapWidth);
	FSlateFontInfo DescriptionFont = DescriptionText->GetFont(); DescriptionFont.Size = 15; DescriptionText->SetFont(DescriptionFont);
	DescriptionBox->AddChild(DescriptionText);
	if (UVerticalBoxSlot* DescriptionSlot = Content->AddChildToVerticalBox(DescriptionBox)) DescriptionSlot->SetPadding(FMargin(14.0f, 4.0f));
	StateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StateText"));
	StateText->SetJustification(ETextJustify::Center);
	FSlateFontInfo StateFont = StateText->GetFont(); StateFont.Size = 16; StateText->SetFont(StateFont);
	StateText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.7f, 0.2f)));
	if (UVerticalBoxSlot* StateSlot = Content->AddChildToVerticalBox(StateText)) StateSlot->SetPadding(FMargin(14.0f, 4.0f, 14.0f, 14.0f));
}

void UMiniGameEntryWidget::SetDefinition(UMiniGameDefinitionDataAsset* InDefinition)
{
	Definition = InDefinition;
	if (!Definition) return;
	if (CoverImage)
	{
		if (Definition->CoverImage)
		{
			CoverImage->SetBrushFromTexture(Definition->CoverImage, true);
			CoverImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			CoverImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (NameText) NameText->SetText(Definition->DisplayName);
	if (DescriptionText) DescriptionText->SetText(Definition->Description);
	if (StateText)
	{
		StateText->SetText(Definition->bEnabled ? FText::GetEmpty() : FText::FromString(TEXT("준비 중")));
		StateText->SetVisibility(Definition->bEnabled ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (SelectButton) SelectButton->SetIsEnabled(Definition->bEnabled);
}

void UMiniGameEntryWidget::SetCardFrameImage(UTexture2D* InImage)
{
	CardFrameTexture = InImage;
	if (CardFrameWidget && InImage)
	{
		CardFrameWidget->SetBrushFromTexture(InImage, true);
	}
	ApplyCardFrameStyle();
}

void UMiniGameEntryWidget::SetCardSize(const FVector2D InSize)
{
	CardSize = InSize;
	if (CardSizeBox && InSize.X > 0.0f && InSize.Y > 0.0f)
	{
		CardSizeBox->SetWidthOverride(InSize.X);
		CardSizeBox->SetHeightOverride(InSize.Y);
	}
}

void UMiniGameEntryWidget::ApplyCardFrameStyle()
{
	if (!SelectButton) return;
	if (CardFrameWidget)
	{
		FButtonStyle TransparentStyle = SelectButton->GetStyle();
		FSlateBrush EmptyBrush;
		EmptyBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		TransparentStyle.SetNormal(EmptyBrush).SetHovered(EmptyBrush).SetPressed(EmptyBrush).SetDisabled(EmptyBrush);
		SelectButton->SetStyle(TransparentStyle);
		return;
	}
	if (!CardFrameTexture) return;
	FButtonStyle Style = SelectButton->GetStyle();
	FSlateBrush NormalBrush;
	NormalBrush.SetResourceObject(CardFrameTexture);
	NormalBrush.DrawAs = ESlateBrushDrawType::Box;
	NormalBrush.Margin = FMargin(0.18f);
	NormalBrush.TintColor = FSlateColor(FLinearColor::White);
	FSlateBrush HoveredBrush = NormalBrush;
	HoveredBrush.TintColor = FSlateColor(FLinearColor(1.25f, 1.25f, 1.35f, 1.0f));
	FSlateBrush PressedBrush = NormalBrush;
	PressedBrush.TintColor = FSlateColor(FLinearColor(0.70f, 0.78f, 1.0f, 1.0f));
	Style.SetNormal(NormalBrush).SetHovered(HoveredBrush).SetPressed(PressedBrush).SetDisabled(NormalBrush);
	SelectButton->SetStyle(Style);
}

void UMiniGameEntryWidget::HandleClicked()
{
	if (Definition && Definition->bEnabled) OnSelected.Broadcast(Definition);
}