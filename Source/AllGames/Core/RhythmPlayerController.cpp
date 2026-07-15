// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "../Data/RhythmSongDataAsset.h"
#include "../Rhythm/RhythmConductor.h"
#include "../UI/RhythmGameplayWidget.h"

ARhythmPlayerController::ARhythmPlayerController()
{
	GameplayWidgetClass = TSoftClassPtr<URhythmGameplayWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_RhythmGameplay.WBP_RhythmGameplay_C")));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> FiveKeyContextFinder(TEXT("/Game/Input/IMC_Rhythm_5Key.IMC_Rhythm_5Key"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> NineKeyContextFinder(TEXT("/Game/Input/IMC_Rhythm_9Key.IMC_Rhythm_9Key"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane1Finder(TEXT("/Game/Input/IA_Lane1.IA_Lane1"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane2Finder(TEXT("/Game/Input/IA_Lane2.IA_Lane2"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane3Finder(TEXT("/Game/Input/IA_Lane3.IA_Lane3"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane4Finder(TEXT("/Game/Input/IA_Lane4.IA_Lane4"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane5Finder(TEXT("/Game/Input/IA_Lane5.IA_Lane5"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane6Finder(TEXT("/Game/Input/IA_Lane6.IA_Lane6"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane7Finder(TEXT("/Game/Input/IA_Lane7.IA_Lane7"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane8Finder(TEXT("/Game/Input/IA_Lane8.IA_Lane8"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Lane9Finder(TEXT("/Game/Input/IA_Lane9.IA_Lane9"));

	FiveKeyMappingContext = FiveKeyContextFinder.Object;
	NineKeyMappingContext = NineKeyContextFinder.Object;
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
	ActiveKeyMode = DefaultKeyMode;
	for (TActorIterator<ARhythmConductor> It(GetWorld()); It; ++It)
	{
		if (const URhythmSongDataAsset* SongData = It->GetSongData())
		{
			ActiveKeyMode = SongData->KeyMode == ERhythmChartKeyMode::FiveKey
				? ERhythmKeyMode::FiveKey
				: ERhythmKeyMode::NineKey;
		}
		break;
	}
	ApplyKeyMode(true);

	if (IsLocalController())
	{
		if (UClass* LoadedWidgetClass = GameplayWidgetClass.LoadSynchronous())
		{
			GameplayWidget = CreateWidget<URhythmGameplayWidget>(this, LoadedWidgetClass);
			if (GameplayWidget)
			{
				GameplayWidget->AddToViewport();
			}
		}
	}
}

void ARhythmPlayerController::SetKeyMode(const ERhythmKeyMode NewKeyMode)
{
	ActiveKeyMode = NewKeyMode;
	ApplyKeyMode(false);
}

int32 ARhythmPlayerController::GetActiveLaneCount() const
{
	return ActiveKeyMode == ERhythmKeyMode::FiveKey ? 5 : 9;
}

void ARhythmPlayerController::ApplyKeyMode(const bool bClearExistingMappings)
{
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (bClearExistingMappings)
			{
				InputSubsystem->ClearAllMappings();
			}
			else
			{
				InputSubsystem->RemoveMappingContext(FiveKeyMappingContext);
				InputSubsystem->RemoveMappingContext(NineKeyMappingContext);
			}

			UInputMappingContext* SelectedContext = ActiveKeyMode == ERhythmKeyMode::FiveKey
				? FiveKeyMappingContext
				: NineKeyMappingContext;

			if (ensureMsgf(SelectedContext, TEXT("Mapping context for the selected rhythm key mode is not assigned.")))
			{
				InputSubsystem->AddMappingContext(SelectedContext, 0);
				UE_LOG(LogTemp, Log, TEXT("Rhythm input mode active: %d keys"), GetActiveLaneCount());
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
			EnhancedInput->BindAction(LaneActions[LaneIndex], ETriggerEvent::Canceled, this, &ThisClass::HandleLaneInput, LaneIndex, false);
		}
	}
}

void ARhythmPlayerController::HandleLaneInput(const FInputActionValue&, const int32 LaneIndex, const bool bPressed)
{
	UE_LOG(LogTemp, Log, TEXT("Lane %d %s"), LaneIndex + 1, bPressed ? TEXT("Pressed") : TEXT("Released"));
	OnLaneInput.Broadcast(LaneIndex, bPressed);
}
