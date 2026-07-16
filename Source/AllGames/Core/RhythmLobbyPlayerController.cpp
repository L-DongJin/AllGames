// Copyright Epic Games, Inc. All Rights Reserved.

#include "RhythmLobbyPlayerController.h"
#include "RhythmAccountSubsystem.h"
#include "../UI/RhythmLoginWidget.h"
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
	if (const URhythmAccountSubsystem* Accounts = GetGameInstance()->GetSubsystem<URhythmAccountSubsystem>();
		Accounts && Accounts->IsLoggedIn())
	{
		ShowLobby();
	}
	else
	{
		ShowLogin();
	}
}

void ARhythmLobbyPlayerController::ShowLogin()
{
	LoginWidget = CreateWidget<URhythmLoginWidget>(this, URhythmLoginWidget::StaticClass());
	if (!LoginWidget)
	{
		return;
	}
	LoginWidget->OnLoginAccepted.AddUObject(this, &ThisClass::ShowLobby);
	LoginWidget->AddToViewport();
	LoginWidget->SetKeyboardFocus();
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(LoginWidget->TakeWidget());
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}

void ARhythmLobbyPlayerController::ShowLobby()
{
	if (LoginWidget)
	{
		LoginWidget->RemoveFromParent();
		LoginWidget = nullptr;
	}

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
