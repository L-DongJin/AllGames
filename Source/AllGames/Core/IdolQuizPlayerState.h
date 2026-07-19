#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "IdolQuizPlayerState.generated.h"

UCLASS()
class ALLGAMES_API AIdolQuizPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	AIdolQuizPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	void SetQuizPlayerName(const FString& InName);
	void AddCorrectAnswer();
	const FString& GetQuizPlayerName()const{return QuizPlayerName;}
	int32 GetCorrectAnswers()const{return CorrectAnswers;}
private:
	UPROPERTY(Replicated) FString QuizPlayerName=TEXT("Player");
	UPROPERTY(Replicated) int32 CorrectAnswers=0;
};
