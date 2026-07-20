// Copyright Epic Games, Inc. All Rights Reserved.
#include "IdolQuizPlayerController.h"
#include "../UI/IdolQuizWidget.h"
#include "IdolQuizGameModeBase.h"
#include "IdolQuizPlayerState.h"
#include "RhythmAccountSubsystem.h"
void AIdolQuizPlayerController::BeginPlay()
{
	Super::BeginPlay();if(!IsLocalController())return;UClass* WidgetClass=LoadClass<UIdolQuizWidget>(nullptr,TEXT("/Game/UI/WBP_IdolQuiz.WBP_IdolQuiz_C"));QuizWidget=CreateWidget<UIdolQuizWidget>(this,WidgetClass?WidgetClass:UIdolQuizWidget::StaticClass());if(!QuizWidget)return;
	QuizWidget->AddToViewport();FInputModeUIOnly Mode;Mode.SetWidgetToFocus(QuizWidget->TakeWidget());SetInputMode(Mode);SetShowMouseCursor(true);FString Name=TEXT("Player");if(UGameInstance* GI=GetGameInstance())if(URhythmAccountSubsystem* Account=GI->GetSubsystem<URhythmAccountSubsystem>())if(!Account->GetUsername().IsEmpty())Name=Account->GetUsername();ServerSetQuizPlayerName(Name);
}
void AIdolQuizPlayerController::ServerSubmitQuizMessage_Implementation(const FString& Message){if(AIdolQuizGameModeBase* GM=GetWorld()->GetAuthGameMode<AIdolQuizGameModeBase>())GM->SubmitMessage(this,Message);}
void AIdolQuizPlayerController::ServerSetQuizPlayerName_Implementation(const FString& Name){if(AIdolQuizPlayerState* PS=GetPlayerState<AIdolQuizPlayerState>())PS->SetQuizPlayerName(Name);}
void AIdolQuizPlayerController::ServerRequestSkip_Implementation(){if(AIdolQuizGameModeBase* GM=GetWorld()->GetAuthGameMode<AIdolQuizGameModeBase>())GM->RequestSkip(this);}
