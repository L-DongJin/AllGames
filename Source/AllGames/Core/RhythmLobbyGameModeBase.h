// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RhythmLobbyGameModeBase.generated.h"

/** Lightweight GameMode used only by LobbyMap. */
UCLASS()
class ALLGAMES_API ARhythmLobbyGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARhythmLobbyGameModeBase();
};
