#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DrawingQuizTypes.h"
#include "DrawingQuizGameMode.generated.h"

class ADrawingQuizPlayerController;
class ADrawingQuizPlayerState;
UCLASS()
class ALLGAMES_API ADrawingQuizGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADrawingQuizGameMode();
	virtual void BeginPlay()override;
	void SubmitChat(ADrawingQuizPlayerController* Sender,const FString& Message);
	void SubmitStrokes(ADrawingQuizPlayerController* Sender,const TArray<FDrawingQuizStrokeSegment>& Segments);
	void ClearCanvas(ADrawingQuizPlayerController* Sender);
private:
	void StartNextRound(); void TickRound(); void ResolveRound(const FString& Feedback); void FinishGame();
	void BuildWordPool(); FString Normalize(const FString& Value)const; FString BuildHint(const FString& Word)const;
	ADrawingQuizPlayerState* GetDrawer()const;
	TArray<FString> Words; int32 CurrentWordIndex=INDEX_NONE; int32 CurrentRound=0; int32 RemainingSeconds=60; int32 DrawerIndex=0; bool bRoundResolved=false;
	FString CurrentWord; FTimerHandle StartTimer; FTimerHandle RoundTimer; FTimerHandle AdvanceTimer;
	UPROPERTY(EditDefaultsOnly,Category="Drawing Quiz",meta=(ClampMin="15",ClampMax="180")) int32 RoundDurationSeconds=60;
	UPROPERTY(EditDefaultsOnly,Category="Drawing Quiz",meta=(ClampMin="1",ClampMax="10")) int32 RoundsPerPlayer=2;
};
