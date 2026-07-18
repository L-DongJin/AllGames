// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MiniGameDefinitionDataAsset.generated.h"

class UTexture2D;
class UWorld;

/** One launchable game shown by the shared AllGames hub. */
UCLASS(BlueprintType)
class ALLGAMES_API UMiniGameDefinitionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mini Game")
	FName GameId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mini Game")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mini Game", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mini Game")
	TObjectPtr<UTexture2D> CoverImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mini Game")
	TSoftObjectPtr<UWorld> EntryMap;

	/** Disabled games remain visible as coming-soon entries. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mini Game")
	bool bEnabled = true;
};
