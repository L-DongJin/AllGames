#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DrawingQuizTypes.h"
#include "DrawingQuizPlayerController.generated.h"

class UDrawingQuizWidget;
UCLASS()
class ALLGAMES_API ADrawingQuizPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	UFUNCTION(Server,Reliable) void ServerSetPlayerName(const FString& Name);
	UFUNCTION(Server,Reliable) void ServerSubmitChat(const FString& Message);
	UFUNCTION(Server,Reliable) void ServerDrawSegments(const TArray<FDrawingQuizStrokeSegment>& Segments);
	UFUNCTION(Server,Reliable) void ServerClearCanvas();
	UFUNCTION(Client,Reliable) void ClientSetSecretWord(const FString& Word);
protected:
	virtual void BeginPlay()override;
private:
	UPROPERTY(Transient) TObjectPtr<UDrawingQuizWidget> QuizWidget;
};
