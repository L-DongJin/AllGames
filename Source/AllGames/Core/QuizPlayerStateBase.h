// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "QuizPlayerStateBase.generated.h"

/** Shared replicated identity presentation used across the quiz lobby and quiz game maps. */
UCLASS(Abstract)
class ALLGAMES_API AQuizPlayerStateBase : public APlayerState
{
	GENERATED_BODY()

public:
	AQuizPlayerStateBase();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OverrideWith(APlayerState* PlayerState) override;

	int32 GetPlayerColorIndex() const { return PlayerColorIndex; }

protected:
	void AssignAvailablePlayerColor();

private:
	UPROPERTY(Replicated)
	int32 PlayerColorIndex = INDEX_NONE;
};
