// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RhythmPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLaneInput, int32, LaneIndex, bool, bPressed);

/** Receives the nine rhythm-lane inputs through Enhanced Input. */
UCLASS()
class ALLGAMES_API ARhythmPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARhythmPlayerController();

	/** Broadcasts zero-based lane input for future judgement logic. */
	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Input")
	FOnLaneInput OnLaneInput;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void HandleLaneInput(const FInputActionValue& Value, int32 LaneIndex, bool bPressed);

	UPROPERTY()
	TObjectPtr<UInputMappingContext> RhythmMappingContext;

	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> LaneActions;
};
