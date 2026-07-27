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
class UMiniGameEntryWidget;

UCLASS()
class ALLGAMES_API UMainHubWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMainHubWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Appearance")
	TObjectPtr<UTexture2D> BackgroundImage;

	/** Shared four-sided game selection card frame assigned in WBP Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Appearance")
	TObjectPtr<UTexture2D> GameCardFrameImage;

	/** Optional frame override for only the Rhythm Game card. Falls back to GameCardFrameImage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Appearance|Cards")
	TObjectPtr<UTexture2D> RhythmGameCardFrameImage;

	/** Optional frame override for only the Quiz Game card. Falls back to GameCardFrameImage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Appearance|Cards")
	TObjectPtr<UTexture2D> QuizGameCardFrameImage;

	/** Cover shown inside the Rhythm Game selection card. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Appearance|Cards")
	TObjectPtr<UTexture2D> RhythmGameCardImage;

	/** Cover shown inside the combined Quiz Game selection card. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Appearance|Cards")
	TObjectPtr<UTexture2D> QuizGameCardImage;

	/** Width and height of each reusable game selection card. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Appearance|Layout", meta = (ClampMin = "0.0"))
	FVector2D GameCardSize = FVector2D::ZeroVector;

	/** Width and height of the title TextBlock container inside every card. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Appearance|Layout", meta = (ClampMin = "0.0"))
	FVector2D GameCardTitleTextBoxSize = FVector2D(340.0f, 52.0f);

	/** Width and height of the description TextBlock container inside every card. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Appearance|Layout", meta = (ClampMin = "0.0"))
	FVector2D GameCardDescriptionTextBoxSize = FVector2D(340.0f, 66.0f);

	/** Reusable visual card widget. Adjust its hierarchy directly in WBP_GameCard. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Data")
	TSoftClassPtr<UMiniGameEntryWidget> GameCardWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AllGames|Data")
	TSoftObjectPtr<UMiniGameCatalogDataAsset> GameCatalog;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildLayout();
	void PopulateGames();
	void HandleGameSelected(UMiniGameDefinitionDataAsset* Definition);
	void SetExitVisible(bool bVisible);

	UFUNCTION() void HandleExitConfirmed();
	UFUNCTION() void HandleExitCanceled();
	UFUNCTION() void HandleRhythmGameClicked();
	UFUNCTION() void HandleQuizGameClicked();

	TObjectPtr<UUniformGridPanel> GameGrid;
	TObjectPtr<UBorder> ExitBackground;
	TObjectPtr<UButton> ExitConfirmButton;
	TObjectPtr<UButton> ExitCancelButton;
	TObjectPtr<UButton> RhythmGameButton;
	TObjectPtr<UButton> QuizGameButton;
	bool bExitVisible = false;
};
