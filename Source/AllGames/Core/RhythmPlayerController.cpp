// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

ARhythmPlayerController::ARhythmPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextFinder(TEXT("/Game/Input/IMC_Rhythm.IMC_Rhythm"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane1Finder(TEXT("/Game/Input/IA_Lane1.IA_Lane1"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane2Finder(TEXT("/Game/Input/IA_Lane2.IA_Lane2"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane3Finder(TEXT("/Game/Input/IA_Lane3.IA_Lane3"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane4Finder(TEXT("/Game/Input/IA_Lane4.IA_Lane4"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane5Finder(TEXT("/Game/Input/IA_Lane5.IA_Lane5"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane6Finder(TEXT("/Game/Input/IA_Lane6.IA_Lane6"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane7Finder(TEXT("/Game/Input/IA_Lane7.IA_Lane7"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane8Finder(TEXT("/Game/Input/IA_Lane8.IA_Lane8"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane9Finder(TEXT("/Game/Input/IA_Lane9.IA_Lane9"));

	RhythmMappingContext = MappingContextFinder.Object;
	LaneActions = {
		Lane1Finder.Object,
		Lane2Finder.Object,
		Lane3Finder.Object,
		Lane4Finder.Object,
		Lane5Finder.Object,
		Lane6Finder.Object,
		Lane7Finder.Object,
		Lane8Finder.Object,
		Lane9Finder.Object,
	};
}

void ARhythmPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (ensureMsgf(RhythmMappingContext, TEXT("IMC_Rhythm is not assigned.")))
			{
				InputSubsystem->AddMappingContext(RhythmMappingContext, 0);
			}
		}
	}
}

void ARhythmPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(InputComponent);
	for (int32 LaneIndex = 0; LaneIndex < LaneActions.Num(); ++LaneIndex)
	{
		if (ensureMsgf(LaneActions[LaneIndex], TEXT("Input action for lane %d is not assigned."), LaneIndex + 1))
		{
			EnhancedInput->BindAction(LaneActions[LaneIndex], ETriggerEvent::Started, this, &ThisClass::HandleLaneInput, LaneIndex, true);
			EnhancedInput->BindAction(LaneActions[LaneIndex], ETriggerEvent::Completed, this, &ThisClass::HandleLaneInput, LaneIndex, false);
		}
	}
}

void ARhythmPlayerController::HandleLaneInput(const FInputActionValue&, const int32 LaneIndex, const bool bPressed)
{
	UE_LOG(LogTemp, Log, TEXT("Lane %d %s"), LaneIndex + 1, bPressed ? TEXT("Pressed") : TEXT("Released"));
	OnLaneInput.Broadcast(LaneIndex, bPressed);
}
