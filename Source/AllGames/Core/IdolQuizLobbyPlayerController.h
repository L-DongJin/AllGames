#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IdolQuizLobbyPlayerController.generated.h"
class UIdolQuizLobbyWidget;
UCLASS()
class ALLGAMES_API AIdolQuizLobbyPlayerController:public APlayerController
{
	GENERATED_BODY()
public:virtual void BeginPlay()override;UFUNCTION(Server,Reliable)void ServerSetQuizPlayerName(const FString& Name);UFUNCTION(Server,Reliable)void ServerStartSelectedGame();
private:UPROPERTY(Transient)TObjectPtr<UIdolQuizLobbyWidget>LobbyWidget;
};
