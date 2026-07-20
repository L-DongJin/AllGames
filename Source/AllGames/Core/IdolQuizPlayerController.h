// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IdolQuizPlayerController.generated.h"
class UIdolQuizWidget;
UCLASS()
class ALLGAMES_API AIdolQuizPlayerController:public APlayerController
{
	GENERATED_BODY()
public: UFUNCTION(Server,Reliable)void ServerSubmitQuizMessage(const FString& Message); UFUNCTION(Server,Reliable)void ServerSetQuizPlayerName(const FString& Name); UFUNCTION(Server,Reliable)void ServerRequestSkip();
protected: virtual void BeginPlay()override;
private: UPROPERTY(Transient)TObjectPtr<UIdolQuizWidget>QuizWidget;
};
