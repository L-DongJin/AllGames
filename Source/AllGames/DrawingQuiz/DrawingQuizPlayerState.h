#pragma once

#include "CoreMinimal.h"
#include "../Core/QuizPlayerStateBase.h"
#include "DrawingQuizPlayerState.generated.h"

UCLASS()
class ALLGAMES_API ADrawingQuizPlayerState : public AQuizPlayerStateBase
{
	GENERATED_BODY()
public:
	ADrawingQuizPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void SetQuizName(const FString& Value);
	void AddScore(int32 Value);
	void SetSolvedCurrentRound(bool bValue);
	const FString& GetQuizName() const { return QuizName; }
	int32 GetQuizScore() const { return QuizScore; }
	bool HasSolvedCurrentRound() const { return bSolvedCurrentRound; }
private:
	UPROPERTY(Replicated) FString QuizName = TEXT("Player");
	UPROPERTY(Replicated) int32 QuizScore = 0;
	UPROPERTY(Replicated) bool bSolvedCurrentRound = false;
};
