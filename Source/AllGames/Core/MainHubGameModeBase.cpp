// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainHubGameModeBase.h"
#include "MainHubPlayerController.h"

AMainHubGameModeBase::AMainHubGameModeBase(){ PlayerControllerClass=AMainHubPlayerController::StaticClass(); DefaultPawnClass=nullptr; }
