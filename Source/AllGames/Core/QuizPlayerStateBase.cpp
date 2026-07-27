// Copyright Epic Games, Inc. All Rights Reserved.

#include "QuizPlayerStateBase.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

AQuizPlayerStateBase::AQuizPlayerStateBase()
{
	bReplicates = true;
}

void AQuizPlayerStateBase::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, PlayerColorIndex);
}

void AQuizPlayerStateBase::AssignAvailablePlayerColor()
{
	if (!HasAuthority() || PlayerColorIndex != INDEX_NONE)
	{
		return;
	}

	TSet<int32> UsedColors;
	if (const AGameStateBase* GameState =
		GetWorld() ? GetWorld()->GetGameState<AGameStateBase>() : nullptr)
	{
		for (const APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (const AQuizPlayerStateBase* Other = Cast<AQuizPlayerStateBase>(PlayerState);
				Other && Other != this && Other->PlayerColorIndex != INDEX_NONE)
			{
				UsedColors.Add(Other->PlayerColorIndex);
			}
		}
	}

	for (int32 Index = 0; Index < 6; ++Index)
	{
		if (!UsedColors.Contains(Index))
		{
			PlayerColorIndex = Index;
			return;
		}
	}
	PlayerColorIndex = FMath::Abs(GetPlayerId()) % 6;
}

void AQuizPlayerStateBase::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	if (AQuizPlayerStateBase* QuizPlayerState = Cast<AQuizPlayerStateBase>(PlayerState))
	{
		QuizPlayerState->PlayerColorIndex = PlayerColorIndex;
	}
}

void AQuizPlayerStateBase::OverrideWith(APlayerState* PlayerState)
{
	Super::OverrideWith(PlayerState);
	if (const AQuizPlayerStateBase* QuizPlayerState = Cast<AQuizPlayerStateBase>(PlayerState))
	{
		PlayerColorIndex = QuizPlayerState->PlayerColorIndex;
	}
}
