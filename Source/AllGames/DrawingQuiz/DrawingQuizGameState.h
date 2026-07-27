#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "DrawingQuizTypes.h"
#include "DrawingQuizGameState.generated.h"

class ADrawingQuizPlayerState;
DECLARE_MULTICAST_DELEGATE(FOnDrawingQuizStateChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDrawingQuizStroke,const FDrawingQuizStrokeSegment&);
DECLARE_MULTICAST_DELEGATE(FOnDrawingQuizCanvasCleared);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDrawingQuizChat,const FString&,const FString&,int32);

UCLASS()
class ALLGAMES_API ADrawingQuizGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	ADrawingQuizGameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	FOnDrawingQuizStateChanged OnStateChanged;
	FOnDrawingQuizStroke OnStroke;
	FOnDrawingQuizCanvasCleared OnCanvasCleared;
	FOnDrawingQuizChat OnChat;
	void SetRoundState(int32 Round,int32 Total,ADrawingQuizPlayerState* Drawer,int32 Seconds,const FString& Hint);
	void SetRemainingTime(int32 Seconds);
	void SetFeedback(const FString& Value);
	void SetFinished(bool bValue);
	void ClearCanvasState();
	UFUNCTION(NetMulticast,Reliable) void MulticastStrokes(const TArray<FDrawingQuizStrokeSegment>& Segments);
	UFUNCTION(NetMulticast,Reliable) void MulticastChat(const FString& PlayerName,const FString& Message,int32 PlayerColorIndex);
	int32 GetRound()const{return CurrentRound;} int32 GetTotalRounds()const{return TotalRounds;} int32 GetRemainingTime()const{return RemainingTime;}
	ADrawingQuizPlayerState* GetDrawer()const{return DrawerPlayer;} const FString& GetHint()const{return Hint;} const FString& GetFeedback()const{return Feedback;} bool IsFinished()const{return bFinished;}
private:
	UFUNCTION() void OnRep_State();
	UFUNCTION() void OnRep_CanvasRevision();
	UPROPERTY(ReplicatedUsing=OnRep_State) int32 CurrentRound=0;
	UPROPERTY(ReplicatedUsing=OnRep_State) int32 TotalRounds=0;
	UPROPERTY(ReplicatedUsing=OnRep_State) int32 RemainingTime=0;
	UPROPERTY(ReplicatedUsing=OnRep_State) TObjectPtr<ADrawingQuizPlayerState> DrawerPlayer;
	UPROPERTY(ReplicatedUsing=OnRep_State) FString Hint;
	UPROPERTY(ReplicatedUsing=OnRep_State) FString Feedback;
	UPROPERTY(ReplicatedUsing=OnRep_State) bool bFinished=false;
	UPROPERTY(ReplicatedUsing=OnRep_CanvasRevision) int32 CanvasRevision=0;
};
