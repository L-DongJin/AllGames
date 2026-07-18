// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MiniGameEntryWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UMiniGameDefinitionDataAsset;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMiniGameEntrySelected, UMiniGameDefinitionDataAsset*);

/** Reusable hub card for one catalog entry. */
UCLASS()
class ALLGAMES_API UMiniGameEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnMiniGameEntrySelected OnSelected;
	void SetDefinition(UMiniGameDefinitionDataAsset* InDefinition);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	TObjectPtr<UMiniGameDefinitionDataAsset> Definition;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(Transient)
	TObjectPtr<UImage> CoverImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StateText;
};
