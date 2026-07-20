// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MiniGameEntryWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class USizeBox;
class UMiniGameDefinitionDataAsset;
class UTexture2D;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMiniGameEntrySelected, UMiniGameDefinitionDataAsset*);

/** Reusable hub card for one catalog entry. */
UCLASS()
class ALLGAMES_API UMiniGameEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnMiniGameEntrySelected OnSelected;
	void SetDefinition(UMiniGameDefinitionDataAsset* InDefinition);
	void SetCardFrameImage(UTexture2D* InImage);
	void SetCardSize(FVector2D InSize);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildLayout();
	void ApplyCardFrameStyle();

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	TObjectPtr<UMiniGameDefinitionDataAsset> Definition;

	UPROPERTY(meta = (BindWidgetOptional), Transient)
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CardFrameTexture;

	UPROPERTY(meta = (BindWidgetOptional), Transient)
	TObjectPtr<UImage> CardFrameWidget;

	UPROPERTY(meta = (BindWidgetOptional), Transient)
	TObjectPtr<USizeBox> CardSizeBox;

	FVector2D CardSize = FVector2D(380.0f, 440.0f);

	UPROPERTY(meta = (BindWidgetOptional), Transient)
	TObjectPtr<UImage> CoverImage;

	UPROPERTY(meta = (BindWidgetOptional), Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional), Transient)
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional), Transient)
	TObjectPtr<UTextBlock> StateText;
};