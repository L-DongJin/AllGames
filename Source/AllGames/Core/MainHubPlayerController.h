// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainHubPlayerController.generated.h"

class UMainHubWidget;
class URhythmLoginWidget;

UCLASS()
class ALLGAMES_API AMainHubPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	void ShowLogin();
	void ShowHub();

	UPROPERTY(Transient) TObjectPtr<URhythmLoginWidget> LoginWidget;
	UPROPERTY(Transient) TObjectPtr<UMainHubWidget> HubWidget;
};
