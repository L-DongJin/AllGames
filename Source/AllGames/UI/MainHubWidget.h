// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHubWidget.generated.h"

class UBorder;
class UButton;
class UTexture2D;
class UUniformGridPanel;
class UMiniGameCatalogDataAsset;
class UMiniGameDefinitionDataAsset;

UCLASS()
class ALLGAMES_API UMainHubWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMainHubWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Appearance")
	TObjectPtr<UTexture2D> BackgroundImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Data")
	TSoftObjectPtr<UMiniGameCatalogDataAsset> GameCatalog;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildLayout();
	void PopulateGames();
	void HandleGameSelected(UMiniGameDefinitionDataAsset* Definition);
	void SetExitVisible(bool bVisible);

	UFUNCTION() void HandleExitConfirmed();
	UFUNCTION() void HandleExitCanceled();

	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> GameGrid;
	UPROPERTY(Transient) TObjectPtr<UBorder> ExitBackground;
	UPROPERTY(Transient) TObjectPtr<UButton> ExitConfirmButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ExitCancelButton;
	bool bExitVisible = false;
};
