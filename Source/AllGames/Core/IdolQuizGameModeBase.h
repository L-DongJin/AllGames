// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "../Data/IdolQuizCatalogDataAsset.h"
#include "IdolQuizGameModeBase.generated.h"
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnIdolQuizQuestionChanged,const FIdolQuizQuestion&,int32,int32);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnIdolQuizAnswerResolved,bool,const FString&,int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnIdolQuizFinished,int32,int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnIdolQuizTimeChanged,int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnIdolQuizHintChanged,const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnIdolQuizRoundTimedOut,const FString&);
UCLASS()
class ALLGAMES_API AIdolQuizGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	AIdolQuizGameModeBase();
	FOnIdolQuizQuestionChanged OnQuestionChanged;
	FOnIdolQuizAnswerResolved OnAnswerResolved;
	FOnIdolQuizFinished OnQuizFinished;
	FOnIdolQuizTimeChanged OnTimeChanged;
	FOnIdolQuizHintChanged OnHintChanged;
	FOnIdolQuizRoundTimedOut OnRoundTimedOut;
	void StartQuiz();
	void SubmitAnswer(const FString& Answer);
	void SubmitMessage(APlayerController* Sender,const FString& Message);
	const FIdolQuizQuestion* GetCurrentQuestion() const;
	int32 GetRemainingTimeSeconds() const { return RemainingTimeSeconds; }
	FString GetCurrentHint() const { const FIdolQuizQuestion* Question=GetCurrentQuestion(); return bHintRevealed&&Question?BuildInitialHint(Question->StageName):FString(); }
protected:
	virtual void BeginPlay() override;
private:
	static FString NormalizeAnswer(const FString& Value);
	static FString BuildInitialHint(const FString& Value);
	static void AppendAliases(const FString& Aliases, TArray<FString>& OutAnswers);
	void AdvanceQuestion();
	void StartRoundTimer();
	void TickRoundTimer();
	UPROPERTY(EditDefaultsOnly,Category="Idol Quiz") TSoftObjectPtr<UDataTable> QuestionTable;
	UPROPERTY(EditDefaultsOnly,Category="Idol Quiz",meta=(ClampMin="1",ClampMax="100")) int32 QuestionsPerGame=10;
	UPROPERTY(EditDefaultsOnly,Category="Idol Quiz|Timer",meta=(ClampMin="5",ClampMax="120")) int32 RoundDurationSeconds=30;
	UPROPERTY(EditDefaultsOnly,Category="Idol Quiz|Timer",meta=(ClampMin="1",ClampMax="60")) int32 HintAfterSeconds=15;
	UPROPERTY(Transient) TObjectPtr<UDataTable> LoadedQuestionTable;
	TArray<const FIdolQuizQuestion*> EligibleQuestions;
	TArray<int32> QuestionOrder;
	int32 CurrentOrderIndex=INDEX_NONE;
	int32 Score=0;
	int32 RemainingTimeSeconds=0;
	bool bRoundResolved=false;
	bool bHintRevealed=false;
	FTimerHandle AdvanceTimer;
	FTimerHandle RoundTimer;
};
