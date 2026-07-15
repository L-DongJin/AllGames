// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RhythmPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class URhythmGameplayWidget;
struct FInputActionValue;

UENUM(BlueprintType)
enum class ERhythmKeyMode : uint8
{
	FiveKey UMETA(DisplayName = "5 Key"),
	NineKey UMETA(DisplayName = "9 Key"),
};

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

	/** Switches the active gameplay layout without changing judgement code. */
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Input")
	void SetKeyMode(ERhythmKeyMode NewKeyMode);

	UFUNCTION(BlueprintPure, Category = "Rhythm|Input")
	int32 GetActiveLaneCount() const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void HandleLaneInput(const FInputActionValue& Value, int32 LaneIndex, bool bPressed);
	void ApplyKeyMode(bool bClearExistingMappings);

	UPROPERTY()
	TObjectPtr<UInputMappingContext> FiveKeyMappingContext;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> NineKeyMappingContext;

	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> LaneActions;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Input")
	ERhythmKeyMode DefaultKeyMode = ERhythmKeyMode::NineKey;

	UPROPERTY(VisibleInstanceOnly, Category = "Rhythm|Input")
	ERhythmKeyMode ActiveKeyMode = ERhythmKeyMode::NineKey;

	UPROPERTY(EditDefaultsOnly, Category = "Rhythm|UI")
	TSoftClassPtr<URhythmGameplayWidget> GameplayWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<URhythmGameplayWidget> GameplayWidget;
};
