// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmLobbyPlayerController.h"
#include "../UI/RhythmLobbyWidget.h"

void ARhythmLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController()) return;
	LobbyWidget = CreateWidget<URhythmLobbyWidget>(this, URhythmLobbyWidget::StaticClass());
	if (LobbyWidget)
	{
		LobbyWidget->AddToViewport();
		LobbyWidget->SetKeyboardFocus();
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(LobbyWidget->TakeWidget());
		SetInputMode(InputMode);
		SetShowMouseCursor(true);
	}
}
