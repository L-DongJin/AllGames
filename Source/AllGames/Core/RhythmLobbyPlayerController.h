// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RhythmLobbyPlayerController.generated.h"

class URhythmLobbyWidget;

UCLASS()
class ALLGAMES_API ARhythmLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<URhythmLobbyWidget> LobbyWidget;
};
