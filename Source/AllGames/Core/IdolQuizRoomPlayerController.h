#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IdolQuizRoomPlayerController.generated.h"
class UIdolQuizRoomWidget;
UCLASS()
class ALLGAMES_API AIdolQuizRoomPlayerController:public APlayerController
{
	GENERATED_BODY()
public:virtual void BeginPlay()override;
private:UPROPERTY(Transient)TObjectPtr<UIdolQuizRoomWidget>RoomWidget;
};
