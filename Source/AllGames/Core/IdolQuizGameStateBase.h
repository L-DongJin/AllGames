#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "IdolQuizGameStateBase.generated.h"
DECLARE_MULTICAST_DELEGATE(FOnIdolQuizStateChanged);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnIdolQuizChatReceived,const FString&,const FString&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnIdolQuizFeedbackReceived,bool,const FString&,const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnIdolQuizSkipReceived,const FString&);
UCLASS()
class ALLGAMES_API AIdolQuizGameStateBase:public AGameStateBase
{
	GENERATED_BODY()
public:
	AIdolQuizGameStateBase();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	FOnIdolQuizStateChanged OnQuizStateChanged; FOnIdolQuizChatReceived OnChatReceived; FOnIdolQuizFeedbackReceived OnFeedbackReceived; FOnIdolQuizSkipReceived OnSkipReceived;
	void SetRoundState(const FSoftObjectPath& ImagePath,int32 Round,int32 Total); void SetRemainingTime(int32 Seconds); void SetHint(const FString& InHint); void SetFinished(bool bInFinished); void SetSkipProgress(int32 Votes,int32 RequiredVotes);
	UFUNCTION(NetMulticast,Reliable)void MulticastChat(const FString& PlayerName,const FString& Message);
	UFUNCTION(NetMulticast,Reliable)void MulticastFeedback(bool bCorrect,const FString& PlayerName,const FString& Message);
	UFUNCTION(NetMulticast,Reliable)void MulticastSkipFeedback(const FString& CorrectAnswer);
	const FSoftObjectPath& GetQuestionImagePath()const{return QuestionImagePath;} int32 GetRound()const{return CurrentRound;} int32 GetTotalRounds()const{return TotalRounds;} int32 GetRemainingTime()const{return RemainingTime;} const FString& GetHint()const{return Hint;} bool IsFinished()const{return bFinished;} int32 GetSkipVotes()const{return SkipVotes;} int32 GetRequiredSkipVotes()const{return RequiredSkipVotes;}
private:
	UFUNCTION()void OnRep_State();
	UPROPERTY(ReplicatedUsing=OnRep_State)FSoftObjectPath QuestionImagePath; UPROPERTY(ReplicatedUsing=OnRep_State)int32 CurrentRound=0; UPROPERTY(ReplicatedUsing=OnRep_State)int32 TotalRounds=0; UPROPERTY(ReplicatedUsing=OnRep_State)int32 RemainingTime=0; UPROPERTY(ReplicatedUsing=OnRep_State)FString Hint; UPROPERTY(ReplicatedUsing=OnRep_State)bool bFinished=false; UPROPERTY(ReplicatedUsing=OnRep_State)int32 SkipVotes=0; UPROPERTY(ReplicatedUsing=OnRep_State)int32 RequiredSkipVotes=1;
};
