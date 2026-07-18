// Copyright Epic Games, Inc. All Rights Reserved.
#include "IdolQuizPlayerController.h"
#include "../UI/IdolQuizWidget.h"
void AIdolQuizPlayerController::BeginPlay()
{
	Super::BeginPlay();if(!IsLocalController())return;QuizWidget=CreateWidget<UIdolQuizWidget>(this,UIdolQuizWidget::StaticClass());if(!QuizWidget)return;
	QuizWidget->AddToViewport();FInputModeUIOnly Mode;Mode.SetWidgetToFocus(QuizWidget->TakeWidget());SetInputMode(Mode);SetShowMouseCursor(true);
}
