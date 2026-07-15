// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RhythmSongCatalogDataAsset.generated.h"

class URhythmSongDataAsset;

/** Editor-managed list of songs shown by the lobby. Add future songs here without changing UI code. */
UCLASS(BlueprintType)
class ALLGAMES_API URhythmSongCatalogDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Songs")
	TArray<TObjectPtr<URhythmSongDataAsset>> Songs;
};
