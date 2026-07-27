// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace QuizPlayerColors
{
	inline constexpr int32 MaxPlayers = 6;

	inline FLinearColor Get(const int32 ColorIndex)
	{
		static const FLinearColor Colors[MaxPlayers] = {
			FLinearColor(1.00f, 0.22f, 0.22f), // Player 1 - red
			FLinearColor(0.20f, 0.52f, 1.00f), // Player 2 - blue
			FLinearColor(0.20f, 0.95f, 0.36f), // Player 3 - green
			FLinearColor(1.00f, 0.84f, 0.16f), // Player 4 - yellow
			FLinearColor(0.68f, 0.38f, 1.00f), // Player 5 - purple
			FLinearColor(1.00f, 0.47f, 0.12f), // Player 6 - orange
		};
		return Colors[FMath::Clamp(ColorIndex, 0, MaxPlayers - 1)];
	}
}
