// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmLobbyGameModeBase.h"
#include "RhythmLobbyPlayerController.h"

ARhythmLobbyGameModeBase::ARhythmLobbyGameModeBase()
{
	PlayerControllerClass = ARhythmLobbyPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
