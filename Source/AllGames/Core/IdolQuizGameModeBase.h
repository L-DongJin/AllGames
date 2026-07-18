// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "../Data/IdolQuizCatalogDataAsset.h"
#include "IdolQuizGameModeBase.generated.h"
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnIdolQuizQuestionChanged,const FIdolQuizQuestion&,int32,int32);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnIdolQuizAnswerResolved,bool,const FString&,int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnIdolQuizFinished,int32,int32);
UCLASS()
class ALLGAMES_API AIdolQuizGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	AIdolQuizGameModeBase();
	FOnIdolQuizQuestionChanged OnQuestionChanged;
	FOnIdolQuizAnswerResolved OnAnswerResolved;
	FOnIdolQuizFinished OnQuizFinished;
	void StartQuiz();
	void SubmitAnswer(const FString& Answer);
	const FIdolQuizQuestion* GetCurrentQuestion() const;
protected:
	virtual void BeginPlay() override;
private:
	static FString NormalizeAnswer(const FString& Value);
	void AdvanceQuestion();
	UPROPERTY(EditDefaultsOnly,Category="Idol Quiz") TSoftObjectPtr<UIdolQuizCatalogDataAsset> QuestionCatalog;
	UPROPERTY(EditDefaultsOnly,Category="Idol Quiz",meta=(ClampMin="1",ClampMax="100")) int32 QuestionsPerGame=10;
	UPROPERTY(Transient) TObjectPtr<UIdolQuizCatalogDataAsset> LoadedCatalog;
	TArray<int32> QuestionOrder;
	int32 CurrentOrderIndex=INDEX_NONE;
	int32 Score=0;
	bool bRoundResolved=false;
	FTimerHandle AdvanceTimer;
};
