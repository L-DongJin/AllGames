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
	UClass* LoginClass=LoadClass<URhythmLoginWidget>(nullptr,TEXT("/Game/UI/WBP_RhythmLogin.WBP_RhythmLogin_C"));
	LoginWidget=CreateWidget<URhythmLoginWidget>(this,LoginClass?LoginClass:URhythmLoginWidget::StaticClass()); if(!LoginWidget)return;
	LoginWidget->OnLoginAccepted.AddUObject(this,&ThisClass::ShowHub); LoginWidget->AddToViewport(); LoginWidget->SetKeyboardFocus();
	FInputModeUIOnly Mode; Mode.SetWidgetToFocus(LoginWidget->TakeWidget()); SetInputMode(Mode); SetShowMouseCursor(true);
}

void AMainHubPlayerController::ShowHub()
{
	if(LoginWidget){LoginWidget->RemoveFromParent();LoginWidget=nullptr;}
	UClass* HubClass=LoadClass<UMainHubWidget>(nullptr,TEXT("/Game/UI/WBP_GameSelect.WBP_GameSelect_C"));
	if(!HubClass)HubClass=LoadClass<UMainHubWidget>(nullptr,TEXT("/Game/UI/WBP_MainHub.WBP_MainHub_C"));
	HubWidget=CreateWidget<UMainHubWidget>(this,HubClass?HubClass:UMainHubWidget::StaticClass()); if(!HubWidget)return;
	HubWidget->AddToViewport(); HubWidget->SetKeyboardFocus(); FInputModeUIOnly Mode; Mode.SetWidgetToFocus(HubWidget->TakeWidget()); SetInputMode(Mode); SetShowMouseCursor(true);
}
