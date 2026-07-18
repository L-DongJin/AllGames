// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainHubPlayerController.h"

#include "RhythmAccountSubsystem.h"
#include "../UI/MainHubWidget.h"
#include "../UI/RhythmLoginWidget.h"

void AMainHubPlayerController::BeginPlay()
{
	Super::BeginPlay(); if(!IsLocalController())return;
	const URhythmAccountSubsystem* Accounts=GetGameInstance()->GetSubsystem<URhythmAccountSubsystem>();
	if(Accounts&&Accounts->IsLoggedIn())ShowHub(); else ShowLogin();
}

void AMainHubPlayerController::ShowLogin()
{
	LoginWidget=CreateWidget<URhythmLoginWidget>(this,URhythmLoginWidget::StaticClass()); if(!LoginWidget)return;
	LoginWidget->OnLoginAccepted.AddUObject(this,&ThisClass::ShowHub); LoginWidget->AddToViewport(); LoginWidget->SetKeyboardFocus();
	FInputModeUIOnly Mode; Mode.SetWidgetToFocus(LoginWidget->TakeWidget()); SetInputMode(Mode); SetShowMouseCursor(true);
}

void AMainHubPlayerController::ShowHub()
{
	if(LoginWidget){LoginWidget->RemoveFromParent();LoginWidget=nullptr;}
	HubWidget=CreateWidget<UMainHubWidget>(this,UMainHubWidget::StaticClass()); if(!HubWidget)return;
	HubWidget->AddToViewport(); HubWidget->SetKeyboardFocus(); FInputModeUIOnly Mode; Mode.SetWidgetToFocus(HubWidget->TakeWidget()); SetInputMode(Mode); SetShowMouseCursor(true);
}
