// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MiniGameCatalogDataAsset.generated.h"

class UMiniGameDefinitionDataAsset;

/** Ordered game list consumed by MainHub; adding a game does not require hub code changes. */
UCLASS(BlueprintType)
class ALLGAMES_API UMiniGameCatalogDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mini Games")
	TArray<TObjectPtr<UMiniGameDefinitionDataAsset>> Games;
};
