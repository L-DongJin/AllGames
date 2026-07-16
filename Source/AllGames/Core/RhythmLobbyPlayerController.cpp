// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmLobbyPlayerController.h"
#include "../UI/RhythmLobbyWidget.h"

ARhythmLobbyPlayerController::ARhythmLobbyPlayerController()
{
	LobbyWidgetClass = TSoftClassPtr<URhythmLobbyWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_RhythmLobby.WBP_RhythmLobby_C")));
}

void ARhythmLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController()) return;
	UClass* WidgetClass = LobbyWidgetClass.LoadSynchronous();
	LobbyWidget = CreateWidget<URhythmLobbyWidget>(
		this,
		WidgetClass ? WidgetClass : URhythmLobbyWidget::StaticClass());
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
