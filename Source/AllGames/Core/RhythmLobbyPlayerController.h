// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RhythmLobbyPlayerController.generated.h"

class URhythmLobbyWidget;
class URhythmLoginWidget;

UCLASS()
class ALLGAMES_API ARhythmLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	ARhythmLobbyPlayerController();
	virtual void BeginPlay() override;

private:
	void ShowLogin();
	void ShowLobby();

	UPROPERTY(Transient)
	TObjectPtr<URhythmLoginWidget> LoginWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Rhythm|UI")
	TSoftClassPtr<URhythmLobbyWidget> LobbyWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<URhythmLobbyWidget> LobbyWidget;
};
